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

#pragma once

#include "Kernel/VFS/KDeviceNode.h"
#include "DeviceControl/BME280.h"
#include "System/Utils/Utils.h"
#include "System/Threads/Thread.h"
#include "System/Threads/Semaphore.h"
//#include "System/Utils/EventTimer.h"

namespace kernel
{

/**\name I2C addresses */
#define BME280_I2C_ADDR_PRIM	0x76
#define BME280_I2C_ADDR_SEC	0x77

/**\name BME280 chip identifier */
#define BME280_CHIP_ID  0x60

/**\name Register Address */
#define BME280_CHIP_ID_ADDR			0xD0
#define BME280_RESET_ADDR			0xE0
#define BME280_TEMP_PRESS_CALIB_DATA_ADDR	0x88
#define BME280_HUMIDITY_CALIB_DATA_ADDR		0xE1
#define BME280_PWR_CTRL_ADDR			0xF4
#define BME280_CTRL_HUM_ADDR			0xF2
#define BME280_CTRL_MEAS_ADDR			0xF4
#define BME280_CONFIG_ADDR			0xF5
#define BME280_DATA_ADDR			0xF7

/**\name API success code */
#define BME280_OK			0

/**\name API error codes */
#define BME280_E_NULL_PTR		(-1)
#define BME280_E_DEV_NOT_FOUND		(-2)
#define BME280_E_INVALID_LEN		(-3)
#define BME280_E_COMM_FAIL		(-4)
#define BME280_E_SLEEP_MODE_FAIL	(-5)

/**\name API warning codes */
#define BME280_W_INVALID_OSR_MACRO       1

/**\name Macros related to size */
#define BME280_TEMP_PRESS_CALIB_DATA_LEN 26
#define BME280_HUMIDITY_CALIB_DATA_LEN   7
#define BME280_P_T_H_DATA_LEN            8
#define BME280_MAX_DATA_LEN              BME280_TEMP_PRESS_CALIB_DATA_LEN

/**\name Sensor power modes */
#define	BME280_SLEEP_MODE		UINT8_C(0x00)
#define	BME280_FORCED_MODE		UINT8_C(0x01)
#define	BME280_NORMAL_MODE		UINT8_C(0x03)

/**\name Macro to combine two 8 bit data's to form a 16 bit data */
#define BME280_CONCAT_BYTES(msb, lsb)     (((uint16_t)msb << 8) | (uint16_t)lsb)

#define BME280_SET_BITS(reg_data, bitname, data) \
				((reg_data & ~(bitname##_MSK)) | \
				((data << bitname##_POS) & bitname##_MSK))
#define BME280_SET_BITS_POS_0(reg_data, bitname, data) \
				((reg_data & ~(bitname##_MSK)) | \
				(data & bitname##_MSK))

#define BME280_GET_BITS(reg_data, bitname)  ((reg_data & (bitname##_MSK)) >> \
							(bitname##_POS))
#define BME280_GET_BITS_POS_0(reg_data, bitname)  (reg_data & (bitname##_MSK))

// 
#define BME280_CTRL_MEAS_SENSOR_MODE_bp   0
#define BME280_CTRL_MEAS_SENSOR_MODE_bm   BIT8(BME280_CTRL_MEAS_SENSOR_MODE_bp,0x03)
#define BME280_CTRL_MEAS_SENSOR_MODE(v)   (BIT8(BME280_CTRL_MEAS_SENSOR_MODE_bp,v) & BME280_CTRL_MEAS_SENSOR_MODE_bm)
#define BME280_CTRL_MEAS_SENSOR_MODE_SLEEP  BME280_CTRL_MEAS_SENSOR_MODE(0)
#define BME280_CTRL_MEAS_SENSOR_MODE_FORCED BME280_CTRL_MEAS_SENSOR_MODE(1)
#define BME280_CTRL_MEAS_SENSOR_MODE_NORMAL BME280_CTRL_MEAS_SENSOR_MODE(3)

#define BME280_CTRL_MEAS_OVRSMPL_PRESS_bp 2
#define BME280_CTRL_MEAS_OVRSMPL_PRESS_bm BIT8(BME280_CTRL_MEAS_OVRSMPL_PRESS_bp, 0x07)
#define BME280_CTRL_MEAS_OVRSMPL_PRESS(v) (BIT8(BME280_CTRL_MEAS_OVRSMPL_PRESS_bp, v) & BME280_CTRL_MEAS_OVRSMPL_PRESS_bm)
#define BME280_CTRL_MEAS_OVRSMPL_PRESS_0  BME280_CTRL_MEAS_OVRSMPL_PRESS(0)
#define BME280_CTRL_MEAS_OVRSMPL_PRESS_1  BME280_CTRL_MEAS_OVRSMPL_PRESS(1)
#define BME280_CTRL_MEAS_OVRSMPL_PRESS_2  BME280_CTRL_MEAS_OVRSMPL_PRESS(2)
#define BME280_CTRL_MEAS_OVRSMPL_PRESS_4  BME280_CTRL_MEAS_OVRSMPL_PRESS(3)
#define BME280_CTRL_MEAS_OVRSMPL_PRESS_8  BME280_CTRL_MEAS_OVRSMPL_PRESS(4)
#define BME280_CTRL_MEAS_OVRSMPL_PRESS_16 BME280_CTRL_MEAS_OVRSMPL_PRESS(5)

#define BME280_CTRL_MEAS_OVRSMPL_TEMP_bp 5
#define BME280_CTRL_MEAS_OVRSMPL_TEMP_bm BIT8(BME280_CTRL_MEAS_OVRSMPL_TEMP_bp, 0x07)
#define BME280_CTRL_MEAS_OVRSMPL_TEMP(v) (BIT8(BME280_CTRL_MEAS_OVRSMPL_TEMP_bp, v) & BME280_CTRL_MEAS_OVRSMPL_TEMP_bm)
#define BME280_CTRL_MEAS_OVRSMPL_TEMP_0  BME280_CTRL_MEAS_OVRSMPL_TEMP(0)
#define BME280_CTRL_MEAS_OVRSMPL_TEMP_1  BME280_CTRL_MEAS_OVRSMPL_TEMP(1)
#define BME280_CTRL_MEAS_OVRSMPL_TEMP_2  BME280_CTRL_MEAS_OVRSMPL_TEMP(2)
#define BME280_CTRL_MEAS_OVRSMPL_TEMP_4  BME280_CTRL_MEAS_OVRSMPL_TEMP(3)
#define BME280_CTRL_MEAS_OVRSMPL_TEMP_8  BME280_CTRL_MEAS_OVRSMPL_TEMP(4)
#define BME280_CTRL_MEAS_OVRSMPL_TEMP_16 BME280_CTRL_MEAS_OVRSMPL_TEMP(5)

#define BME280_CTRL_HUM_OVRSMPL_bp 0
#define BME280_CTRL_HUM_OVRSMPL_bm BIT8(BME280_CTRL_HUM_OVRSMPL_bp, 0x07)
#define BME280_CTRL_HUM_OVRSMPL(v) (BIT8(BME280_CTRL_HUM_OVRSMPL_bp, v) & BME280_CTRL_HUM_OVRSMPL_bm)
#define BME280_CTRL_HUM_OVRSMPL_0  BME280_CTRL_HUM_OVRSMPL(0)
#define BME280_CTRL_HUM_OVRSMPL_1  BME280_CTRL_HUM_OVRSMPL(1)
#define BME280_CTRL_HUM_OVRSMPL_2  BME280_CTRL_HUM_OVRSMPL(2)
#define BME280_CTRL_HUM_OVRSMPL_4  BME280_CTRL_HUM_OVRSMPL(3)
#define BME280_CTRL_HUM_OVRSMPL_8  BME280_CTRL_HUM_OVRSMPL(4)
#define BME280_CTRL_HUM_OVRSMPL_16 BME280_CTRL_HUM_OVRSMPL(5)

#define BME280_FILTER_MSK		UINT8_C(0x1C)
#define BME280_FILTER_POS		UINT8_C(0x02)

#define BME280_STANDBY_MSK		UINT8_C(0xE0)
#define BME280_STANDBY_POS		UINT8_C(0x05)

/**\name Sensor component selection macros
   These values are internal for API implementation. Don't relate this to
   data sheet.*/
#define BME280_PRESS		UINT8_C(1)
#define BME280_TEMP			UINT8_C(1 << 1)
#define BME280_HUM			UINT8_C(1 << 2)
#define BME280_ALL			UINT8_C(0x07)

/**\name Settings selection macros */
#define BME280_OSR_PRESS_SEL		UINT8_C(1)
#define BME280_OSR_TEMP_SEL			UINT8_C(1 << 1)
#define BME280_OSR_HUM_SEL			UINT8_C(1 << 2)
#define BME280_FILTER_SEL			UINT8_C(1 << 3)
#define BME280_STANDBY_SEL			UINT8_C(1 << 4)
#define BME280_ALL_SETTINGS_SEL		UINT8_C(0x1F)

/**\name Oversampling macros */
#define BME280_NO_OVERSAMPLING		UINT8_C(0x00)
#define BME280_OVERSAMPLING_1X		UINT8_C(0x01)
#define BME280_OVERSAMPLING_2X		UINT8_C(0x02)
#define BME280_OVERSAMPLING_4X		UINT8_C(0x03)
#define BME280_OVERSAMPLING_8X		UINT8_C(0x04)
#define BME280_OVERSAMPLING_16X		UINT8_C(0x05)

/**\name Standby duration selection macros */
#define BME280_STANDBY_TIME_1_MS              (0x00)
#define BME280_STANDBY_TIME_62_5_MS           (0x01)
#define BME280_STANDBY_TIME_125_MS			  (0x02)
#define BME280_STANDBY_TIME_250_MS            (0x03)
#define BME280_STANDBY_TIME_500_MS            (0x04)
#define BME280_STANDBY_TIME_1000_MS           (0x05)
#define BME280_STANDBY_TIME_10_MS             (0x06)
#define BME280_STANDBY_TIME_20_MS             (0x07)

/**\name Filter coefficient selection macros */
#define BME280_FILTER_COEFF_OFF               (0x00)
#define BME280_FILTER_COEFF_2                 (0x01)
#define BME280_FILTER_COEFF_4                 (0x02)
#define BME280_FILTER_COEFF_8                 (0x03)
#define BME280_FILTER_COEFF_16                (0x04)

/*!
 * @brief Interface selection Enums
 */
enum bme280_intf
{
	/*! SPI interface */
	BME280_SPI_INTF,
	/*! I2C interface */
	BME280_I2C_INTF
};

/*!
 * @brief Type definitions
 */
//typedef int8_t (*bme280_com_fptr_t)(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len);
//typedef void (*bme280_delay_fptr_t)(uint32_t period);

/*!
 * @brief Calibration data
 */
struct bme280_calib_data
{
 /**
 * @ Trim Variables
 */
/**@{*/
    uint16_t dig_T1;    
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t  dig_H1;
    int16_t dig_H2;
    uint8_t  dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t  dig_H6;
    int32_t t_fine;
/**@}*/
} __attribute__((packed));

/*!
 * @brief bme280 sensor structure which comprises of temperature, pressure and
 * humidity data
 */
struct bme280_data {
	/*! Compensated pressure */
	double pressure;
	/*! Compensated temperature */
	double temperature;
	/*! Compensated humidity */
	double humidity;
};

/*!
 * @brief bme280 sensor structure which comprises of uncompensated temperature,
 * pressure and humidity data
 */
 #if 0
struct bme280_uncomp_data {
	/*! un-compensated pressure */
	uint32_t pressure;
	/*! un-compensated temperature */
	uint32_t temperature;
	/*! un-compensated humidity */
	uint32_t humidity;
};
#endif
/*!
 * @brief bme280 sensor settings structure which comprises of mode,
 * oversampling and filter settings.
 */
struct bme280_settings {
	/*! pressure oversampling */
	uint8_t osr_p;
	/*! temperature oversampling */
	uint8_t osr_t;
	/*! humidity oversampling */
	uint8_t osr_h;
	/*! filter coefficient */
	uint8_t filter;
	/*! standby time */
	uint8_t standby_time;
};


class BME280Driver : public KDeviceNode, public Thread, public SignalTarget
{
public:
    BME280Driver(const char* i2cPath);
    ~BME280Driver();

    virtual int Run() override;
    void SlotTick();

    bool Initialize(const char* i2cPath);

    virtual int DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual ssize_t Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length) override;

private:
    enum class State_e
    {
        Initializing,
        Idle,
        ReadingTempPressCalibration,
        ReadingHumidityCalibration,
        ReadingRegister,
        ProcessingRegister
    };

    void RequestData(uint8_t address, uint8_t length);

    double CompensateTemperature(uint32_t uncompTemperature);
    double CompensatePressure(uint32_t uncompPressure) const;
    double CompensateHumidity(uint32_t uncompHumidity) const;

    static void TransactionCallback(void* userObject, void* data, ssize_t length);
    void TransactionCallback(void* data, ssize_t length);

//    EventTimer m_Timer;

    Semaphore m_Mutex;
    State_e m_State = State_e::Initializing;

    bigtime_t m_UpdatePeriode  = 500000;
    bigtime_t m_LastUpdateTime = 0;
    bigtime_t m_ReadStartTime = 0;
    bigtime_t m_ReadRetryCount = 0;

    int m_I2CDevice = -1;
    //I2C* m_Port = nullptr;
    uint8_t m_DeviceAddress = BME280_I2C_ADDR_PRIM;


    bme280_calib_data m_CalibrationData;

    BME280Values m_CurrentValues;
/*    double m_Temperature = 0.0;
    double m_Pressure    = 0.0;
    double m_Humidity    = 0.0;*/


    uint8_t m_ReceiveBuffer[BME280_MAX_DATA_LEN];
    uint8_t m_BytesToReceive = 0;
    uint8_t m_BytesReceived = 0;

    BME280Driver( const BME280Driver &c );
    BME280Driver& operator=( const BME280Driver &c );
};

} // namespace
