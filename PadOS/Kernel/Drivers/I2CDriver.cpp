// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.02.2018 21:25:42

#include "sam.h"

#include "I2CDriver.h"
#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/HAL/SAME70System.h"
#include "DeviceControl/I2C.h"
#include "System/System.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

I2CDriver::I2CDriver(Channels channel) : m_Mutex("i2c_driver")
{
    m_State = State_e::Idle;
    m_Requests.resize(8);

    switch(channel)
    {
        case Channels::Channel0:
            m_Port = TWIHS0;
            SAME70System::EnablePeripheralClock(ID_TWIHS0);
            DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN3_bm, DigitalPinPeripheralID::A);
            DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN4_bm, DigitalPinPeripheralID::A);
            NVIC_ClearPendingIRQ(TWIHS0_IRQn);
            kernel::Kernel::RegisterIRQHandler(TWIHS0_IRQn, IRQCallback, this);
            break;
        case Channels::Channel1:
            m_Port = TWIHS1;
            SAME70System::EnablePeripheralClock(ID_TWIHS1);
            DigitalPort::SetPeripheralMux(e_DigitalPortID_B, PIN4_bm, DigitalPinPeripheralID::A);
            DigitalPort::SetPeripheralMux(e_DigitalPortID_B, PIN5_bm, DigitalPinPeripheralID::A);
            NVIC_ClearPendingIRQ(TWIHS1_IRQn);
            kernel::Kernel::RegisterIRQHandler(TWIHS1_IRQn, IRQCallback, this);
            break;
        case Channels::Channel2:
            m_Port = TWIHS2;
            SAME70System::EnablePeripheralClock(ID_TWIHS2);
            DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN27_bm, DigitalPinPeripheralID::C);
            DigitalPort::SetPeripheralMux(e_DigitalPortID_D, PIN28_bm, DigitalPinPeripheralID::C);
            NVIC_ClearPendingIRQ(TWIHS2_IRQn);
            kernel::Kernel::RegisterIRQHandler(TWIHS2_IRQn, IRQCallback, this);
            break;
        default:
            break;
    }
    SetBaudrate(400000);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

