// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 01.06.2020 16:30:05

#pragma once

#include <System/Platform.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KDriverParametersBase.h>
#include <Kernel/HAL/STM32/DMARequestID.h>
#include <Kernel/HAL/DigitalPort.h>

enum class SPIID : int;

struct WS2812BDriverParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "ws2812b";

    WS2812BDriverParameters() = default;
    WS2812BDriverParameters(
        const PString& devicePath,
        SPIID           portID,
        PinMuxTarget    pinData,
        bool            swapIOPins = false
    )
        : KDriverParametersBase(devicePath)
        , PortID(portID)
        , PinData(pinData)
        , SwapIOPins(swapIOPins)
    {}

    SPIID           PortID;
    PinMuxTarget    PinData;
    bool            SwapIOPins = false;

    friend void to_json(Pjson& data, const WS2812BDriverParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
        data.update(Pjson{
            {"port_id",         value.PortID},
            {"pin_data",        value.PinData},
            {"swap_io_pins",    value.SwapIOPins}
        });
    }
    friend void from_json(const Pjson& data, WS2812BDriverParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));

        data.at("port_id").get_to(outValue.PortID);
        data.at("pin_data").get_to(outValue.PinData);
        data.at("swap_io_pins").get_to(outValue.SwapIOPins);
    }
};

namespace kernel
{

class WS2812BDriverInode : public KInode, public KFilesystemFileOps
{
public:
    WS2812BDriverInode(const WS2812BDriverParameters& parameters);

    virtual void   DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual size_t Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void   ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

private:
    enum class State
    {
        Idle,
        Sending
    };

    static constexpr size_t RESET_BYTE_COUNT = 900; // 280e-6/(1/(200e6/64)) = 875
    static IRQResult IRQCallbackSend(IRQn_Type irq, void* userData);
    IRQResult HandleIRQ();

    PErrorCode WaitForIdle();

    void SetLEDCount(size_t count);
    size_t GetLEDCount() const;

    void SetExponential(bool exponential);
    bool GetExponential() const;

    KMutex              m_Mutex;
    KConditionVariable  m_TransmitCondition;

    volatile State m_State = State::Idle;

    SPI_TypeDef*    m_Port;
    DMAMUX_REQUEST  m_DMARequestTX;

    int     m_SendDMAChannel = -1;
    int32_t     m_ReceiveBufferSize = 1024;
    int32_t     m_ReceiveBufferOutPos = 0;
    int32_t     m_ReceiveBufferInPos = 0;
    size_t      m_LEDCount = 0;
    size_t      m_TransmitBufferSize = 0;
    uint8_t*    m_TransmitBuffer = nullptr;
    bool        m_Exponential = true;
};


} // namespace
