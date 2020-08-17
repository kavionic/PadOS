// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.03.2020 21:59:38

 #include "System/Platform.h"

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "Kernel/Drivers/STM32/TLV493DDriver.h"
#include "Kernel/Drivers/STM32/I2CDriver.h"
#include "DeviceControl/I2C.h"
#include "System/System.h"
#include "Kernel/VFS/FileIO.h"
#include "Kernel/VFS/KFSVolume.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TLV493DDriver::TLV493DDriver() : Thread("tlv493d_driver"), m_Mutex("tlv493d_driver:mutex"), m_NewFrameCondition("tlv493d_driver_new_frame"), m_NewConfigCondition("tlv493d_driver_new_config")
{
	m_Config.frame_rate = 10;
	m_Config.temparature_scale = 1.0f;
	m_Config.temperature_offset = 0.0f;

    SetDeleteOnExit(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TLV493DDriver::~TLV493DDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TLV493DDriver::Setup(const char* devicePath, const char* i2cPath, const DigitalPin& powerPin)
{
	m_PowerPin = powerPin;
	m_I2CDevice = FileIO::Open(i2cPath, O_RDWR);

	ConfigChanged();

	if (m_I2CDevice >= 0)
    {
		m_PowerPin.SetDirection(DigitalPinDirection_e::Out);
		m_PowerPin.SetDriveStrength(DigitalPinDriveStrength_e::Low);
		m_PowerPin = true;
		m_PowerPin.SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);

		snooze_ms(5);

		Ptr<KINode> inode = ptr_new<KINode>(nullptr, nullptr, this, false);
		Kernel::RegisterDevice(devicePath, inode);
		
		Start(true);

        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TLV493DDriver::ResetSensor()
{
	for (;;)
	{
		printf("Resetting TLV493D.\n");

		m_PowerPin = false;
		snooze_ms(100);
		m_PowerPin = true;
		snooze_ms(100);

		I2CIOCTL_ClearBus(m_I2CDevice);

		I2CIOCTL_SetSlaveAddress(m_I2CDevice, 0);

		int errorCount = 0;

		uint8_t cfg = 0xff;

		// Write to slave-address zero to reset the sensor. After the address the sensor read the SDA
		// to set it's slave-address to one of two values. So writing 0x00 will make it read a '0'
		// and writing 0xff will make it read a '1'. The actual write operation will return an error
		// code since the data-byte is being transmitted while the sensor is resetting, and will not
		// be ack'd.

		FileIO::Write(m_I2CDevice, 0, &cfg, 1);
		I2CIOCTL_SetSlaveAddress(m_I2CDevice, m_DeviceAddress);

		errorCount = 0;
		while (FileIO::Read(m_I2CDevice, 0, &m_ReadRegisters, sizeof(m_ReadRegisters)) != sizeof(m_ReadRegisters) && errorCount++ < 5) {
			printf("Error: Failed to read initial TLV493D registers!\n");
			snooze_ms(10);
		}
		printf("TLV493DDriver: initial registers read.\n");

		m_WriteRegisters.Reserved1 = 0;
		m_WriteRegisters.Mode1 = m_ReadRegisters.DefaultCfg1;
		m_WriteRegisters.Reserved2 = m_ReadRegisters.DefaultCfg2;
		m_WriteRegisters.Mode2 = m_ReadRegisters.DefaultCfg3;

		m_WriteRegisters.Mode1 |= TLV493D_MODE1_LOW;
		m_WriteRegisters.Mode1 |= TLV493D_MODE1_FAST;
		m_WriteRegisters.Mode1 &= uint8_t(~TLV493D_MODE1_INT);
		UpdateParity();

		errorCount = 0;
		while (FileIO::Write(m_I2CDevice, 0, &m_WriteRegisters, sizeof(m_WriteRegisters)) != sizeof(m_WriteRegisters) && ++errorCount < 5) {
			snooze_ms(10);
		} if (errorCount == 5) {
			printf("Error: Failed to write TLV493D config registers!\n");
			snooze_s(1);
			continue;
		} else if (errorCount > 0) {
			printf("TLV493DDriver: config written (%d retries).\n", errorCount);
		} else {
			printf("TLV493DDriver: config written.\n");
		}

		FileIO::Read(m_I2CDevice, 0, &m_ReadRegisters, sizeof(m_ReadRegisters)); // Trigger first conversion
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int TLV493DDriver::Run()
{
	printf("TLV493DDriver: Resetting sensor.");

    I2CIOCTL_SetInternalAddrLen(m_I2CDevice, 0);

	ResetSensor();

	int result = FileIO::Read(m_I2CDevice, 0, &m_ReadRegisters, sizeof(m_ReadRegisters) - 3);
	if (result != sizeof(m_ReadRegisters) - 3) {
		printf("Error: Failed to read initial TLV493D registers! %d (%s)\n", result, strerror(errno));
	}

	int errorCount = 0;
	int sampleCount = 0;
	m_LastUpdateTime = get_system_time();
	for (;;)
	{
		CRITICAL_SCOPE(m_Mutex);
		m_NewConfigCondition.WaitDeadline(m_Mutex, m_LastUpdateTime + m_PeriodTime);

		if (errorCount >= 3) {
			printf("Error: TLV493D to many errors. Reset sensor.\n");
			ResetSensor();
			errorCount = 0;
		}

		TimeValMicros currentTime = get_system_time();
		TimeValMicros timeSinceLastUpdate = currentTime - m_LastUpdateTime;

		if (timeSinceLastUpdate < m_PeriodTime) continue;

		if (timeSinceLastUpdate < m_PeriodTime * 2LL) {
			m_LastUpdateTime += m_PeriodTime;
		} else {
			m_LastUpdateTime = currentTime;	// Catch-up if we are falling too far behind.
		}

		uint8_t	lastFrame = m_ReadRegisters.TempHFrmCh & TLV493D_FRAME;
		int result = FileIO::Read(m_I2CDevice, 0, &m_ReadRegisters, sizeof(m_ReadRegisters) - 3);
		if (result != sizeof(m_ReadRegisters) - 3) {
			errorCount++;
			printf("Error: Failed to read TLV493D registers! %d (%s)\n", result, strerror(errno));
			continue;
		}
		if ((m_ReadRegisters.TempHFrmCh & TLV493D_FRAME) == lastFrame) {
			errorCount++;
			if (errorCount > 1) {
				printf("Error: TLV493D frame counter didn't advance %d.\n", errorCount);
			}
			continue;
		}
		errorCount = 0;

		int16_t x = int16_t((m_ReadRegisters.BXH << 8) | ((m_ReadRegisters.BXYL >> 4) << 4)) / 16;
		int16_t y = int16_t((m_ReadRegisters.BYH << 8) | ((m_ReadRegisters.BXYL & 0xf) << 4)) / 16;
		int16_t z = int16_t((m_ReadRegisters.BZH << 8) | ((m_ReadRegisters.FlagsBZL & 0xf) << 4)) / 16;

		float fluxX = float(x) * TLV493D_LSB_FLUX;
		float fluxY = float(y) * TLV493D_LSB_FLUX;
		float fluxZ = float(z) * TLV493D_LSB_FLUX;

		if (m_Config.oversampling < 1)
		{
			m_CurrentValues.x = fluxX;
			m_CurrentValues.y = fluxY;
			m_CurrentValues.z = fluxZ;
			m_NewFrameCondition.Wakeup(0);
		}
		else
		{
			m_CurrentValues.x += (fluxX - m_CurrentValues.x) * m_AveragingScale;
			m_CurrentValues.y += (fluxY - m_CurrentValues.y) * m_AveragingScale;
			m_CurrentValues.z += (fluxZ - m_CurrentValues.z) * m_AveragingScale;
			sampleCount++;
			if (sampleCount >= m_Config.oversampling) {
				m_NewFrameCondition.Wakeup(0);
				sampleCount = 0;
			}
		}
	}
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TLV493DDriver::ConfigChanged()
{
	if (m_Config.frame_rate > 0)
	{
        m_PeriodTime = TimeValMicros::FromMicroseconds(TimeValMicros::TicksPerSecond / m_Config.frame_rate);
		if (m_PeriodTime < TLV493D_CONVERSION_TIME) m_PeriodTime = TLV493D_CONVERSION_TIME;
		I2CIOCTL_SetTimeout(m_I2CDevice, m_PeriodTime * 2LL);
	}
	else
	{
		m_PeriodTime = TimeValMicros::infinit;
		I2CIOCTL_SetTimeout(m_I2CDevice, TimeValMicros::FromSeconds(1LL));
	}
	m_AveragingScale = 1.0f / float(m_Config.oversampling);
	m_NewFrameCondition.Wakeup(0);

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TLV493DDriver::UpdateParity()
{
	uint8_t value = 0x00;
	// set parity bit to 1. Algorithm will calculate an even parity and replace this bit so parity becomes odd.

	m_WriteRegisters.Mode1 |= TLV493D_MODE1_PARITY;

	// Combine array to one byte first

	const uint8_t* data = reinterpret_cast<const uint8_t*>(&m_WriteRegisters);
	for(int i = 0; i < sizeof(m_WriteRegisters); ++i)
	{
		value ^= data[i];
	}
	// Combine all bits of this byte
	value = uint8_t(value ^ (value >> 1));
	value = uint8_t(value ^ (value >> 2));
	value = uint8_t(value ^ (value >> 4));

	// Parity is in the LSB of 'value'.
	if ((value & 0x01) == 0) {
		m_WriteRegisters.Mode1 &= uint8_t(~TLV493D_MODE1_PARITY);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int TLV493DDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    switch(request)
    {
		case TLV493DIOCTL_SET_CONFIG:
			if (inData == nullptr) {
				set_last_error(EINVAL);
				return -1;
			}
			memcpy(&m_Config, inData, std::min(sizeof(m_CurrentValues), inDataLength));
			ConfigChanged();
			return sizeof(tlv493d_config);
		case TLV493DIOCTL_GET_VALUES_SYNC:
			if (inData == nullptr || inDataLength < sizeof(tlv493d_data) || reinterpret_cast<const tlv493d_data*>(inData)->frame_number != m_CurrentValues.frame_number) {
				m_NewFrameCondition.Wait(m_Mutex);
			}
			// FALLTHROUGH
		case TLV493DIOCTL_GET_VALUES:
            if (outData == nullptr || outDataLength < sizeof(tlv493d_data)) {
                set_last_error(EINVAL);
                return -1;
            }
            memcpy(outData, &m_CurrentValues, outDataLength);
            return sizeof(tlv493d_data);
        default:
            return -1;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t TLV493DDriver::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    return 0;
}
