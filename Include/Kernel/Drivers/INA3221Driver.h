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
// Created: 15.02.2018 20:29:47

#pragma once

#include "DeviceControl/INA3221.h"
#include "Signals/SignalTarget.h"
#include "Kernel/VFS/KDeviceNode.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/KMutex.h"
#include "Threads/Looper.h"
#include "Threads/EventTimer.h"

namespace kernel
{

#define INA3221_CONFIG            0x00  // Configuration All-register reset, shunt and bus voltage ADC conversion times and averaging, operating mode. 01110001 00100111 7127 R/W
#define INA3221_SHUNT_VOLTAGE_1   0x01 // Channel-1 Shunt Voltage Averaged shunt voltage value. 00000000 00000000 0000 R
#define INA3221_BUS_VOLTAGE_1     0x02 // Channel-1 Bus Voltage Averaged bus voltage value. 00000000 00000000 0000 R
#define INA3221_SHUNT_VOLTAGE_2   0x03 // Channel-2 Shunt Voltage Averaged shunt voltage value. 00000000 00000000 0000 R
#define INA3221_BUS_VOLTAGE_2     0x04 // Channel-2 Bus Voltage Averaged bus voltage value. 00000000 00000000 0000 R
#define INA3221_SHUNT_VOLTAGE_3   0x05 // Channel-3 Shunt Voltage Averaged shunt voltage value. 00000000 00000000 0000 R
#define INA3221_BUS_VOLTAGE_3     0x06 // Channel-3 Bus Voltage Averaged bus voltage value. 00000000 00000000 0000 R
#define INA3221_CRITICAL_LIMIT_1  0x07 // Channel-1 Critical Alert Limit Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded. 01111111 11111000 7FF8 R/W
#define INA3221_WARNING_LIMIT_1   0x08 // Channel-1 Warning Alert Limit Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded. 01111111 11111000 7FF8 R/W
#define INA3221_CRITICAL_LIMIT_2  0x09 // Channel-2 Critical Alert Limit Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded. 01111111 11111000 7FF8 R/W
#define INA3221_WARNING_LIMIT_2   0x0A // Channel-2 Warning Alert Limit Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded. 01111111 11111000 7FF8 R/W
#define INA3221_CRITICAL_LIMIT_3  0x0B // Channel-3 Critical Alert Limit Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded. 01111111 11111000 7FF8 R/W
#define INA3221_WARNING_LIMIT_3   0x0C // Channel-3 Warning Alert Limit Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded. 01111111 11111000 7FF8 R/W
#define INA3221_SHUNT_VOLTAGE_SUM 0x0D // Shunt-Voltage Sum Contains the summed value of the each of the selected shunt voltage conversions. 00000000 00000000 0000 R
#define INA3221_BUS_VOLTAGE_SUM   0x0E // Shunt-Voltage Sum Limit Contains limit value to compare to the Shunt Voltage Sum register to determine if the corresponding limit has been exceeded. 01111111 11111110 7FFE R/W
#define INA3221_STATUS            0x0F // Mask/Enable Alert configuration, alert status indication, summation control and status. 00000000 00000010 0002 R/W
#define INA3221_PWR_VALID_UPPER   0x10 // Power-Valid Upper Limit Contains limit value to compare all bus voltage conversions to determine if the Power Valid level has been reached. 00100111 00010000 2710 R/W
#define INA3221_PWR_VALID_LOWER   0x11 // Power-Valid Lower Limit Contains limit value to compare all bus voltage conversions to determine if the any voltage rail has dropped below the Power Valid range. 00100011 00101000 2328 R/W
#define INA3221_MANUFACTURER_ID   0xFE // Manufacturer ID Contains unique manufacturer identification number. 01010100 01001001 5449 R
#define INA3221_DIE_ID            0xFF // Die ID Contains unique die identification number. 00110010 00100000 3220 R


// Bit definitions for the CONFIG register:
#define INA3221_CONFIG_MODE_bp    0
#define INA3221_CONFIG_VSH_CT_bp  3
#define INA3221_CONFIG_VBUS_CT_bp 6
#define INA3221_CONFIG_AVG_bp     9
#define INA3221_CONFIG_CH3_EN_bp  12
#define INA3221_CONFIG_CH2_EN_bp  13
#define INA3221_CONFIG_CH1_EN_bp  14
#define INA3221_CONFIG_RESET_bp   15

#define INA3221_CONFIG_MODE_bm   BIT16(INA3221_CONFIG_MODE_bp, 0x7)
#define INA3221_CONFIG_MODE(v)   (BIT16(INA3221_CONFIG_MODE_bp, v) & INA3221_CONFIG_MODE_bm)
#define   INA3221_CONFIG_MODE_POWER_DOWN           INA3221_CONFIG_MODE(0) // Power-down
#define   INA3221_CONFIG_MODE_SHUNT_SINGLESHOT     INA3221_CONFIG_MODE(1) // Shunt voltage, single-shot (triggered)
#define   INA3221_CONFIG_MODE_BUS_SINGLESHOT       INA3221_CONFIG_MODE(2) // Bus voltage, single-shot (triggered)
#define   INA3221_CONFIG_MODE_SHUNT_BUS_SINGLESHOT INA3221_CONFIG_MODE(3) // Shunt and bus, single-shot (triggered)
#define   INA3221_CONFIG_MODE_POWER_DOWN2          INA3221_CONFIG_MODE(4) // Power-down
#define   INA3221_CONFIG_MODE_SHUNT_CONTINUOS      INA3221_CONFIG_MODE(5) // Shunt voltage, continuous
#define   INA3221_CONFIG_MODE_BUS_CONTINUOS        INA3221_CONFIG_MODE(6) // Bus voltage, continuous
#define   INA3221_CONFIG_MODE_SHUNT_BUS_CONTINOUS  INA3221_CONFIG_MODE(7) // Shunt and bus, continuous (default)

#define INA3221_CONFIG_VSHCT_bm  BIT16(INA3221_CONFIG_VSH_CT_bp, 0x7)
#define INA3221_CONFIG_VSHCT(v)  (BIT16(INA3221_CONFIG_VSH_CT_bp, v) & INA3221_CONFIG_VSHCT_bm)

#define INA3221_CONFIG_VBUS_CT_bm BIT16(INA3221_CONFIG_VBUS_CT_bp, 0x7)
#define INA3221_CONFIG_VBUS_CT(v) (BIT16(INA3221_CONFIG_VBUS_CT_bp, v) & INA3221_CONFIG_VBUS_CT_bm)

#define INA3221_CONFIG_AVG_bm    BIT16(INA3221_CONFIG_AVG_bp, 0x7)
#define INA3221_CONFIG_AVG(v)    (BIT16(INA3221_CONFIG_AVG_bp, v) & INA3221_CONFIG_AVG_bm)
#define   INA3221_CONFIG_AVG_1    INA3221_CONFIG_AVG(0)
#define   INA3221_CONFIG_AVG_4    INA3221_CONFIG_AVG(1)
#define   INA3221_CONFIG_AVG_16   INA3221_CONFIG_AVG(2)
#define   INA3221_CONFIG_AVG_64   INA3221_CONFIG_AVG(3)
#define   INA3221_CONFIG_AVG_128  INA3221_CONFIG_AVG(4)
#define   INA3221_CONFIG_AVG_256  INA3221_CONFIG_AVG(5)
#define   INA3221_CONFIG_AVG_512  INA3221_CONFIG_AVG(6)
#define   INA3221_CONFIG_AVG_1024 INA3221_CONFIG_AVG(7)

#define INA3221_CONFIG_CH3_EN_bm   BIT16(INA3221_CONFIG_CH3_EN_bp, 1)
#define INA3221_CONFIG_CH2_EN_bm   BIT16(INA3221_CONFIG_CH2_EN_bp, 1)
#define INA3221_CONFIG_CH1_EN_bm   BIT16(INA3221_CONFIG_CH1_EN_bp, 1)
#define INA3221_CONFIG_RESET_bm    BIT16(INA3221_CONFIG_RESET_bp, 1)

// Bit definitions for the STATUS register:

#define INA3221_STATUS_CVRF_bp    0 // Conversion ready
#define INA3221_STATUS_TCF_bp     1 // Timing control alert flag
#define INA3221_STATUS_PVF_bp     2 // Power valid flag
#define INA3221_STATUS_WARNING_3  3 // Warning alert flag ch3
#define INA3221_STATUS_WARNING_2  4 // Warning alert flag ch2
#define INA3221_STATUS_WARNING_1  5 // Warning alert flag ch1
#define INA3221_STATUS_SF         6 // Summation alert flag
#define INA3221_STATUS_CRITICAL_3 7 // Critical alert flag ch3
#define INA3221_STATUS_CRITICAL_2 8 // Critical alert flag ch2
#define INA3221_STATUS_CRITICAL_1 9 // Critical alert flag ch1
#define INA3221_STATUS_CRITICAL_LATCH_EN 10 // Warning alert latch enable
#define INA3221_STATUS_WARNING_LATCH_EN  11 // Critical alert latch enable
#define INA3221_STATUS_SUMMATION_EN_3 12 // Shunt summation enable ch3
#define INA3221_STATUS_SUMMATION_EN_2 13 // Shunt summation enable ch2
#define INA3221_STATUS_SUMMATION_EN_1 14 // Shunt summation enable ch1

#define INA3221_SENSOR_IDX_1 0
#define INA3221_SENSOR_IDX_2 1
#define INA3221_SENSOR_IDX_3 2


class INA3221Driver : public PtrTarget, public os::Looper, public KFilesystemFileOps, public SignalTarget
{
public:
    INA3221Driver();
    ~INA3221Driver();

