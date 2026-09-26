// This file is part of PadOS.
//
// Copyright (c) 2021-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.03.2021 22:30

#pragma once

#include <DeviceControl/USART.h>
#include <Kernel/KMutex.h>

namespace kernel
{

class TMC2209IODriver : public PtrTarget
{
public:
    TMC2209IODriver() : m_Mutex("TMC2209SerDrv", PEMutexRecursionMode_RaiseError) {}
    void Setup(const PString& controlPortPath, uint32_t baudrate);

    uint32_t    ReadRegister(uint8_t chipAddress, uint8_t registerAddress);
    void        WriteRegister(uint8_t chipAddress, uint8_t registerAddress, uint32_t data);

private:
    void SerialRead(void* buffer, size_t length);
    void SerialWrite(const void* buffer, size_t length);

    void    WaitForIdle();

    KMutex          m_Mutex;
    TimeValNanos    m_LastActiveTime;
    uint32_t        m_Baudrate = 400000;
    int             m_ControlPort = -1;
    int             m_CurrentChip = 0;
    USARTPin        m_ActivUARTPin = USARTPin::TX;
};


} // namespace kernel
