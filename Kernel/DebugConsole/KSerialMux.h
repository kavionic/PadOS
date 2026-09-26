// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    KSerialMux(const PString& portPath);

    void Setup();

    virtual void* Run() override;

private:
    struct Channel
    {
        Channel(int termReadFD, int termWriteFD, int muxWriteFD, int muxReadFD);

        KSerialPseudoTerminal Terminal;
        int InputPipeWriteFD = -1;
        int OutputPipeReadFD = -1;
    };

    void RunMux();
    void CloseSerialPort();
    void ProcessIncomingByte(uint8_t byte);
    void DispatchFrame(uint16_t channelID, const uint8_t* data, size_t length);
    void HandleControlFrame(const ShellMuxControlPayload& payload);
    void SendMuxFrame(uint16_t channelID, const char* data, size_t length);
    void SendToSerial(const char* data, size_t length);
    
    Channel& CreateChannel(uint16_t channelID);
    void     DestroyChannel(uint16_t channelID);
    void     ClearChannels();

    KObjectWaitGroup m_WaitGroup;
    KMutex           m_SerialWriteMutex;
    int              m_SerialFD = -1;
    PString          m_PortPath;

    std::map<uint16_t, Channel> m_Channels;

    enum class ParseState { SyncByte0, SyncByte1, Header, Payload };
    ParseState           m_ParseState = ParseState::SyncByte0;
    uint8_t              m_HeaderBytes[4];
    size_t               m_HeaderBytesReceived = 0;
    ShellMuxHeader       m_CurrentHeader;
    std::vector<uint8_t> m_PayloadBuf;
};


} // namespace kernel
