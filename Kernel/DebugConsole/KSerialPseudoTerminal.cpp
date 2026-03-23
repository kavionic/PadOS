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

#include <termios.h>

#include <Kernel/KProcess.h>
#include <Kernel/KProcessGroups.h>
#include <Kernel/DebugConsole/KSerialPseudoTerminal.h>
#include <Kernel/VFS/Kpty.h>

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSerialPseudoTerminal::KSerialPseudoTerminal(int serialFD)
    : KThread("serialpty")
    , m_WaitGroup("serialptywg")
{
    m_SerialFD = serialFD;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSerialPseudoTerminal::KSerialPseudoTerminal(const PString& portPath)
    : KThread("serialpty")
    , m_WaitGroup("serialptywg")
    , m_PortPath(portPath)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialPseudoTerminal::Setup()
{
    Start_trw(KSpawnThreadFlag::SpawnProcess);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KSerialPseudoTerminal::Run()
{
    PString ptyPath;

    ksignal_trw(SIGHUP, SIG_IGN);
    ksignal_trw(SIGINT, SIG_IGN);
    ksignal_trw(SIGQUIT, SIG_IGN);
    ksignal_trw(SIGALRM, SIG_IGN);
    ksignal_trw(SIGCHLD, SIG_DFL);
    ksignal_trw(SIGTTIN, SIG_IGN);
    ksignal_trw(SIGTTOU, SIG_IGN);

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

    if (m_MasterPTY == -1) {
        return nullptr;
    }

    m_WaitGroup.AddFile_trw(m_MasterPTY);

    ksetsid_trw();

    m_SlavePTY = open(ptyPath.c_str(), O_RDWR);

    ktcsetpgrp_trw(m_SlavePTY, kgetpgrp());

    termios	sTerm;

    ktcgetattr_trw(m_SlavePTY, &sTerm);
    sTerm.c_iflag = 0;
    sTerm.c_oflag = 0;
    sTerm.c_lflag = ISIG;
    ktcsetattr_trw(m_SlavePTY, TCSANOW, &sTerm);

    struct winsize winSize = {};

    winSize.ws_col = 80;
    winSize.ws_row = 24;

    kdevice_control_trw(m_SlavePTY, TIOCSWINSZ, &winSize, sizeof(winSize), nullptr, 0);

    KDebugConsole debugConsole(ptyPath.c_str());
    debugConsole.Setup();

    if (m_PortPath.empty() && m_SerialFD != -1) {
        m_SerialFD = kreopen_file_trw(m_SerialFD, O_RDWR | O_DIRECT | O_NONBLOCK);
        m_WaitGroup.AddFile_trw(m_SerialFD);
    }
    for (;;)
    {
        try
        {
            char buffer[32];

            if (m_SerialFD == -1)
            {
                try
                {
                    m_SerialFD = kopen_trw(m_PortPath.c_str(), O_RDWR | O_DIRECT | O_NONBLOCK);
                    m_WaitGroup.AddFile_trw(m_SerialFD);
                }
                catch (std::exception& exc)
                {
                    snooze_ms(100);
                    continue;
                }
            }
            size_t length;
            try
            {
                m_WaitGroup.WaitDeadline(m_NextSizeQueryTime);

                const TimeValNanos curTime = get_monotonic_time();

                if (curTime >= m_NextSizeQueryTime)
                {
                    m_NextSizeQueryTime = curTime + TimeValNanos::FromMilliseconds(250);
                    SendANSICode(PANSI_ControlCode::XTerm_XTWINOPS, 18);
                }

                length = kread_trw(m_SerialFD, buffer, sizeof(buffer));

                if (length > 0) {
                    ProcessSerialInput(buffer, length);
                }
                length = kread_trw(m_MasterPTY, buffer, sizeof(buffer));
                if (length > 0) {
                    kwrite(m_SerialFD, buffer, length);
                }
            }
            catch (std::exception& exc)
            {
                if (!m_PortPath.empty())
                {
                    m_WaitGroup.RemoveFile_trw(m_SerialFD);
                    kclose(m_SerialFD);
                    m_SerialFD = -1;
                }
                continue;
            }
        }
        catch (std::exception& exc)
        {
            snooze_ms(100);
        }
    }
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
        if (args.size() >= 3 && args[0] == 8)
        {
            PIPoint size(args[2], args[1]);
            if (size != m_TerminalSize)
            {
                m_TerminalSize = size;
                struct winsize winSize;

                winSize.ws_col = uint16_t(m_TerminalSize.x);
                winSize.ws_row = uint16_t(m_TerminalSize.y);
                winSize.ws_xpixel = 0;
                winSize.ws_ypixel = 0;

                kdevice_control_trw(m_MasterPTY, TIOCSWINSZ, &winSize, sizeof(winSize), nullptr, 0);
            }
        }
    }
}


} // namespace kernel
