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

#include "sam.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "INA3221Driver.h"
#include "I2CDriver.h"
#include "DeviceControl/I2C.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

INA3221Driver::INA3221Driver(const char* i2cPath) : Looper("ina3221_driver", 10), m_Mutex("ina3221_driver"), m_Timer(bigtime_from_ms(10))
{
    SetDeleteOnExit(false);
    Initialize(i2cPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

INA3221Driver::~INA3221Driver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool INA3221Driver::Initialize(const char* i2cPath)
{
    m_I2CDevice = Kernel::OpenFile(i2cPath, O_RDWR);

    if (m_I2CDevice >= 0)
    {
        I2CIOCTL_SetSlaveAddress(m_I2CDevice, m_DeviceAddress);
        I2CIOCTL_SetInternalAddrLen(m_I2CDevice, 1);

        m_Timer.SignalTrigged.Connect(this, &INA3221Driver::SlotTick);
        AddTimer(&m_Timer);
        Start(true);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void INA3221Driver::SlotTick()
{
    CRITICAL_SCOPE(m_Mutex);
    bigtime_t time = Kernel::GetTime();

    if (m_State == State_e::Idle)
    {
        m_LastUpdateTime = time;
        m_State = State_e::ReadingRegister;
        m_CurrentRegister = INA3221_SHUNT_VOLTAGE_1;
        m_ReadStartTime = time;
        m_ReadRetryCount = 0;
        if (Kernel::Read(m_I2CDevice, m_CurrentRegister, m_RegisterBuffer, sizeof(m_RegisterBuffer)) == sizeof(m_RegisterBuffer)) {
            m_State = State_e::ProcessingRegister;
        }                
        m_Timer.Set(1000);
    }
    else if (m_State == State_e::ReadingRegister)
    {
        if (time - m_ReadStartTime >= 50000)
        {
            m_ReadRetryCount++;
            if (m_ReadRetryCount > 10)
            {
                printf("ERROR: INA3221Driver request timed out\n");
                m_LastUpdateTime = time;
                m_State = State_e::Idle;
                m_Timer.Set(m_UpdatePeriode);
            }
            else
            {
                m_ReadStartTime = time;
                if (Kernel::Read(m_I2CDevice, m_CurrentRegister, m_RegisterBuffer, sizeof(m_RegisterBuffer)) == sizeof(m_RegisterBuffer)) {
                    m_State = State_e::ProcessingRegister;
                }
                m_Timer.Set(2000);
            }
        }
    }
    else if (m_State == State_e::ProcessingRegister)
    {
        int16_t value = int16_t(uint16_t(m_RegisterBuffer[0]) << 8 | m_RegisterBuffer[1]);
        m_State = State_e::ReadingRegister;
        switch(m_CurrentRegister)
        {
            case INA3221_SHUNT_VOLTAGE_1:
                m_CurrentValues.Currents[0] += (CalculateCurrent(0, value) - m_CurrentValues.Currents[0]) * 0.01;
                m_CurrentRegister = INA3221_BUS_VOLTAGE_1;
                break;
            case INA3221_BUS_VOLTAGE_1:
                m_CurrentValues.Voltages[0] += (CalculateVoltage(value) - m_CurrentValues.Voltages[0]) * 0.01;
                m_CurrentRegister = INA3221_SHUNT_VOLTAGE_2;
                break;
            case INA3221_SHUNT_VOLTAGE_2:
                m_CurrentValues.Currents[1] += (CalculateCurrent(1, value) - m_CurrentValues.Currents[1]) * 0.01;
                m_CurrentRegister = INA3221_BUS_VOLTAGE_2;
                break;
            case INA3221_BUS_VOLTAGE_2:
                m_CurrentValues.Voltages[1] += (CalculateVoltage(value) - m_CurrentValues.Voltages[1]) * 0.01;
                m_CurrentRegister = INA3221_SHUNT_VOLTAGE_3;
                break;
            case INA3221_SHUNT_VOLTAGE_3:
                m_CurrentValues.Currents[2] +=  (CalculateCurrent(2, value) - m_CurrentValues.Currents[2]) * 0.01;
                m_CurrentRegister = INA3221_BUS_VOLTAGE_3;
                break;
            case INA3221_BUS_VOLTAGE_3:
                m_CurrentValues.Voltages[2] += (CalculateVoltage(value) - m_CurrentValues.Voltages[2]) * 0.01;
                m_State = State_e::Idle;
                m_Timer.Set(m_UpdatePeriode - (time - m_LastUpdateTime));
                break;
        }
        if (m_State == State_e::ReadingRegister)
        {
            m_ReadStartTime = time;
            m_ReadRetryCount = 0;
            if (Kernel::Read(m_I2CDevice, m_CurrentRegister, m_RegisterBuffer, sizeof(m_RegisterBuffer)) == sizeof(m_RegisterBuffer)) {
                m_State = State_e::ProcessingRegister;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int INA3221Driver::DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    switch(request)
    {
        case INA3221_CMD_GET_MEASUREMENTS:
            if (outData == nullptr || outDataLength < sizeof(INA3221Values)) {
                set_last_error(EINVAL);
                return -1;
            }
            memcpy(outData, &m_CurrentValues, outDataLength);
            return sizeof(INA3221Values);
        case INA3221_CMD_SET_SHUNT_CFG:
        {
            if (inData == nullptr || inDataLength < sizeof(INA3221ShuntConfig)) {
                set_last_error(EINVAL);
                return -1;
            }
            const INA3221ShuntConfig* config = static_cast<const INA3221ShuntConfig*>(inData);
            for (int i = 0; i < INA3221_SENSOR_COUNT; ++i) {
                m_ShuntValues[i] = config->ShuntValues[i];
            }
            return sizeof(INA3221ShuntConfig);
        }
        case INA3221_CMD_GET_SHUNT_CFG:
        {
            if (outData == nullptr || outDataLength < sizeof(INA3221ShuntConfig)) {
                set_last_error(EINVAL);
                return -1;
            }
            INA3221ShuntConfig* config = static_cast<INA3221ShuntConfig*>(outData);
            for (int i = 0; i < INA3221_SENSOR_COUNT; ++i) {
                config->ShuntValues[i] = m_ShuntValues[i];
            }
            return sizeof(INA3221ShuntConfig);
        }
        default:
            return -1;
    }
}
