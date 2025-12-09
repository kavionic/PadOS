// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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

#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KDriverParametersBase.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KSemaphore.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/HAL/DigitalPort.h>
#include <DeviceControl/I2C.h>

enum class I2CID : int;
enum class SPIID : int;

enum class I2CSpeed : int
{
    Standard,
    Fast,
    FastPlus
};

struct I2CDriverParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "i2c";

    I2CDriverParameters() = default;
    I2CDriverParameters(
        const PString&  devicePath,
        I2CID           portID,
        PinMuxTarget    clockPin,
        PinMuxTarget    dataPin,
        double          fallTime,
        double          riseTime
    )
        : KDriverParametersBase(devicePath)
        , PortID(portID)
        , ClockPin(clockPin)
        , DataPin(dataPin)
        , FallTime(fallTime)
        , RiseTime(riseTime)
    {}

    I2CID           PortID;
    PinMuxTarget    ClockPin;
    PinMuxTarget    DataPin;
    double          FallTime;
    double          RiseTime;

    friend void to_json(Pjson& data, const I2CDriverParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
        data.update(Pjson{
            {"port_id",         value.PortID },
            {"pin_clock",       value.ClockPin },
            {"pin_data",        value.DataPin },
            {"fall_time",       value.FallTime },
            {"rise_time",       value.RiseTime }
        });
    }
    friend void from_json(const Pjson& data, I2CDriverParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));

        data.at("port_id").get_to(outValue.PortID);
        data.at("pin_clock").get_to(outValue.ClockPin);
        data.at("pin_data").get_to(outValue.DataPin);
        data.at("fall_time").get_to(outValue.FallTime);
        data.at("rise_time").get_to(outValue.RiseTime);
    }

};


namespace kernel
{
class I2CDriver;
class KSemaphore;

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
    TimeValNanos   m_Timeout = TimeValNanos::infinit; // Timeout for any IO operations.
};


class I2CDriverINode : public KINode, public KFilesystemFileOps
{
public:
    I2CDriverINode(const I2CDriverParameters& parameters);
    virtual ~I2CDriverINode() override;


    Ptr<KFileNode> Open(int flags);
    virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int flags) override;

    virtual void   DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual size_t Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;

private:
    void ResetPeripheral();
    void ClearBus();
    int SetSpeed(I2CSpeed speed);
    int GetBaudrate() const;

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
    
    void UpdateTransactionLength(uint32_t& CR2);

    static IRQResult IRQCallbackEvent(IRQn_Type irq, void* userData);
    IRQResult HandleEventIRQ();

    static IRQResult IRQCallbackError(IRQn_Type irq, void* userData);
    IRQResult HandleErrorIRQ();

    KMutex m_Mutex;
    KConditionVariable      m_RequestCondition;
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
    volatile PErrorCode     m_TransactionError = PErrorCode::Success;
};

    
} // namespace
