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

#include <string.h>

#include "I2CDriver.h"
#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/HAL/SAME70System.h"
#include "DeviceControl/I2C.h"
#include "System/System.h"
#include "Kernel/Scheduler.h"
#include "Kernel/KSemaphore.h"
#include "Kernel/SpinTimer.h"

using namespace kernel;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

I2CDriver::I2CDriver(Channels channel) : m_Mutex("i2c_driver", 1, true), m_RequestSema("i2c_request", 0, false)
{
    m_State = State_e::Idle;

    switch(channel)
    {
        case Channels::Channel0:
            m_Port = TWIHS0;
            m_DataPin.Set(e_DigitalPortID_A, PIN3_bp);
            m_ClockPin.Set(e_DigitalPortID_A, PIN4_bp);
            m_PeripheralID = DigitalPinPeripheralID::A;
            SAME70System::EnablePeripheralClock(ID_TWIHS0);
            NVIC_ClearPendingIRQ(TWIHS0_IRQn);
            kernel::Kernel::RegisterIRQHandler(TWIHS0_IRQn, IRQCallback, this);
            break;
        case Channels::Channel1:
            m_Port = TWIHS1;
            m_DataPin.Set(e_DigitalPortID_B, PIN4_bp);
            m_ClockPin.Set(e_DigitalPortID_B, PIN5_bp);
            m_PeripheralID = DigitalPinPeripheralID::A;
            SAME70System::EnablePeripheralClock(ID_TWIHS1);
            NVIC_ClearPendingIRQ(TWIHS1_IRQn);
            kernel::Kernel::RegisterIRQHandler(TWIHS1_IRQn, IRQCallback, this);
            break;
        case Channels::Channel2:
            m_Port = TWIHS2;
            SAME70System::EnablePeripheralClock(ID_TWIHS2);
            m_DataPin.Set(e_DigitalPortID_D, PIN27_bp);
            m_ClockPin.Set(e_DigitalPortID_D, PIN28_bp);
            m_PeripheralID = DigitalPinPeripheralID::C;
            NVIC_ClearPendingIRQ(TWIHS2_IRQn);
            kernel::Kernel::RegisterIRQHandler(TWIHS2_IRQn, IRQCallback, this);
            break;
        default:
            break;
    }
    m_DataPin = true;
    m_ClockPin = true;
    
    m_DataPin.SetDirection(DigitalPinDirection_e::OpenCollector);
    m_ClockPin.SetDirection(DigitalPinDirection_e::OpenCollector);
    
    m_DataPin.SetPeripheralMux(m_PeripheralID);
    m_ClockPin.SetPeripheralMux(m_PeripheralID);
    
    m_Port->TWIHS_IDR = ~0UL; // Disable interrupts.

    SetBaudrate(400000);
    Reset();
    ClearBus();
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
    if (length == 0) {
        return 0;
    }
    CRITICAL_SCOPE(m_Mutex);

    m_State = State_e::Reading;
    m_RequestSema.SetCount(0);
    
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

    while((m_Port->TWIHS_SR & TWIHS_SR_TXCOMP_Msk) == 0);
    
    m_Port->TWIHS_SR; // Clear status register.
    m_Port->TWIHS_RHR; // Clear receive register.
    
    m_Port->TWIHS_MMR = 0;
    m_Port->TWIHS_MMR = CalcAddress(i2cfile->m_SlaveAddress, i2cfile->m_InternalAddressLength) | TWIHS_MMR_MREAD_Msk;
    m_Port->TWIHS_IADR = 0;
    m_Port->TWIHS_IADR = position;
    
    m_Buffer = buffer;
    m_Length = length;
    m_CurPos = 0;

    if (length > 1) {
        m_Port->TWIHS_CR = TWIHS_CR_START_Msk;
    } else {
        m_Port->TWIHS_CR = TWIHS_CR_START_Msk | TWIHS_CR_STOP_Msk;
    }
    m_Port->TWIHS_IER = TWIHS_IER_RXRDY_Msk | TWIHS_IER_NACK_Msk | TWIHS_IER_TXCOMP_Msk;

    if (!m_RequestSema.AcquireTimeout(bigtime_from_ms(std::max<uint32_t>(2, i2cfile->m_RelativeTimeout * length / m_Baudrate))))
    {
        printf("I2CDriver::Read() request failed: %s\n", strerror(get_last_error()));        
        m_Port->TWIHS_IDR = ~0UL; // Disable interrupts.
        Reset();
        ClearBus();
    }
    m_Port->TWIHS_IDR = ~0;
    m_State = State_e::Idle;
    if (m_CurPos < 0) {
        set_last_error(-m_CurPos);
        return -1;
    }
    return m_CurPos;    
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
        while((m_Port->TWIHS_SR & TWIHS_SR_TXCOMP_Msk) == 0);
        while((m_Port->TWIHS_SR & TWIHS_SR_TXRDY_Msk) == 0);
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
    bigtime_t startTime = get_system_time();
    while((m_Port->TWIHS_SR & TWIHS_SR_TXCOMP_Msk) == 0 && (get_system_time() - startTime) < 2000000);
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriver::Reset()
{
    m_Port->TWIHS_CR = TWIHS_CR_SWRST; // Reset.

    m_Port->TWIHS_FILTR = TWIHS_FILTR_FILT_Msk | TWIHS_FILTR_PADFEN_Msk | TWIHS_FILTR_THRES(7);

    // Set master mode
    m_Port->TWIHS_CR = TWIHS_CR_MSDIS;
    m_Port->TWIHS_CR = TWIHS_CR_SVDIS;
    m_Port->TWIHS_CR = TWIHS_CR_MSEN;
    SetBaudrate(m_Baudrate);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriver::ClearBus()
{
    m_DataPin.SetPeripheralMux(DigitalPinPeripheralID::None);
    m_ClockPin.SetPeripheralMux(DigitalPinPeripheralID::None);
    m_DataPin = true;
    m_ClockPin = true;
    for (int i = 0; i < 16; ++i)
    {
        // Clear at ~50kHz
        m_ClockPin = false;
        if (i == 15) {
            m_DataPin = false;
        }
        SpinTimer::SleepuS(10);
        m_ClockPin = true;
        SpinTimer::SleepuS(10);
    }
    m_DataPin = true; // Send STOP
    SpinTimer::SleepuS(10);

    m_DataPin.SetPeripheralMux(m_PeripheralID);
    m_ClockPin.SetPeripheralMux(m_PeripheralID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::SetBaudrate(uint32_t baudrate)
{
    uint32_t peripheralFrequency = SAME70System::GetFrequencyPeripheral();

    // 400k is the max speed allowed for master.
    if (baudrate > 400000) {
        set_last_error(EINVAL);
        return -1;
    }

    CRITICAL_SCOPE(m_Mutex);

    // Highest frequency that give a low-level time of at least 1.3uS
    static const uint32_t lowLevelTimeLimit = 384000; 
    
    if (baudrate > lowLevelTimeLimit) 
    {
        uint32_t lowDivider  = peripheralFrequency / (lowLevelTimeLimit * 2) - 3; // Low-level time fixed for 1.3us.
        uint32_t highDivider = peripheralFrequency / ((baudrate + (baudrate - lowLevelTimeLimit)) * 2) - 3; // Rest of the cycle time for high-level.

        uint32_t clkDivider = 0;

        // lowDivider / highDivider must fit in 8 bits
        while (lowDivider > 0xff || highDivider > 0xff)
        {
            clkDivider++;
            lowDivider  >>= 1;
            highDivider >>= 1;
        }
        // clkDivider must fit in 3 bits
        if (clkDivider > 7)
        {
            set_last_error(EINVAL);
            return -1;
        }            
        // Set clock waveform generator register
        m_Port->TWIHS_CWGR = TWIHS_CWGR_CLDIV(lowDivider) | TWIHS_CWGR_CHDIV(highDivider) | TWIHS_CWGR_CKDIV(clkDivider);
    }
    else
    {
        uint32_t highLowDivider = peripheralFrequency / (baudrate * 2) - 3;
        uint32_t clkDivider = 0;
        // highLowDivider must fit in 8 bits, clkDivider must fit in 3 bits
        while (highLowDivider > 0xff)
        {
            clkDivider++;
            highLowDivider >>= 1;
        }
        // clkDivider must fit in 3 bits
        if (clkDivider > 7)
        { 
            set_last_error(EINVAL);
            return -1;
        }            
        // Set clock waveform generator register
        m_Port->TWIHS_CWGR = TWIHS_CWGR_CLDIV(highLowDivider) | TWIHS_CWGR_CHDIV(highLowDivider) | TWIHS_CWGR_CKDIV(clkDivider);
    }
    m_Baudrate = baudrate;

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
    
    if (m_State == State_e::Reading)
    {
        if (status & TWIHS_SR_NACK_Msk)
        {
            m_Port->TWIHS_IDR = ~0;
            m_CurPos = -EIO;
            m_RequestSema.Release();
            return;
        }
        else if (status & TWIHS_SR_RXRDY_Msk)
        {
            if (m_CurPos == (m_Length - 2)) {
                m_Port->TWIHS_CR = TWIHS_CR_STOP_Msk;
            }
            assert(m_CurPos < m_Length);
            static_cast<uint8_t*>(m_Buffer)[m_CurPos++] = (m_Port->TWIHS_RHR & TWIHS_RHR_RXDATA_Msk) >> TWIHS_RHR_RXDATA_Pos;
        }
        if (status & TWIHS_SR_TXCOMP_Msk)
        {
            m_Port->TWIHS_IDR = ~0;
            m_RequestSema.Release();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t I2CDriver::CalcAddress(uint32_t slaveAddress, int addressLength)
{
    return TWIHS_MMR_DADR(slaveAddress) | TWIHS_MMR_IADRSZ(addressLength);
}
