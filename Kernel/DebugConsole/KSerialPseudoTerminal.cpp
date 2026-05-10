// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 21.02.2026 22:00

#include <mutex>
#include <termios.h>

#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/KProcess.h>
#include <Kernel/KProcessGroups.h>
#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/DebugConsole/KSerialPseudoTerminal.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/Kpty.h>

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSerialPseudoTerminal::KSerialPseudoTerminal(int serialReadFD, int serialWriteFD, bool queryTerminalSize)
    : KThread("serialpty")
    , m_SerialReadFD(serialReadFD)
    , m_SerialWriteFD(serialWriteFD)
    , m_QueryTerminalSize(queryTerminalSize)
    , m_TerminalSizeNotifier("ptysznfy", CLOCK_MONOTONIC, 0)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialPseudoTerminal::Setup()
{
    Start_trw(KSpawnThreadFlag::SpawnProcess);
    while (!m_PTYReady.load()) {
        snooze_ms(1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KSerialPseudoTerminal::Run()
{
    PString ptyPath;

#ifdef PADOS_MODULE_POSIX_SIGNALS
    ksignal_trw(SIGHUP, SIG_IGN);
    ksignal_trw(SIGINT, SIG_IGN);
    ksignal_trw(SIGQUIT, SIG_IGN);
    ksignal_trw(SIGALRM, SIG_IGN);
    ksignal_trw(SIGCHLD, SIG_DFL);
    ksignal_trw(SIGTTIN, SIG_IGN);
    ksignal_trw(SIGTTOU, SIG_IGN);
#endif // PADOS_MODULE_POSIX_SIGNALS

    for (int i = 0; i < 10000; ++i)
    {
        stat_t sStat;

        ptyPath = PString::format_string("/dev/pty/master/pty{}", i);

        if (stat(ptyPath.c_str(), &sStat) == -1)
        {
            m_MasterPTY = open(ptyPath.c_str(), O_RDWR | O_NONBLOCK | O_CREAT);
            if (m_MasterPTY >= 0)
            {
                ptyPath = PString::format_string("/dev/pty/slave/pty{}", i);
                break;
            }
        }
    }

    if (m_MasterPTY == -1)
    {
        m_PTYReady.store(true);
        return nullptr;
    }

    ksetsid_trw();

    m_SlavePTY = open(ptyPath.c_str(), O_RDWR);

    ktcsetpgrp_trw(m_SlavePTY, kgetpgrp());

    termios sTerm;

    ktcgetattr_trw(m_SlavePTY, &sTerm);
    sTerm.c_iflag = 0;
    sTerm.c_oflag = OPOST | ONLCR;
    sTerm.c_lflag = ISIG;
    ktcsetattr_trw(m_SlavePTY, TCSANOW, &sTerm);

    struct winsize winSize = {};

    winSize.ws_col = 80;
    winSize.ws_row = 24;

    kdevice_control_trw(m_SlavePTY, TIOCSWINSZ, &winSize, sizeof(winSize), nullptr, 0);

    KDebugConsole debugConsole(ptyPath.c_str());
    debugConsole.Setup();

    fcntl(m_SerialReadFD, F_SETFL, fcntl(m_SerialReadFD, F_GETFL) | O_NONBLOCK);

    m_PTYReady.store(true);

    KObjectWaitGroup waitGroup("ptyselect");
    waitGroup.AddFile_trw(m_SerialReadFD);
    waitGroup.AddFile_trw(m_MasterPTY);
    waitGroup.AddObject_trw(&m_TerminalSizeNotifier);

    TimeValNanos nextQueryTime;

    for (;;)
    {
        if (m_QueryTerminalSize)
        {
            waitGroup.WaitDeadline(nextQueryTime);
            const TimeValNanos curTime = get_monotonic_time();
            if (curTime >= nextQueryTime)
            {
                nextQueryTime = curTime + TimeValNanos::FromMilliseconds(250);
                SendANSICode(PANSI_ControlCode::XTerm_XTWINOPS, 18);
            }
        }
        else
        {
            waitGroup.Wait();
        }
        while (m_TerminalSizeNotifier.TryAcquire() == PErrorCode::Success) {}
        if (m_PendingTerminalSizeChange.load())
        {
            m_PendingTerminalSizeChange.store(false);
            ApplyTerminalSize(
                m_PendingWidth.load(),
                m_PendingHeight.load(),
                m_PendingPixelWidth.load(),
                m_PendingPixelHeight.load()
            );
        }

        try
        {
            char buffer[64];
            const size_t length = kread_trw(m_SerialReadFD, buffer, sizeof(buffer));
            if (length > 0) {
                ProcessSerialInput(buffer, length);
            }
        }
        catch (std::exception&) {}

        try
        {
            char buffer[32];
            const size_t length = kread_trw(m_MasterPTY, buffer, sizeof(buffer));
            if (length > 0) {
                kwrite(m_SerialWriteFD, buffer, length);
            }
        }
        catch (std::exception&) {}
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialPseudoTerminal::SendToSerial(const char* text, size_t length)
{
    kwrite(m_SerialWriteFD, text, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialPseudoTerminal::ProcessSerialInput(const char* buffer, size_t length)
{
    SendToSlave(buffer, length);

    for (size_t i = 0; i < length; ++i)
    {
        const PANSI_ControlCode controlChar = m_ANSICodeParser.ProcessCharacter(buffer[i]);
        if (controlChar != PANSI_ControlCode::None)
        {
            if (controlChar != PANSI_ControlCode::Pending) {
                ProcessControlChar(controlChar, m_ANSICodeParser.GetArguments());
            }
            continue;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialPseudoTerminal::ProcessControlChar(PANSI_ControlCode controlChar, const std::vector<int>& args)
{
    if (controlChar == PANSI_ControlCode::XTerm_XTWINOPS)
    {
        if (args.size() >= 3 && args[0] == 8) {
            ApplyTerminalSize(uint16_t(args[2]), uint16_t(args[1]), 0, 0);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Called from external threads — atomics make the lock-free store safe.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialPseudoTerminal::SetTerminalSize(uint16_t width, uint16_t height, uint16_t pixelWidth, uint16_t pixelHeight)
{
    m_PendingWidth.store(width);
    m_PendingHeight.store(height);
    m_PendingPixelWidth.store(pixelWidth);
    m_PendingPixelHeight.store(pixelHeight);
    m_PendingTerminalSizeChange.store(true);
    m_TerminalSizeNotifier.Release();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialPseudoTerminal::ApplyTerminalSize(uint16_t width, uint16_t height, uint16_t pixelWidth, uint16_t pixelHeight)
{
    const PIPoint size(width, height);
    if (size != m_TerminalSize)
    {
        m_TerminalSize = size;
        struct winsize winSize;

        winSize.ws_col    = uint16_t(m_TerminalSize.x);
        winSize.ws_row    = uint16_t(m_TerminalSize.y);
        winSize.ws_xpixel = pixelWidth;
        winSize.ws_ypixel = pixelHeight;

        kdevice_control_trw(m_MasterPTY, TIOCSWINSZ, &winSize, sizeof(winSize), nullptr, 0);
    }
}


} // namespace kernel
