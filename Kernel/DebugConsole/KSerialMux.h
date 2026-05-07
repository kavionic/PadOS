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

#pragma once

#include <map>
#include <vector>

#include <Kernel/KThread.h>
#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/KMutex.h>
#include <Kernel/KTime.h>
#include <SerialConsole/ShellMuxProtocol.h>

#include "KSerialPseudoTerminal.h"

namespace kernel
{

class KSerialPseudoTerminal;

class KSerialMux : public KThread
{
public:
    KSerialMux(int serialFD);
    KSerialMux(const PString& portPath);

    void EnableMux();
    void Setup();

    virtual void* Run() override;

private:
    struct ChannelState
    {
        KSerialPseudoTerminal   Terminal;
        TimeValNanos            NextSizeQueryTime;
    };

    void RunPassthrough();
    void RunMux();
    void ProcessIncomingByte(uint8_t byte);
    void DispatchFrame(uint16_t channelID, const uint8_t* data, size_t length);
    void HandleControlFrame(const ShellMuxControlPayload& payload);
    void SendMuxFrame(uint16_t channelID, const char* data, size_t length);
    void SendToSerial(const char* data, size_t length);
    ChannelState&   CreateChannel(uint16_t channelID);
    void            DestroyChannel(uint16_t channelID);
    TimeValNanos GetNextSizeQueryDeadline() const;
    void SendSizeQueries();

    KObjectWaitGroup m_WaitGroup;
    KMutex           m_SerialWriteMutex;
    int              m_SerialFD = -1;
    PString          m_PortPath;
    bool             m_MuxEnabled = false;

    std::map<uint16_t, ChannelState> m_Channels;

    enum class ParseState { SyncByte0, SyncByte1, Header, Payload };
    ParseState           m_ParseState = ParseState::SyncByte0;
    uint8_t              m_HeaderBytes[4];
    size_t               m_HeaderBytesReceived = 0;
    ShellMuxHeader       m_CurrentHeader;
    std::vector<uint8_t> m_PayloadBuf;
};


} // namespace kernel