I2CDriver::~I2CDriver()
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileHandle> I2CDriver::Open( int flags)
{
    Ptr<I2CFile> file = ptr_new<I2CFile>();
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::DeviceControl( Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

    int* inArg  = (int*)inData;
    int* outArg = (int*)outData;

    switch(request)
    {
        case I2CIOCTL_SET_SLAVE_ADDRESS:
            if (inArg == nullptr || inDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            i2cfile->m_SlaveAddress = *inArg;
            break;
        case I2CIOCTL_GET_SLAVE_ADDRESS:
            if (outArg == nullptr || outDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            *outArg = i2cfile->m_SlaveAddress;
            break;
        case I2CIOCTL_SET_INTERNAL_ADDR_LEN:
            if (inArg == nullptr || inDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            i2cfile->m_InternalAddressLength = *inArg;
            break;
        case I2CIOCTL_GET_INTERNAL_ADDR_LEN:
            if (outArg == nullptr || outDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            *outArg = i2cfile->m_InternalAddressLength;
            break;
        case I2CIOCTL_SET_INTERNAL_ADDR:
            if (inArg == nullptr || inDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            i2cfile->m_InternalAddress = *inArg;
            break;
        case I2CIOCTL_GET_INTERNAL_ADDR:
            if (outArg == nullptr || outDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            *outArg = i2cfile->m_InternalAddress;
            break;
        case I2CIOCTL_SET_BAUDRATE:
            if (inArg == nullptr || inDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            SetBaudrate(*inArg);
            break;
        case I2CIOCTL_GET_BAUDRATE:
            if (outArg == nullptr || outDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            *outArg = GetBaudrate();
            break;
        default: set_last_error(EINVAL); return -1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t I2CDriver::Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);
    if (length > 0)
    {
        while((m_Port->TWIHS_SR & TWIHS_SR_TXCOMP_Msk) == 0);

        m_Port->TWIHS_CR = TWIHS_CR_SVDIS_Msk | TWIHS_CR_MSEN_Msk;
        m_Port->TWIHS_MMR = CalcAddress(i2cfile->m_SlaveAddress, i2cfile->m_InternalAddressLength) | TWIHS_MMR_MREAD_Msk;
        m_Port->TWIHS_IADR = position;
        
        //        if (length > 1) {
        m_Port->TWIHS_CR = TWIHS_CR_START_Msk;
        //            } else {
        //            m_Port->TWIHS_CR = TWIHS_CR_START_Msk | TWIHS_CR_STOP_Msk;
        //        }
        for (size_t i = 0; i < length - 1; ++i)
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
        //        if (length > 1) {
        m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
        //        }
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

ssize_t I2CDriver::Write(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);
    if (length > 0)
    {
        //        while((m_Port->TWIHS_SR & (TWIHS_SR_TXRDY_Msk | TWIHS_SR_TXCOMP_Msk)) != (TWIHS_SR_TXRDY_Msk | TWIHS_SR_TXCOMP_Msk));
        while((m_Port->TWIHS_SR & TWIHS_SR_TXCOMP_Msk) == 0);
        while((m_Port->TWIHS_SR & TWIHS_SR_TXRDY_Msk) == 0);
        //        m_Port->TWIHS_CR = TWIHS_CR_MSDIS_Msk;
        m_Port->TWIHS_CR = TWIHS_CR_SVDIS_Msk | TWIHS_CR_MSEN_Msk;
        m_Port->TWIHS_MMR = CalcAddress(i2cfile->m_SlaveAddress, i2cfile->m_InternalAddressLength);
        m_Port->TWIHS_IADR = position;

        for (size_t i = 0; i < length; ++i)
        {
            m_Port->TWIHS_THR = TWIHS_THR_TXDATA(((uint8_t*)buffer)[i]);
            if (i < (length - 1))
            {
                for (;;)
                {
                    uint32_t status = m_Port->TWIHS_SR;
                    if (status & TWIHS_SR_NACK_Msk) {
                        m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
                        return i;
                    } else if (status & TWIHS_SR_TXRDY_Msk) {
                        break;
                    }
                }
            }
        }
        m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
        for (;;)
        {
            uint32_t status = m_Port->TWIHS_SR;
            if (status & TWIHS_SR_NACK_Msk) {
                return length - 1;
            } else if (status & TWIHS_SR_TXCOMP_Msk) {
                break;
            }
        }
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::ReadAsync(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback)
{
    CRITICAL_SCOPE(m_Mutex);
//    DEBUG_DISABLE_IRQ();
    if (length <= 0) {
        return 0;
    }        
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

    bigtime_t startTime = get_system_time();
    for(;;)
    {
        State_e expectedState = State_e::Idle;
        if (!m_State.compare_exchange_strong(expectedState, State_e::Reading))
        {
            if (get_system_time() - startTime > 1000000) {
//                set_last_error(EBUSY);
                m_Port->TWIHS_CR   = TWIHS_CR_SVDIS_Msk | TWIHS_CR_MSDIS_Msk;
                m_Port->TWIHS_IDR = ~0;
                if (m_State == State_e::Reading)
                {
                    m_State = State_e::Idle;
//                    request.m_Callback(request.m_UserObject, request.m_Buffer, request.m_CurPos);
//                    requestFinished = true;
                }

                return -1;
            }
            continue;
            //set_last_error(EBUSY);
            //return -1;
        }
        break;
    }
    startTime = get_system_time();
    while((m_Port->TWIHS_SR & TWIHS_SR_TXCOMP_Msk) == 0 && (get_system_time() - startTime) < 200000);
    if (m_PendingRequests < m_Requests.size())
    {
        I2CWriteRequest& request = m_Requests[m_CurrentRequest + m_PendingRequests];
        m_PendingRequests++;

        request.m_Callback              = callback;
        request.m_UserObject            = userObject;
        request.m_Buffer                = buffer;
        request.m_Length                = length;
        request.m_CurPos                = 0;
        request.m_SlaveAddress          = i2cfile->m_SlaveAddress;
        request.m_InternalAddressLength = i2cfile->m_InternalAddressLength;
        request.m_InternalAddress       = position;

        if (m_PendingRequests == 1) {
            StartWriteRequest(request);
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriver::StartWriteRequest(const I2CWriteRequest& request)
{
    m_Port->TWIHS_CR   = TWIHS_CR_SVDIS_Msk | TWIHS_CR_MSEN_Msk;
    m_Port->TWIHS_MMR  = CalcAddress(request.m_SlaveAddress, request.m_InternalAddressLength) | TWIHS_MMR_MREAD_Msk;
    m_Port->TWIHS_IADR = request.m_InternalAddress;
        
    if (request.m_Length > 1) {
        m_Port->TWIHS_CR = TWIHS_CR_START_Msk;
    } else {
        m_Port->TWIHS_CR = TWIHS_CR_START_Msk | TWIHS_CR_STOP_Msk;
    }
    m_Port->TWIHS_IER = TWIHS_IER_RXRDY_Msk | TWIHS_IER_NACK_Msk | TWIHS_IER_TXCOMP_Msk;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::WriteAsync(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback)
{
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::CancelAsyncRequest(Ptr<KFileHandle> file, int handle)
{
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::SetBaudrate(uint32_t baudrate)
{
    CRITICAL_SCOPE(m_Mutex);
    m_Baudrate = baudrate;

    uint32_t peripheralFrequency = SAME70System::GetFrequencyPeripheral();
    m_Port->TWIHS_CWGR = TWIHS_CWGR_CLDIV(peripheralFrequency / baudrate / 2 - 3) | TWIHS_CWGR_CHDIV(peripheralFrequency / baudrate / 2 - 3) | TWIHS_CWGR_CKDIV(1);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::GetBaudrate() const
{
    return m_Baudrate;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriver::HandleIRQ()
{
    uint32_t status = m_Port->TWIHS_SR;
    
    if (m_PendingRequests > 0)
    {
        bool requestFinished = false;

        I2CWriteRequest& request = m_Requests[m_CurrentRequest];
        if (status & TWIHS_SR_NACK_Msk)
        {
            m_Port->TWIHS_IDR = ~0;
            m_State = State_e::Idle;
            request.m_Callback(request.m_UserObject, request.m_Buffer, request.m_CurPos);
            requestFinished = true;
        }
        else if (status & TWIHS_SR_RXRDY_Msk)
        {
            uint8_t data = (m_Port->TWIHS_RHR & TWIHS_RHR_RXDATA_Msk) >> TWIHS_RHR_RXDATA_Pos;
            if (request.m_CurPos < request.m_Length) {
                ((uint8_t*)request.m_Buffer)[request.m_CurPos++] = data;
            }
            if (request.m_CurPos == (request.m_Length - 1)) {
                m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
            } //else if (m_CurPos == m_Length) {
            //            m_State = State_e::Idle;
            //            m_Callback(m_UserObject, m_Buffer, m_CurPos);
            //        }
        }
        
        if (status & TWIHS_SR_TXCOMP_Msk)
        {
            m_Port->TWIHS_IDR = ~0;
            if (m_State == State_e::Reading)
            {
                m_State = State_e::Idle;
                request.m_Callback(request.m_UserObject, request.m_Buffer, request.m_CurPos);
                requestFinished = true;
            }
        }
        if (requestFinished)
        {
            m_CurrentRequest = (m_CurrentRequest + 1) % m_Requests.size();
            m_PendingRequests--;
            if (m_PendingRequests != 0) {
                StartWriteRequest(m_Requests[m_PendingRequests]);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t I2CDriver::CalcAddress(uint32_t slaveAddress, int len)
{
    int32_t addressLength = TWIHS_MMR_IADRSZ_NONE;
    switch(len)
    {
        case 1: addressLength = TWIHS_MMR_IADRSZ_1_BYTE; break;
        case 2: addressLength = TWIHS_MMR_IADRSZ_2_BYTE; break;
        case 3: addressLength = TWIHS_MMR_IADRSZ_3_BYTE; break;
    }
    return TWIHS_MMR_DADR(slaveAddress) | addressLength;
}
