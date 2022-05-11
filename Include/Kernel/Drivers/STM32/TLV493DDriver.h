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

#pragma once

#include <cmath>

#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/VFS/KDeviceNode.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "DeviceControl/TLV493D.h"
#include "Utils/Utils.h"
#include "Threads/Thread.h"

namespace kernel
{

// I2C addresses
#define TLV493D_I2C_ADDR_PRIM    0xbc
#define TLV493D_I2C_ADDR_SEC     0x3e





struct tlv493d_read_registers
{
    uint8_t BXH;
    uint8_t BYH;
    uint8_t BZH;
    uint8_t TempHFrmCh;
    uint8_t BXYL;
    uint8_t FlagsBZL;
    uint8_t TempL;
    uint8_t DefaultCfg1;
    uint8_t DefaultCfg2;
    uint8_t DefaultCfg3;
};

// Fields in tlv493d_read_registers::TempHFrmCh
static const uint8_t TLV493D_CHANNEL_Pos = 0;
static const uint8_t TLV493D_CHANNEL = 0x03;
static const uint8_t TLV493D_FRAME_Pos  = 2;
static const uint8_t TLV493D_FRAME  = 0x0c;

// Fields in tlv493d_read_registers::FlagsBZL
static const uint8_t TLV493D_POWER_DOWN = 0x10;
static const uint8_t TLV493D_BZL_Pos = 0;
static const uint8_t TLV493D_BZL = 0xf;

struct tlv493d_write_registers
{
    uint8_t Reserved1;
    uint8_t Mode1;
    uint8_t Reserved2;
    uint8_t Mode2;
};

static const uint8_t TLV493D_MODE1_LOW      = 0x01;
static const uint8_t TLV493D_MODE1_FAST     = 0x02;
static const uint8_t TLV493D_MODE1_INT      = 0x04;
static const uint8_t TLV493D_MODE1_IICADDR1 = 0x20;
static const uint8_t TLV493D_MODE1_IICADDR2 = 0x40;
static const uint8_t TLV493D_MODE1_PARITY   = 0x80;

constexpr int32_t   TLV493D_MAX_FRAMERATE = 2475; // 3.3kHz - 25%
constexpr TimeValMicros TLV493D_CONVERSION_TIME = TimeValMicros::FromMicroseconds(TimeValMicros::TicksPerSecond / TLV493D_MAX_FRAMERATE);

class TLV493DDriver : public PtrTarget, public os::Thread, public SignalTarget, public KFilesystemFileOps
{
public:
    IFLASHC TLV493DDriver();
    IFLASHC ~TLV493DDriver();

    IFLASHC bool Setup(const char* devicePath, const char* i2cPath, DigitalPinID powerPin);

    IFLASHC virtual int Run() override;

    IFLASHC virtual int DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    IFLASHC virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;

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

    IFLASHC void ResetSensor();

    IFLASHC void ConfigChanged();
    IFLASHC void UpdateParity();

    KMutex  m_Mutex;
    KConditionVariable m_NewFrameCondition;
    KConditionVariable m_NewConfigCondition;

    State_e         m_State = State_e::Initializing;
    TimeValMicros   m_LastUpdateTime;
    TimeValMicros   m_PeriodTime;
    float           m_AveragingScale = 1.0f;
    DigitalPin      m_PowerPin;
    int             m_I2CDevice = -1;
    uint8_t         m_DeviceAddress = TLV493D_I2C_ADDR_PRIM;

    tlv493d_config  m_Config;

    tlv493d_read_registers m_ReadRegisters;
    tlv493d_write_registers m_WriteRegisters;

    tlv493d_data    m_CurrentValues;


    TLV493DDriver(const TLV493DDriver &) = delete;
    TLV493DDriver& operator=(const TLV493DDriver &) = delete;
};

} // namespace
