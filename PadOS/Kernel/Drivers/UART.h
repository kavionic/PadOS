// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 01.12.2017 01:12:19

#pragma once


#include "sam.h"

#include <stdint.h>
#include <atomic>

namespace kernel
{

class UART
{
public:
    enum class Channels : int8_t
    {
        Channel_None    = 0x7f,
        Channel0_A9A10  = 0x00,

        Channel1_A5A4   = 0x01,
        Channel1_A5A6   = 0x11,
        Channel1_A5D26  = 0x21,

        Channel2_D25D26 = 0x02,

        Channel3_D28D30 = 0x13,
        Channel3_D28D31 = 0x23,

        Channel4_D18D3  = 0x04,
        Channel4_D18D19 = 0x14,

        ChannelIDMask = 0x0f,
        ChannelCount = 5
    };
    enum class Parity
    {
        EVEN  = UART_MR_PAR_EVEN,
        ODD   = UART_MR_PAR_ODD,
        SPACE = UART_MR_PAR_SPACE,
        MARK  = UART_MR_PAR_MARK,
        NONE  = UART_MR_PAR_NO
    };

    typedef bool ResultCallback(void* userObject, void* data, int32_t length);
    
    UART();
    ~UART();
    
    bool Initialize(Channels channel, uint32_t baudrate);
    
    void SetParity(Parity parity) { m_Port->UART_MR = (m_Port->UART_MR & ~UART_MR_PAR_Msk) | uint32_t(parity); }
    bool EnableTX(bool enable);
    bool EnableRX(bool enable);
    void ResetStatus() { m_Port->UART_CR = UART_CR_RSTSTA_Msk; }

    int32_t Send(const void* buffer, int32_t length);
    int32_t Receive(void* buffer, int32_t length);

    int32_t SendAsync(ResultCallback* callback, void* userObject, const void* buffer, int32_t length);
    int32_t ReceiveAsync(ResultCallback* callback, void* userObject, void* buffer, int32_t length);

    void Cancel();
    bool IsIdle() const { return m_State == State_e::Idle; }

private:
    friend void ::UART0_Handler();
    friend void ::UART1_Handler();
    friend void ::UART2_Handler();
    friend void ::UART3_Handler();
    friend void ::UART4_Handler();
    
    enum class State_e
    {
        Idle,
        Reading,
        Writing
    };
    
    void HandleIRQ();
    
    Uart*    m_Port = nullptr;
    Channels m_Channel = Channels::Channel_None;

    std::atomic<State_e> m_State;
    void*   m_Buffer = nullptr;
    int32_t m_Length = 0;
    int32_t m_CurPos = 0;
    ResultCallback* m_Callback = nullptr;
    void*           m_UserObject = nullptr;
    
    UART( const UART &c );
    UART& operator=( const UART &c );
};

} // namespace
