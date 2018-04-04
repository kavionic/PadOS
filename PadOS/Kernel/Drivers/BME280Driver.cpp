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
// Created: 16.02.2018 23:40:30

 #include "sam.h"

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "BME280Driver.h"

#include "I2CDriver.h"
#include "DeviceControl/I2C.h"
#include "System/System.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

BME280Driver::BME280Driver(const char* i2cPath) : Thread("bme280_driver"), m_Mutex("bme280_driver")
{
    SetDeleteOnExit(false);
    Initialize(i2cPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

BME280Driver::~BME280Driver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool BME280Driver::Initialize(const char* i2cPath)
{
    m_I2CDevice = Kernel::OpenFile(i2cPath, O_RDWR);

    if (m_I2CDevice >= 0)
    {
        I2CIOCTL_SetSlaveAddress(m_I2CDevice, m_DeviceAddress);
        I2CIOCTL_SetInternalAddrLen(m_I2CDevice, 1);

        Start(true);

        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int BME280Driver::Run()
{
    for (;;)
    {
        SlotTick();
        snooze(bigtime_from_ms(10));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void BME280Driver::SlotTick()
{
    CRITICAL_SCOPE(m_Mutex);
    bigtime_t time = get_system_time();
    if (m_State == State_e::Idle)
    {
        m_LastUpdateTime = time;
        m_State = State_e::ReadingRegister;
        RequestData(BME280_DATA_ADDR, BME280_P_T_H_DATA_LEN);
    }
    else if (m_State == State_e::ReadingRegister && m_BytesReceived == m_BytesToReceive)
    {
        // Variables to store the sensor data
        uint32_t data_xlsb;
        uint32_t data_lsb;
        uint32_t data_msb;

        // Store the parsed register values for pressure data
        data_msb = (uint32_t)m_ReceiveBuffer[0] << 12;
        data_lsb = (uint32_t)m_ReceiveBuffer[1] << 4;
        data_xlsb = (uint32_t)m_ReceiveBuffer[2] >> 4;
        uint32_t pressure = data_msb | data_lsb | data_xlsb;

        // Store the parsed register values for temperature data
        data_msb = (uint32_t)m_ReceiveBuffer[3] << 12;
        data_lsb = (uint32_t)m_ReceiveBuffer[4] << 4;
        data_xlsb = (uint32_t)m_ReceiveBuffer[5] >> 4;
        uint32_t temperature = data_msb | data_lsb | data_xlsb;

        // Store the parsed register values for temperature data
        data_lsb = (uint32_t)m_ReceiveBuffer[6] << 8;
        data_msb = (uint32_t)m_ReceiveBuffer[7];
        uint32_t humidity = data_msb | data_lsb;

        m_CurrentValues.m_Temperature = CompensateTemperature(temperature);
        m_CurrentValues.m_Pressure    = CompensatePressure(pressure);
        m_CurrentValues.m_Humidity    = CompensateHumidity(humidity);

//        printf("Temp: %.3f, Pres: %.3f, Hum: %.3f\n", m_CurrentValues.m_Temperature, m_CurrentValues.m_Pressure / 100.0, m_CurrentValues.m_Humidity);
        m_BytesToReceive = 0;
        //m_Timer.Start(m_UpdatePeriode - (time - m_LastUpdateTime));
        m_State = State_e::Idle;
    }
    else if (m_State == State_e::Initializing)
    {
        if (m_I2CDevice != -1)
        {
            uint8_t chipID = 0;

            if (Kernel::Read(m_I2CDevice, BME280_CHIP_ID_ADDR, &chipID, 1) != 1) {
                printf("ERROR: BME280 failed to read chip ID\n");
            } else {
                printf("BME280 ChipID: %02x\n", chipID);
            }
            uint8_t humCfg = 0;
            if (Kernel::Read(m_I2CDevice, BME280_CTRL_HUM_ADDR, &humCfg, 1) != 1) {
                printf("ERROR: BME280 failed to read humidity config\n");
            }

            humCfg = (humCfg & ~BME280_CTRL_HUM_OVRSMPL_bm) | BME280_CTRL_HUM_OVRSMPL_4;
            uint8_t measCfg = BME280_CTRL_MEAS_SENSOR_MODE_NORMAL | BME280_CTRL_MEAS_OVRSMPL_PRESS_4 | BME280_CTRL_MEAS_OVRSMPL_TEMP_4;

            if (Kernel::Write(m_I2CDevice, BME280_CTRL_HUM_ADDR, &humCfg, 1) != 1) {
                printf("ERROR: BME280 failed to write humidity config\n");
            }
            if (Kernel::Write(m_I2CDevice, BME280_CTRL_MEAS_ADDR, &measCfg, 1) != 1) {
                printf("ERROR: BME280 failed to write measurement config\n");
            }
            uint8_t measCfg2 = 0;
            if (Kernel::Read(m_I2CDevice, BME280_CTRL_MEAS_ADDR, &measCfg2, 1) != 1) {
                printf("ERROR: BME280 failed to read measurement config\n");
            }

            m_State = State_e::ReadingTempPressCalibration;
            RequestData(BME280_TEMP_PRESS_CALIB_DATA_ADDR, BME280_TEMP_PRESS_CALIB_DATA_LEN);
            //m_Timer.Start(10000);
        }
    }
    else if (m_State == State_e::ReadingTempPressCalibration && m_BytesReceived == m_BytesToReceive)
    {
        if (m_I2CDevice != -1)
        {
            m_CalibrationData.dig_T1 = BME280_CONCAT_BYTES(m_ReceiveBuffer[1], m_ReceiveBuffer[0]);
            m_CalibrationData.dig_T2 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[3], m_ReceiveBuffer[2]);
            m_CalibrationData.dig_T3 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[5], m_ReceiveBuffer[4]);
            m_CalibrationData.dig_P1 = BME280_CONCAT_BYTES(m_ReceiveBuffer[7], m_ReceiveBuffer[6]);
            m_CalibrationData.dig_P2 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[9], m_ReceiveBuffer[8]);
            m_CalibrationData.dig_P3 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[11], m_ReceiveBuffer[10]);
            m_CalibrationData.dig_P4 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[13], m_ReceiveBuffer[12]);
            m_CalibrationData.dig_P5 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[15], m_ReceiveBuffer[14]);
            m_CalibrationData.dig_P6 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[17], m_ReceiveBuffer[16]);
            m_CalibrationData.dig_P7 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[19], m_ReceiveBuffer[18]);
            m_CalibrationData.dig_P8 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[21], m_ReceiveBuffer[20]);
            m_CalibrationData.dig_P9 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[23], m_ReceiveBuffer[22]);
            m_CalibrationData.dig_H1 = m_ReceiveBuffer[25];

            m_State = State_e::ReadingHumidityCalibration;
            RequestData(BME280_HUMIDITY_CALIB_DATA_ADDR, BME280_HUMIDITY_CALIB_DATA_LEN);
        }
    }
    else if (m_State == State_e::ReadingHumidityCalibration && m_BytesReceived == m_BytesToReceive)
    {
        if (m_I2CDevice != -1)
        {
            int16_t dig_H4_lsb;
            int16_t dig_H4_msb;
            int16_t dig_H5_lsb;
            int16_t dig_H5_msb;

            m_CalibrationData.dig_H2 = (int16_t)BME280_CONCAT_BYTES(m_ReceiveBuffer[1], m_ReceiveBuffer[0]);
            m_CalibrationData.dig_H3 = m_ReceiveBuffer[2];

            dig_H4_msb = (int16_t)(int8_t)m_ReceiveBuffer[3] * 16;
            dig_H4_lsb = (int16_t)(m_ReceiveBuffer[4] & 0x0F);
            m_CalibrationData.dig_H4 = dig_H4_msb | dig_H4_lsb;

            dig_H5_msb = (int16_t)(int8_t)m_ReceiveBuffer[5] * 16;
            dig_H5_lsb = (int16_t)(m_ReceiveBuffer[4] >> 4);
            m_CalibrationData.dig_H5 = dig_H5_msb | dig_H5_lsb;
            m_CalibrationData.dig_H6 = (int8_t)m_ReceiveBuffer[6];
            m_State = State_e::Idle;
            m_BytesToReceive = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int BME280Driver::DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    switch(request)
    {
        case BME280IOCTL_GET_VALUES:
            if (outData == nullptr || outDataLength < sizeof(BME280Values)) {
                set_last_error(EINVAL);
                return -1;
            }
            memcpy(outData, &m_CurrentValues, outDataLength);
            return sizeof(BME280Values);
        default:
            return -1;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t BME280Driver::Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void BME280Driver::RequestData(uint8_t address, uint8_t length)
{
    assert(length <= sizeof(m_ReceiveBuffer));
    m_BytesToReceive = length;
    m_ReadStartTime  = get_system_time();
    m_ReadRetryCount = 0;
    m_BytesReceived = Kernel::Read(m_I2CDevice, address, m_ReceiveBuffer, m_BytesToReceive);
    if (m_BytesReceived != m_BytesToReceive) {
        printf("ERROR: BME280Driver failed to read from device (%d/%d): %s\n", m_BytesReceived, m_BytesToReceive, strerror(get_last_error()));        
    }
}

double BME280Driver::CompensateTemperature(uint32_t uncompTemperature)
{
    double var1;
    double var2;
    double temperature;
    double temperature_min = -40;
    double temperature_max = 85;

    var1 = ((double)uncompTemperature) / 16384.0 - ((double)m_CalibrationData.dig_T1) / 1024.0;
    var1 = var1 * ((double)m_CalibrationData.dig_T2);
    var2 = (((double)uncompTemperature) / 131072.0 - ((double)m_CalibrationData.dig_T1) / 8192.0);
    var2 = (var2 * var2) * ((double)m_CalibrationData.dig_T3);
    m_CalibrationData.t_fine = (int32_t)(var1 + var2);
    temperature = (var1 + var2) / 5120.0;

    if (temperature < temperature_min)
    temperature = temperature_min;
    else if (temperature > temperature_max)
    temperature = temperature_max;

    return temperature;
}

double BME280Driver::CompensatePressure(uint32_t uncompPressure) const
{
    double var1;
    double var2;
    double var3;
    double pressure;
    double pressure_min = 30000.0;
    double pressure_max = 110000.0;

    var1 = ((double)m_CalibrationData.t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)m_CalibrationData.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)m_CalibrationData.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)m_CalibrationData.dig_P4) * 65536.0);
    var3 = ((double)m_CalibrationData.dig_P3) * var1 * var1 / 524288.0;
    var1 = (var3 + ((double)m_CalibrationData.dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)m_CalibrationData.dig_P1);
    // Avoid exception caused by division by zero
    if (var1)
    {
        pressure = 1048576.0 - (double) uncompPressure;
        pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
        var1 = ((double)m_CalibrationData.dig_P9) * pressure * pressure / 2147483648.0;
        var2 = pressure * ((double)m_CalibrationData.dig_P8) / 32768.0;
        pressure = pressure + (var1 + var2 + ((double)m_CalibrationData.dig_P7)) / 16.0;

        if (pressure < pressure_min) {
            pressure = pressure_min;
        } else if (pressure > pressure_max) {
            pressure = pressure_max;
        }
    }
    else
    { // Invalid case
        pressure = pressure_min;
    }
    return pressure;
}

