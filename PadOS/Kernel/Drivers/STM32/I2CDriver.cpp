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

#include "Platform.h"

#include <string.h>
#include <cmath>

#include "I2CDriver.h"
#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/HAL/SAME70System.h"
#include "System/System.h"
#include "Kernel/Scheduler.h"
#include "Kernel/KSemaphore.h"
#include "Kernel/SpinTimer.h"
#include "Kernel/VFS/KFSVolume.h"

using namespace kernel;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

I2CDriverINode::I2CDriverINode(KFilesystemFileOps* fileOps
								, I2C_TypeDef* port
								, const PinMuxTarget& clockPinCfg
								, const PinMuxTarget& dataPinCfg
								, IRQn_Type eventIRQ
								, IRQn_Type errorIRQ
								, uint32_t clockFrequency
								, double fallTime
								, double riseTime)
	: KINode(nullptr, nullptr, fileOps, false)
	, m_Mutex("I2CDriverINode")
	, m_RequestCondition("I2CDriverINodeRequest")
	, m_Port(port)
	, m_ClockPin(clockPinCfg)
	, m_DataPin(dataPinCfg)
	, m_ClockFrequency(clockFrequency)
	, m_FallTime(fallTime)
	, m_RiseTime(riseTime)
{
    m_State = State_e::Idle;

	DigitalPin clockPin(m_ClockPin.PINID);
	DigitalPin dataPin(m_DataPin.PINID);

	clockPin.SetDirection(DigitalPinDirection_e::OpenCollector);
	dataPin.SetDirection(DigitalPinDirection_e::OpenCollector);

	clockPin.SetPeripheralMux(m_ClockPin.MUX);
	dataPin.SetPeripheralMux(m_DataPin.MUX);

	clockPin.SetPullMode(PinPullMode_e::Up);
	dataPin.SetPullMode(PinPullMode_e::Up);

	kernel::Kernel::RegisterIRQHandler(eventIRQ, IRQCallbackEvent, this);
	kernel::Kernel::RegisterIRQHandler(errorIRQ, IRQCallbackError, this);

	SetSpeed(I2CSpeed::Fast);

	ResetPeripheral();
    ClearBus();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

I2CDriverINode::~I2CDriverINode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> I2CDriverINode::Open( int flags)
{
    Ptr<I2CFile> file = ptr_new<I2CFile>();
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriverINode::DeviceControl( Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
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
//            SetBaudrate(*inArg);
            break;
        case I2CIOCTL_GET_BAUDRATE:
            if (outArg == nullptr || outDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
            *outArg = GetBaudrate();
            break;
		case I2CIOCTL_SET_TIMEOUT:
			if (inData == nullptr || inDataLength != sizeof(bigtime_t)) { set_last_error(EINVAL); return -1; }
			i2cfile->m_Timeout = *reinterpret_cast<const bigtime_t*>(inData);
			break;
		case I2CIOCTL_GET_TIMEOUT:
			if (outData == nullptr || outDataLength != sizeof(bigtime_t)) { set_last_error(EINVAL); return -1; }
			*reinterpret_cast<bigtime_t*>(outData) = i2cfile->m_Timeout;
			break;
		case I2CIOCTL_CLEAR_BUS:
			ClearBus();
			break;
		default: set_last_error(EINVAL); return -1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t I2CDriverINode::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    if (length == 0) {
        return 0;
    }
    CRITICAL_SCOPE(m_Mutex);
	
    m_State = State_e::Reading;
    
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

	m_Buffer = reinterpret_cast<uint8_t*>(buffer);
	m_Length = length;
	m_CurPos = 0;

	m_Port->CR2 = ((i2cfile->m_SlaveAddress << I2C_CR2_SADD_Pos) & I2C_CR2_SADD_Msk)
				| I2C_CR2_RD_WRN
				| ((i2cfile->m_SlaveAddressLength == I2C_ADDR_LEN_10BIT) ? I2C_CR2_ADD10 : 0)
				| ((m_Length <= (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos)) ? (m_Length << I2C_CR2_NBYTES_Pos) : I2C_CR2_NBYTES_Msk)
				| (m_Length > (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos) ? I2C_CR2_RELOAD : 0)
				| I2C_CR2_AUTOEND;

	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		const uint32_t interruptFlags = I2C_CR1_RXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;
		m_Port->ICR = I2C_ICR_ADDRCF | I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF | I2C_ICR_PECCF | I2C_ICR_TIMOUTCF | I2C_ICR_ALERTCF;
		m_Port->CR1 |= interruptFlags;

		m_Port->CR2 |= I2C_CR2_START;


		if (!m_RequestCondition.IRQWaitTimeout(i2cfile->m_Timeout))
		{
			ResetPeripheral();
			m_CurPos = -ETIME;
		}
		m_Port->CR1 &= ~interruptFlags;
	} CRITICAL_END;
	m_State = State_e::Idle;
    if (m_CurPos < 0)
    {
        set_last_error(-m_CurPos);
		printf("I2CDriver::Read() request failed: %s\n", strerror(get_last_error()));
        return -1;
    }
    return m_CurPos;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t I2CDriverINode::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
	if (length == 0) {
		return 0;
	}
	CRITICAL_SCOPE(m_Mutex);

	m_State = State_e::Writing;

	Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

	m_Buffer = reinterpret_cast<uint8_t*>(const_cast<void*>(buffer));
	m_Length = length;
	m_CurPos = 0;


	m_Port->CR2 = ((i2cfile->m_SlaveAddress << I2C_CR2_SADD_Pos) & I2C_CR2_SADD_Msk)
				| ((i2cfile->m_SlaveAddressLength == I2C_ADDR_LEN_10BIT) ? I2C_CR2_ADD10 : 0)
				| ((m_Length <= (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos)) ? (m_Length << I2C_CR2_NBYTES_Pos) : I2C_CR2_NBYTES_Msk)
				| (m_Length > (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos) ? I2C_CR2_RELOAD : 0)
				| I2C_CR2_AUTOEND;

	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		const uint32_t interruptFlags = I2C_CR1_TXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;

		m_Port->ICR = I2C_ICR_ADDRCF | I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF | I2C_ICR_PECCF | I2C_ICR_TIMOUTCF | I2C_ICR_ALERTCF;
		m_Port->CR1 |= interruptFlags;

		m_Port->CR2 |= I2C_CR2_START;

		if (m_Port->ISR & I2C_ISR_TXIS)
		{
			m_Port->TXDR = m_Buffer[m_CurPos++];
		}

		if (!m_RequestCondition.IRQWaitTimeout(i2cfile->m_Timeout))
		{
			ResetPeripheral();
			m_CurPos = -ETIME;
		}
		m_Port->CR1 &= interruptFlags;
	} CRITICAL_END;
	m_State = State_e::Idle;
	if (m_CurPos < 0)
	{
		set_last_error(-m_CurPos);
		printf("I2CDriver::Write() request failed: %s\n", strerror(get_last_error()));
		return -1;
	}
	return m_CurPos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::ResetPeripheral()
{
	m_Port->CR1 = 0;
	m_Port->CR1 = I2C_CR1_PE
				| ((m_DigitalFilterCount << I2C_CR1_DNF_Pos) & I2C_CR1_DNF_Msk)
				| ((m_AnalogFilterEnabled) ? 0 : I2C_CR1_ANFOFF);

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::ClearBus()
{
	DigitalPin clockPin(m_ClockPin.PINID);
	DigitalPin dataPin(m_DataPin.PINID);


	clockPin.SetPeripheralMux(DigitalPinPeripheralID::None);
	dataPin.SetPeripheralMux(DigitalPinPeripheralID::None);

	clockPin.SetDirection(DigitalPinDirection_e::OpenCollector);
	dataPin.SetDirection(DigitalPinDirection_e::OpenCollector);

	dataPin = true;
	clockPin = true;
    for (int i = 0; i < 16; ++i)
    {
        // Clear at ~50kHz
		clockPin = false;
        if (i == 15) {
			dataPin = false;
        }
        SpinTimer::SleepuS(10);
		clockPin = true;
        SpinTimer::SleepuS(10);
    }
	dataPin = true; // Send STOP
    SpinTimer::SleepuS(10);

	clockPin.SetPeripheralMux(m_ClockPin.MUX);
	dataPin.SetPeripheralMux(m_DataPin.MUX);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriverINode::SetSpeed(I2CSpeed speed)
{
	int speedIndex = int(speed);
    if (speedIndex < 0 || speedIndex > int(I2CSpeed::FastPlus)) {
        set_last_error(EINVAL);
        return -1;
    }

    CRITICAL_SCOPE(m_Mutex);

	const I2CSpec& spec = I2CSpecs[speedIndex];
	
	constexpr int32_t PRESC_MAX		= I2C_TIMINGR_PRESC_Msk >> I2C_TIMINGR_PRESC_Pos;
	constexpr int32_t SCLL_MAX		= I2C_TIMINGR_SCLL_Msk >> I2C_TIMINGR_SCLL_Pos;
	constexpr int32_t SCLH_MAX		= I2C_TIMINGR_SCLH_Msk >> I2C_TIMINGR_SCLH_Pos;
	constexpr int32_t SDADEL_MAX	= I2C_TIMINGR_SDADEL_Msk >> I2C_TIMINGR_SDADEL_Pos;
	constexpr int32_t SCLDEL_MAX	= I2C_TIMINGR_SCLDEL_Msk >> I2C_TIMINGR_SCLDEL_Pos;

	uint32_t minError = std::numeric_limits<uint32_t>::max();

	int32_t bestPRESC  = 0;
	int32_t bestSDADEL = 0;
	int32_t bestSCLDEL = 0;
	int32_t bestSCLL   = 0;
	int32_t bestSCLH   = 0;

	for (int prescale = 0; prescale <= PRESC_MAX; ++prescale)
	{
		uint32_t scaledClock = m_ClockFrequency / (prescale + 1);
		double clockCycleTime = double(1) / double(scaledClock);

		double filterDelayMin = m_AnalogFilterEnabled ? 50.0e-9 : 0.0;
		double filterDelayMax = 260.0e-9;

		filterDelayMin += double(m_DigitalFilterCount + 2) * clockCycleTime;
		filterDelayMax += double(m_DigitalFilterCount + 3) * clockCycleTime;

		double tsync1 = filterDelayMin + m_FallTime;
		double tsync2 = filterDelayMin + m_RiseTime;

		int32_t SCLL = std::max(0, int(ceil((spec.ClockLowMin - tsync1) / clockCycleTime)) - 1);
		if (SCLL > SCLL_MAX) continue;

		int32_t SCLH = std::max(0, int(ceil((spec.ClockHighMin - tsync2) / clockCycleTime)) - 1);
		if (SCLH > SCLH_MAX) continue;

		int32_t totalCycleTime = std::max(0, int(ceil((1.0 / double(spec.Baudrate) - tsync1 - tsync2) / clockCycleTime)) - 2);

		int32_t extraTime = totalCycleTime - SCLL - SCLH;
		int32_t extraTimeH = extraTime / 2;
		int32_t extraTimeL = extraTime - extraTimeH;

		SCLL += extraTimeL;
		if (SCLL > SCLL_MAX) {
			extraTimeH += SCLL - SCLL_MAX;
			SCLL = SCLL_MAX;
		}
		SCLH += extraTimeH;
		if (SCLH > SCLH_MAX) {
			SCLL += SCLH - SCLH_MAX;
			SCLH = SCLH_MAX;
		}
		if (SCLL > SCLL_MAX) {
			continue;
		}

		double holdTimeMin = spec.DataHoldTimeMin + m_FallTime - filterDelayMin;
		double validTimeMax = spec.DataValidTimeMax - m_RiseTime - filterDelayMax;

		int32_t validClocksMax = std::max(0, int(ceil(validTimeMax / clockCycleTime)) - 1);
		if (validClocksMax > SDADEL_MAX) validClocksMax = SDADEL_MAX;

		int32_t SDADEL = std::max(0, int(ceil(holdTimeMin / clockCycleTime)) - 1);
		if (SDADEL > validClocksMax) {
			continue;
		}

		int32_t SCLDEL = std::max(0, int(ceil(spec.DataSetupTimeMin / clockCycleTime)) - 1);
		if (SDADEL > SCLDEL_MAX) {
			continue;
		}
		double clockPeriod = double(SCLL + SCLH + 2) * clockCycleTime + tsync1 + tsync2;

		uint32_t rate = 1.0 / clockPeriod;
		if (rate < spec.BaudrateMin || rate > spec.Baudrate) {
			continue;
		}

		uint32_t error = abs(rate - spec.Baudrate);
		if (error < minError)
		{
			minError = error;

			bestPRESC = prescale;
			bestSDADEL = SDADEL;
			bestSCLDEL = SCLDEL;
			bestSCLL = SCLL;
			bestSCLH = SCLH;
			m_Baudrate = rate;
		}
	}
	if (minError == std::numeric_limits<uint32_t>::max()) {
		printf("ERROR: I2C failed to set baudrate!\n");
		return -1;
	}
//	m_Port->TIMINGR = 0x00b03fdb;
	m_Port->TIMINGR = (bestPRESC  << I2C_TIMINGR_PRESC_Pos)
					| (bestSDADEL << I2C_TIMINGR_SDADEL_Pos)
					| (bestSCLDEL << I2C_TIMINGR_SCLDEL_Pos)
					| (bestSCLL   << I2C_TIMINGR_SCLL_Pos)
					| (bestSCLH   << I2C_TIMINGR_SCLH_Pos);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriverINode::GetBaudrate() const
{
    return m_Baudrate;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::HandleEventIRQ()
{
	uint32_t status = m_Port->ISR;

	if (status & I2C_ISR_NACKF)
	{
		m_CurPos = -ECONNREFUSED;
		m_Port->ICR = I2C_ICR_NACKCF;
		m_RequestCondition.Wakeup();
		return;
	}

	if (m_State == State_e::Reading && status & I2C_ISR_RXNE)
	{
		m_Buffer[m_CurPos++] = m_Port->RXDR;
	}
	if (m_State == State_e::Writing && status & I2C_ISR_TXIS)
	{
		m_Port->TXDR = m_Buffer[m_CurPos++];
	}

	if (status & I2C_ISR_TCR) // Transfer complete reload.
	{
		uint32_t CR2 = m_Port->CR2;
		CR2 &= ~(I2C_CR2_NBYTES_Msk | I2C_CR2_RELOAD);

		int32_t remainingLength = m_Length - m_CurPos;
		if (remainingLength <= (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos)) {
			CR2 |= remainingLength << I2C_CR2_NBYTES_Pos;
		} else {
			CR2 |= I2C_CR2_NBYTES_Msk | I2C_CR2_RELOAD;
		}
		m_Port->CR2 = CR2;
	}
	if (status & I2C_ISR_STOPF) // Transfer complete.
	{
		m_Port->ICR = I2C_ICR_STOPCF;
		m_RequestCondition.Wakeup();
	}
	if (status & I2C_ISR_TC) // Transfer complete.
	{
		m_RequestCondition.Wakeup();
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::HandleErrorIRQ()
{
	m_Port->CR1 &= ~I2C_CR1_ERRIE;
	m_CurPos = -EIO;
	m_RequestCondition.Wakeup();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriver::Setup(const char* devicePath, I2C_TypeDef* port, const PinMuxTarget& clockPin, const PinMuxTarget& dataPin, IRQn_Type eventIRQ, IRQn_Type errorIRQ, uint32_t clockFrequency, double fallTime, double riseTime)
{
    Ptr<I2CDriverINode> node = ptr_new<I2CDriverINode>(this, port, clockPin, dataPin, eventIRQ, errorIRQ, clockFrequency, fallTime, riseTime);
    Kernel::RegisterDevice(devicePath, node);    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> I2CDriver::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> inode, int flags)
{
    return ptr_static_cast<I2CDriverINode>(inode)->Open(flags);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t I2CDriver::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    return ptr_static_cast<I2CDriverINode>(file->GetINode())->Read(file, position, buffer, length);    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t I2CDriver::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    return ptr_static_cast<I2CDriverINode>(file->GetINode())->Write(file, position, buffer, length);    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    return ptr_static_cast<I2CDriverINode>(file->GetINode())->DeviceControl(file, request, inData, inDataLength, outData, outDataLength);
}
