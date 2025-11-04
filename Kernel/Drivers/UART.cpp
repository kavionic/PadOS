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

#include "UART.h"
#include "Kernel/HAL/SAME70System.h"
#include "Kernel/HAL/DigitalPort.h"

namespace kernel
{

static UART* g_Handlers[int(UART::Channels::ChannelCount)];

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UART0_Handler()
{
    if (g_Handlers[0] != nullptr)
    {
        g_Handlers[0]->HandleIRQ();
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UART1_Handler()
{
    if (g_Handlers[1] != nullptr)
    {
        g_Handlers[1]->HandleIRQ();
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UART2_Handler()
{
    if (g_Handlers[2] != nullptr)
    {
        g_Handlers[2]->HandleIRQ();
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UART3_Handler()
{
    if (g_Handlers[3] != nullptr)
    {
        g_Handlers[3]->HandleIRQ();
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UART4_Handler()
{
    if (g_Handlers[4] != nullptr)
    {
        g_Handlers[4]->HandleIRQ();
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

UART::UART()
{
    m_State = State_e::Idle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

UART::~UART()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool UART::Initialize(Channels channel, uint32_t baudrate)
{
    int channelIndex = int(channel) & int(Channels::ChannelIDMask);
    switch(channelIndex)
    {
        case 0:
            m_Port = UART0;
            SAME70System::EnablePeripheralClock(ID_UART0);
            NVIC_SetPriority(UART0_IRQn, 6);
            NVIC_ClearPendingIRQ(UART0_IRQn);
            NVIC_EnableIRQ(UART0_IRQn);
            break;
        case 1:
            m_Port = UART1;
            SAME70System::EnablePeripheralClock(ID_UART1);
            NVIC_SetPriority(UART1_IRQn, 6);
            NVIC_ClearPendingIRQ(UART1_IRQn);
            NVIC_EnableIRQ(UART1_IRQn);
            break;
        case 2:
            m_Port = UART2;
            SAME70System::EnablePeripheralClock(ID_UART2);
            NVIC_SetPriority(UART2_IRQn, 6);
            NVIC_ClearPendingIRQ(UART2_IRQn);
            NVIC_EnableIRQ(UART2_IRQn);
            break;
        case 3:
            m_Port = UART3;
            SAME70System::EnablePeripheralClock(ID_UART3);
            NVIC_SetPriority(UART3_IRQn, 6);
            NVIC_ClearPendingIRQ(UART3_IRQn);
            NVIC_EnableIRQ(UART3_IRQn);
            break;
        case 4:
            m_Port = UART4;
            SAME70System::EnablePeripheralClock(ID_UART4);
            NVIC_SetPriority(UART4_IRQn, 6);
            NVIC_ClearPendingIRQ(UART4_IRQn);
            NVIC_EnableIRQ(UART4_IRQn);
            break;

        default:
            return false;
    }
    m_Channel = channel;
    g_Handlers[channelIndex] = this;
    uint32_t peripheralFrequency = SAME70System::GetFrequencyPeripheral();
    uint32_t divider = (peripheralFrequency + baudrate * 8) / (baudrate * 16);
    m_Port->UART_BRGR = divider;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool UART::EnableTX(bool enable)
{
    if (m_Port == nullptr) return false;

    if (enable)
    {
        switch(m_Channel)
        {
            case Channels::Channel0_A9A10:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN10_bm, DigitalPinPeripheralID::A);            
                break;
            case Channels::Channel1_A5A4:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN4_bm, DigitalPinPeripheralID::C); // TX alt. 1
                break;
            case Channels::Channel1_A5A6:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN6_bm, DigitalPinPeripheralID::C); // TX alt. 2
                break;
            case Channels::Channel1_A5D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN26_bm, DigitalPinPeripheralID::D); // TX alt. 3
                break;
            case Channels::Channel2_D25D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN26_bm, DigitalPinPeripheralID::C);
                break;
            case Channels::Channel3_D28D30:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN30_bm, DigitalPinPeripheralID::A); // TX alt. 1
                break;
            case Channels::Channel3_D28D31:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN31_bm, DigitalPinPeripheralID::B); // TX alt. 2
                break;
            case Channels::Channel4_D18D3:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN3_bm, DigitalPinPeripheralID::C); // TX alt. 1
                break;
            case Channels::Channel4_D18D19:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN19_bm, DigitalPinPeripheralID::C); // TX alt. 2
                break;
            default:
                return false;
        }
        m_Port->UART_CR = UART_CR_TXEN_Msk;
        return true;
    }
    else
    {
        m_Port->UART_CR = UART_CR_TXDIS_Msk;
        switch(m_Channel)
        {
            case Channels::Channel0_A9A10:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN10_bm, DigitalPinPeripheralID::None);
                break;
            case Channels::Channel1_A5A4:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN4_bm, DigitalPinPeripheralID::None); // TX alt. 1
                break;
            case Channels::Channel1_A5A6:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN6_bm, DigitalPinPeripheralID::None); // TX alt. 2
                break;
            case Channels::Channel1_A5D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN26_bm, DigitalPinPeripheralID::None); // TX alt. 3
                break;
            case Channels::Channel2_D25D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN26_bm, DigitalPinPeripheralID::None);
                break;
            case Channels::Channel3_D28D30:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN30_bm, DigitalPinPeripheralID::None); // TX alt. 1
                break;
            case Channels::Channel3_D28D31:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN31_bm, DigitalPinPeripheralID::None); // TX alt. 2
                break;
            case Channels::Channel4_D18D3:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN3_bm, DigitalPinPeripheralID::None); // TX alt. 1
                break;
            case Channels::Channel4_D18D19:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN19_bm, DigitalPinPeripheralID::None); // TX alt. 2
                break;
            default:
                return false;
        }
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool UART::EnableRX(bool enable)
{
    if (m_Port == nullptr) return false;

    if (enable)
    {
        switch(m_Channel)
        {
            case Channels::Channel0_A9A10:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN9_bm, DigitalPinPeripheralID::A);
                break;
            case Channels::Channel1_A5A4:
            case Channels::Channel1_A5A6:
            case Channels::Channel1_A5D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN5_bm, DigitalPinPeripheralID::C); // RX
                break;
            case Channels::Channel2_D25D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN25_bm, DigitalPinPeripheralID::C);
                break;
            case Channels::Channel3_D28D30:
            case Channels::Channel3_D28D31:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN28_bm, DigitalPinPeripheralID::A); // RX
                break;
            case Channels::Channel4_D18D3:
            case Channels::Channel4_D18D19:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN18_bm, DigitalPinPeripheralID::C); // RX
                break;
            default:
                return false;
        }
        m_Port->UART_CR = UART_CR_RXEN_Msk;
        return true;
    }
    else
    {
        switch(m_Channel)
        {
            case Channels::Channel0_A9A10:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN9_bm, DigitalPinPeripheralID::None);
                break;
            case Channels::Channel1_A5A4:
            case Channels::Channel1_A5A6:
            case Channels::Channel1_A5D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN5_bm, DigitalPinPeripheralID::None); // RX
                break;
            case Channels::Channel2_D25D26:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN25_bm, DigitalPinPeripheralID::None);
                break;
            case Channels::Channel3_D28D30:
            case Channels::Channel3_D28D31:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN28_bm, DigitalPinPeripheralID::None); // RX
                break;
            case Channels::Channel4_D18D3:
            case Channels::Channel4_D18D19:
                DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN18_bm, DigitalPinPeripheralID::None); // RX
                break;
            default:
                return false;
        }
        m_Port->UART_CR = UART_CR_RXDIS_Msk;
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t UART::Send(const void* buffer, int32_t length)
{
    if (length > 0)
    {
        for (int32_t i = 0; i < length; ++i)
        {
            while((m_Port->UART_SR & UART_SR_TXRDY_Msk) == 0);
            m_Port->UART_THR = UART_THR_TXCHR(((uint8_t*)buffer)[i]);
        }
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
/*
int32_t UART::Receive(void* buffer, int32_t length)
{
    if (length > 0)
    {
        while((m_Port->TWIHS_SR & (TWIHS_SR_TXRDY_Msk | TWIHS_SR_TXCOMP_Msk)) == 0);

        m_Port->TWIHS_CR = TWIHS_CR_SVDIS_Msk | TWIHS_CR_MSEN_Msk;
        m_Port->TWIHS_IADR = internalAddress;
        
        if (length > 1) {
            m_Port->TWIHS_CR = TWIHS_CR_START_Msk;
            } else {
            m_Port->TWIHS_CR = TWIHS_CR_START_Msk | TWIHS_CR_STOP_Msk;
        }
        for (int32_t i = 0; i < length - 1; ++i)
        {
            for (;;)
            {
                uint32_t status = m_Port->TWIHS_SR;
                if (status & TWIHS_SR_NACK_Msk) {
                    m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
                    return i;
                    } else if (status & TWIHS_SR_RXRDY_Msk) {
                    break;
                }
            }
            ((uint8_t*)buffer)[i] = (m_Port->TWIHS_RHR & TWIHS_RHR_RXDATA_Msk) >> TWIHS_RHR_RXDATA_Pos;
        }
        if (length > 1) {
            m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
        }
        for (;;)
        {
            uint32_t status = m_Port->TWIHS_SR;
            if (status & TWIHS_SR_NACK_Msk) {
                return length - 1;
                } else if (status & TWIHS_SR_RXRDY_Msk) {
                break;
            }
        }
        ((uint8_t*)buffer)[length-1] = (m_Port->TWIHS_RHR & TWIHS_RHR_RXDATA_Msk) >> TWIHS_RHR_RXDATA_Pos;
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t UART::ReceiveAsync(ResultCallback* callback, void* userObject, void* buffer, int32_t length)
{
    if (length <= 0) {
        return 0;
    }        
    while((m_Port->UART_SR & UART_SR_RXRDY_Msk) == 0);
    
    State_e expectedState = State_e::Idle;
    if (!m_State.compare_exchange_strong(expectedState, State_e::Reading))
    {
        return -1;
    }
    
    m_Callback   = callback;
    m_UserObject = userObject;
    m_Buffer     = buffer;
    m_Length     = length;
    m_CurPos     = 0;
    

    m_Port->UART_IER = UART_IER_RXRDY_Msk;

    m_Port->TWIHS_CR   = TWIHS_CR_SVDIS_Msk | TWIHS_CR_MSEN_Msk;
    m_Port->TWIHS_IADR = internalAddress;
        
    if (m_Length > 1) {
        m_Port->TWIHS_CR = TWIHS_CR_START_Msk;
    } else {
        m_Port->TWIHS_CR = TWIHS_CR_START_Msk | TWIHS_CR_STOP_Msk;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UART::Cancel()
{
    if (m_State != State_e::Idle)
    {
        m_Port->TWIHS_CR   = TWIHS_CR_SVDIS_Msk | TWIHS_CR_MSDIS_Msk | TWIHS_CR_STOP_Msk;
        volatile int32_t status = m_Port->TWIHS_SR;
        (void)status;

        m_State = State_e::Idle;
    }
}
*/
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UART::HandleIRQ()
{
/*    uint32_t status = m_Port->TWIHS_SR;
    
    uint8_t data = (m_Port->TWIHS_RHR & TWIHS_RHR_RXDATA_Msk) >> TWIHS_RHR_RXDATA_Pos;
    
    if (status & TWIHS_SR_NACK_Msk) {
        m_Port->TWIHS_IDR = ~0;
        m_State = State_e::Idle;
        m_Callback(m_UserObject, m_Buffer, m_CurPos);
    } else if (status & TWIHS_SR_RXRDY_Msk) {
        ((uint8_t*)m_Buffer)[m_CurPos++] = data;

        if (m_CurPos == (m_Length - 1)) {
            m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
        } else if (m_CurPos == m_Length) {
            m_Callback(m_UserObject, m_Buffer, m_CurPos);
        }
    }
    
    if (status & TWIHS_SR_TXCOMP_Msk)
    {
        m_Port->TWIHS_IDR = ~0;
        m_State = State_e::Idle;
    }        */
}    

} // namespace kernel