/*!
 * @brief This internal API is used to compensate the raw humidity data and
 * return the compensated humidity data in double data type.
 */
double BME280Driver::CompensateHumidity(uint32_t uncompHumidity) const
{
    double humidity;
    double humidity_min = 0.0;
    double humidity_max = 100.0;
    double var1;
    double var2;
    double var3;
    double var4;
    double var5;
    double var6;

    var1 = ((double)m_CalibrationData.t_fine) - 76800.0;
    var2 = (((double)m_CalibrationData.dig_H4) * 64.0 + (((double)m_CalibrationData.dig_H5) / 16384.0) * var1);
    var3 = uncompHumidity - var2;
    var4 = ((double)m_CalibrationData.dig_H2) / 65536.0;
    var5 = (1.0 + (((double)m_CalibrationData.dig_H3) / 67108864.0) * var1);
    var6 = 1.0 + (((double)m_CalibrationData.dig_H6) / 67108864.0) * var1 * var5;
    var6 = var3 * var4 * (var5 * var6);
    humidity = var6 * (1.0 - ((double)m_CalibrationData.dig_H1) * var6 / 524288.0);

    if (humidity > humidity_max) {
        humidity = humidity_max;
    } else if (humidity < humidity_min) {
        humidity = humidity_min;
    }
    return humidity;
}