    bool Setup(const char* devicePath, const char* i2cPath);

    virtual int DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
//    virtual ssize_t Read(Ptr<KFileHandle> file, off64_t position, void* buffer, size_t length) override;

private:
    void SlotTick();

    enum class State_e
    {
        Idle,
        ReadingRegister,
        ProcessingRegister
    };


    KMutex  m_Mutex;
    State_e m_State = State_e::Idle;
    int     m_CurrentRegister = 0;

    bigtime_t m_UpdatePeriode  = 10000;
    bigtime_t m_LastUpdateTime = 0;
    bigtime_t m_ReadStartTime  = 0;
    bigtime_t m_ReadRetryCount = 0;

    int  m_I2CDevice;
    uint8_t m_DeviceAddress = 0x40;

    double CalculateCurrent(int sensor, int16_t value) const { return double(value / 8)*40e-6/m_ShuntValues[sensor]; }
    double CalculateVoltage(int16_t value) const { return double(value / 8) * 8.0e-3; }

    os::EventTimer m_Timer;

    INA3221Values m_CurrentValues = { {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };
    double m_ShuntValues[INA3221_SENSOR_COUNT] = {1.0, 1.0, 1.0};


    uint8_t m_RegisterBuffer[2];

    INA3221Driver( const INA3221Driver &c );
    INA3221Driver& operator=( const INA3221Driver &c );
};

} // namespace
