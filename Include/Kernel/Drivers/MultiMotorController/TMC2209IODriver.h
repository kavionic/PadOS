// This file is part of PadOS.
//
// Copyright (C) 2021-2025 Kurt Skauen <http://kavionic.com/>
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
    void Setup(const char* controlPortPath, uint32_t baudrate);

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
