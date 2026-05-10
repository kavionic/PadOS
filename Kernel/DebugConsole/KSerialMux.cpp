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
// Created: 05.05.2026

#include <mutex>

#include <fcntl.h>
#include <PadOS/Time.h>
#include <Kernel/KProcess.h>
#include <Kernel/VFS/KPipeFilesystem.h>
#include <Kernel/KProcessGroups.h>
#include <Kernel/DebugConsole/KSerialPseudoTerminal.h>
#include <Kernel/DebugConsole/KSerialMux.h>
#include <Kernel/VFS/FileIO.h>
#include <Utils/ANSIEscapeCodeParser.h>

namespace kernel
{

// Low byte then high byte of magic 0xA55A in little-endian
static constexpr uint8_t MAGIC_BYTE0 = static_cast<uint8_t>(ShellMuxHeader::MAGIC & 0xFF);
static constexpr uint8_t MAGIC_BYTE1 = static_cast<uint8_t>(ShellMuxHeader::MAGIC >> 8);


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSerialMux::KSerialMux(const PString& portPath)
    : KThread("serialmux")
    , m_WaitGroup("serialmuxwg")
    , m_SerialWriteMutex("serialmuxmtx", PEMutexRecursionMode_RaiseError)
    , m_PortPath(portPath)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::Setup()
{
    Start_trw(KSpawnThreadFlag::SpawnProcess);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KSerialMux::Run()
{
#ifdef PADOS_MODULE_POSIX_SIGNALS
    ksignal_trw(SIGHUP, SIG_IGN);
    ksignal_trw(SIGINT, SIG_IGN);
    ksignal_trw(SIGQUIT, SIG_IGN);
    ksignal_trw(SIGALRM, SIG_IGN);
    ksignal_trw(SIGCHLD, SIG_DFL);
    ksignal_trw(SIGTTIN, SIG_IGN);
    ksignal_trw(SIGTTOU, SIG_IGN);
#endif // PADOS_MODULE_POSIX_SIGNALS

    RunMux();
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::RunMux()
{
    for (;;)
    {
        try
        {
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
            try
            {
                m_WaitGroup.Wait();

                try
                {
                    char inBuffer[64];
                    const size_t inLength = kread_trw(m_SerialFD, inBuffer, sizeof(inBuffer));
                    for (size_t i = 0; i < inLength; ++i) {
                        ProcessIncomingByte(static_cast<uint8_t>(inBuffer[i]));
                    }
                }
                catch (std::exception& exc)
                {
                    kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_PTY, "Caught exception while reading serial port: {}.", exc.what());
                    if (!m_PortPath.empty())
                    {
                        m_WaitGroup.RemoveFile_trw(m_SerialFD);
                        kclose(m_SerialFD);
                        m_SerialFD = -1;
                    }
                    ClearChannels();
                    m_ParseState = ParseState::SyncByte0;
                    continue;
                }

                for (auto& [channelID, channel] : m_Channels)
                {
                    try
                    {
                        char outBuffer[64];
                        const size_t outLength = kread_trw(channel.OutputPipeReadFD, outBuffer, sizeof(outBuffer));
                        if (outLength > 0) {
                            SendMuxFrame(channelID, outBuffer, outLength);
                        }
                    }
                    catch (std::exception&) {}
                }
            }
            catch (std::exception& exc)
            {
                snooze_ms(100);
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

void KSerialMux::ProcessIncomingByte(uint8_t byte)
{
    switch (m_ParseState)
    {
        case ParseState::SyncByte0:
            if (byte == MAGIC_BYTE0) {
                m_ParseState = ParseState::SyncByte1;
            }
            break;

        case ParseState::SyncByte1:
            if (byte == MAGIC_BYTE1) {
                m_HeaderBytesReceived = 0;
                m_ParseState = ParseState::Header;
            } else if (byte == MAGIC_BYTE0) {
                // Stay in SyncByte1 — consecutive 0x5A bytes
            } else {
                m_ParseState = ParseState::SyncByte0;
            }
            break;

        case ParseState::Header:
            m_HeaderBytes[m_HeaderBytesReceived++] = byte;
            if (m_HeaderBytesReceived == 4)
            {
                m_CurrentHeader.Magic     = ShellMuxHeader::MAGIC;
                m_CurrentHeader.ChannelID = uint16_t(m_HeaderBytes[0]) | (uint16_t(m_HeaderBytes[1]) << 8);
                m_CurrentHeader.Length    = uint16_t(m_HeaderBytes[2]) | (uint16_t(m_HeaderBytes[3]) << 8);

                if (m_CurrentHeader.Length == 0)
                {
                    DispatchFrame(m_CurrentHeader.ChannelID, nullptr, 0);
                    m_ParseState = ParseState::SyncByte0;
                }
                else if (m_CurrentHeader.Length > SHELL_MUX_MAX_PAYLOAD)
                {
                    m_ParseState = ParseState::SyncByte0;
                }
                else
                {
                    m_PayloadBuf.clear();
                    m_PayloadBuf.reserve(m_CurrentHeader.Length);
                    m_ParseState = ParseState::Payload;
                }
            }
            break;

        case ParseState::Payload:
            m_PayloadBuf.push_back(byte);
            if (m_PayloadBuf.size() == m_CurrentHeader.Length)
            {
                DispatchFrame(m_CurrentHeader.ChannelID, m_PayloadBuf.data(), m_PayloadBuf.size());
                m_ParseState = ParseState::SyncByte0;
            }
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::DispatchFrame(uint16_t channelID, const uint8_t* data, size_t length)
{
    if (channelID == SHELL_MUX_CONTROL_CHANNEL)
    {
        if (length >= sizeof(ShellMuxControlPayload))
        {
            ShellMuxControlPayload payload;
            memcpy(&payload, data, sizeof(payload));

            if (payload.Command == ShellMuxCommand::WindowSizeChange)
            {
                if (length >= sizeof(ShellMuxWindowSizePayload))
                {
                    ShellMuxWindowSizePayload sizePayload;
                    memcpy(&sizePayload, data, sizeof(sizePayload));
                    auto iter = m_Channels.find(sizePayload.ChannelID);
                    if (iter != m_Channels.end()) {
                        iter->second.Terminal.SetTerminalSize(sizePayload.Width, sizePayload.Height, sizePayload.PixelWidth, sizePayload.PixelHeight);
                    }
                }
            }
            else
            {
                HandleControlFrame(payload);
            }
        }
        return;
    }

    auto iter = m_Channels.find(channelID);
    if (iter != m_Channels.end()) {
        kwrite(iter->second.InputPipeWriteFD, data, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::HandleControlFrame(const ShellMuxControlPayload& payload)
{
    if (payload.Command == ShellMuxCommand::OpenChannel)
    {
        uint16_t newChannelID = 1;
        while (m_Channels.count(newChannelID) != 0) {
            ++newChannelID;
        }
        CreateChannel(newChannelID);

        ShellMuxControlPayload ack;
        ack.Command   = ShellMuxCommand::OpenChannelAck;
        ack.ChannelID = newChannelID;
        SendMuxFrame(SHELL_MUX_CONTROL_CHANNEL, reinterpret_cast<const char*>(&ack), sizeof(ack));
    }
    else if (payload.Command == ShellMuxCommand::CloseChannel)
    {
        DestroyChannel(payload.ChannelID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::SendMuxFrame(uint16_t channelID, const char* data, size_t length)
{
    ShellMuxHeader header;
    header.Magic     = ShellMuxHeader::MAGIC;
    header.ChannelID = channelID;
    header.Length    = uint16_t(length);

    std::lock_guard<KMutex> lock(m_SerialWriteMutex);
    kwrite(m_SerialFD, &header, sizeof(header));
    if (length > 0) {
        kwrite(m_SerialFD, data, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::SendToSerial(const char* data, size_t length)
{
    std::lock_guard<KMutex> lock(m_SerialWriteMutex);
    kwrite(m_SerialFD, data, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSerialMux::Channel::Channel(int termReadFD, int termWriteFD, int muxWriteFD, int muxReadFD)
    : Terminal(termReadFD, termWriteFD)
    , InputPipeWriteFD(muxWriteFD)
    , OutputPipeReadFD(muxReadFD)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSerialMux::Channel& KSerialMux::CreateChannel(uint16_t channelID)
{
    int inputPipe[2];
    int outputPipe[2];
    kpipe_trw(inputPipe);
    kpipe_trw(outputPipe);

    fcntl(outputPipe[0], F_SETFL, fcntl(outputPipe[0], F_GETFL) | O_NONBLOCK);

    auto result  = m_Channels.try_emplace(channelID, inputPipe[0], outputPipe[1], inputPipe[1], outputPipe[0]);
    Channel& channel = result.first->second;

    channel.Terminal.SetDeleteOnExit(false);
    channel.Terminal.Setup();

    m_WaitGroup.AddFile_trw(outputPipe[0]);

    return channel;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::DestroyChannel(uint16_t channelID)
{
    auto iter = m_Channels.find(channelID);
    if (iter != m_Channels.end())
    {
        m_WaitGroup.RemoveFile_trw(iter->second.OutputPipeReadFD);
        kclose(iter->second.InputPipeWriteFD);
        kclose(iter->second.OutputPipeReadFD);
        m_Channels.erase(iter);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSerialMux::ClearChannels()
{
    for (auto& [channelID, channel] : m_Channels)
    {
        m_WaitGroup.RemoveFile_trw(channel.OutputPipeReadFD);
        kclose(channel.InputPipeWriteFD);
        kclose(channel.OutputPipeReadFD);
    }
    m_Channels.clear();
}



} // namespace kernel
