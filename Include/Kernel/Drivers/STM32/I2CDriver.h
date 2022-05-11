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

#pragma once

#include <atomic>

#include "Kernel/VFS/KDeviceNode.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/IRQDispatcher.h"
#include "Kernel/KSemaphore.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/HAL/DigitalPort.h"
#include "DeviceControl/I2C.h"


namespace kernel
{
class I2CDriver;
class KSemaphore;
enum class I2CID : int;
enum class SPIID : int;

enum class I2CSpeed : int
{
    Standard,
    Fast,
    FastPlus
};

struct I2CSpec
{
    uint32_t Baudrate;
    uint32_t BaudrateMin;
    double ClockLowMin;
    double ClockHighMin;
    double DataHoldTimeMin;
    double DataValidTimeMax;
    double DataSetupTimeMin;
    double FallTimeMax;
    double RiseTimeMax;
};

constexpr I2CSpec I2CSpecs[] =
{
    [int(I2CSpeed::Standard)] =
    {
        .Baudrate = 100000,
        .BaudrateMin = 80000,
        .ClockLowMin = 4700.0e-9,
        .ClockHighMin = 4000.0e-9,
        .DataHoldTimeMin = 0.0,
        .DataValidTimeMax = 3450.0e-9,
        .DataSetupTimeMin = 250.0e-9,
        .FallTimeMax = 300.0e-9,
        .RiseTimeMax = 1000.0e-9,
    },
    [int(I2CSpeed::Fast)] =
    {
        .Baudrate = 400000,
        .BaudrateMin = 320000,
        .ClockLowMin = 1300.0e-9,
        .ClockHighMin = 600.0e-9,
        .DataHoldTimeMin = 0.0,
        .DataValidTimeMax = 900.0e-9,
        .DataSetupTimeMin = 100.0e-9,
        .FallTimeMax = 300.0e-9,
        .RiseTimeMax = 300.0e-9,
    },
    [int(I2CSpeed::FastPlus)] =
    {
        .Baudrate = 1000000,
        .BaudrateMin = 800000,
        .ClockLowMin = 500.0e-9,
        .ClockHighMin = 260.0e-9,
        .DataHoldTimeMin = 0.0,
        .DataValidTimeMax = 450.0e-9,
        .DataSetupTimeMin = 50.0e-9,
        .FallTimeMax = 100.0e-9,
        .RiseTimeMax = 120.0e-9,
    },
};

class I2CFile : public KFileNode
{
public:
    I2CFile(int openFlags) : KFileNode(openFlags) {}

    I2C_ADDR_LEN    m_SlaveAddressLength = I2C_ADDR_LEN_7BIT;
    uint8_t         m_SlaveAddress = 0;
    int8_t          m_InternalAddressLength = 0;
    TimeValMicros   m_Timeout = TimeValMicros::infinit; // Timeout for any IO operations.
};


class I2CDriverINode : public KINode
{
public:
    IFLASHC I2CDriverINode(I2CDriver* driver, I2CID portID, const PinMuxTarget& clockPin, const PinMuxTarget& dataPin, uint32_t clockFrequency, double fallTime, double riseTime);
    IFLASHC virtual ~I2CDriverINode() override;


    IFLASHC Ptr<KFileNode> Open(int flags);

    IFLASHC int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);
    IFLASHC ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length);
    IFLASHC ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length);

private:
    IFLASHC void ResetPeripheral();
    IFLASHC void ClearBus();
    IFLASHC int SetSpeed(I2CSpeed speed);
    IFLASHC int GetBaudrate() const;

    I2CDriverINode(const I2CDriverINode&) = delete;
    I2CDriverINode& operator=(const I2CDriverINode&) = delete;

    enum class State_e
    {
        Idle,
        SendReadAddress,
        SendWriteAddress,
        Reading,
        Writing
    };
    
    IFLASHC void UpdateTransactionLength(uint32_t& CR2);

    static IFLASHC IRQResult IRQCallbackEvent(IRQn_Type irq, void* userData);
    IFLASHC IRQResult HandleEventIRQ();

    static IFLASHC IRQResult IRQCallbackError(IRQn_Type irq, void* userData);
    IFLASHC IRQResult HandleErrorIRQ();

    KMutex m_Mutex;
    KConditionVariable      m_RequestCondition;
    Ptr<I2CDriver>          m_Driver;
    I2C_TypeDef*            m_Port;
    PinMuxTarget            m_ClockPin;
    PinMuxTarget            m_DataPin;
    uint32_t                m_ClockFrequency;
    double                  m_FallTime;
    double                  m_RiseTime;

    std::atomic<State_e>    m_State;
    bool                    m_AnalogFilterEnabled = true;
    int                     m_DigitalFilterCount = 0;
    uint32_t                m_Baudrate = 400000;

    uint8_t                 m_RegisterAddress[4];
    int8_t                  m_RegisterAddressLength = 0;
    int8_t                  m_RegisterAddressPos = 0;
    uint8_t*                m_Buffer = nullptr;
    int32_t                 m_Length = 0;
    volatile int32_t        m_CurPos = 0;
};

struct I2CDriverSetup
{
    os::String      DevicePath;
    I2CID           PortID;
    PinMuxTarget    ClockPin;
    PinMuxTarget    DataPin;
    uint32_t        ClockFrequency;
    double          FallTime;
    double          RiseTime;
};

class I2CDriver : public PtrTarget, public KFilesystemFileOps
{
public:
    IFLASHC void Setup(const char* devicePath, I2CID portID, const PinMuxTarget& clockPin, const PinMuxTarget& dataPin, uint32_t clockFrequency, double fallTime, double riseTime);
    IFLASHC void Setup(const I2CDriverSetup& setup);

    IFLASHC virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int flags) override;
    IFLASHC virtual int              CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;

    IFLASHC virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    IFLASHC virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    IFLASHC virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

};
    
} // namespace
