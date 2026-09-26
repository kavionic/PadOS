// This file is part of PadOS.
//
// Copyright (c) 2020-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.05.2020 23:00:00



#pragma once

#include <Kernel/Drivers/SDMMCDriver/SDMMCDriver.h>
#include <Kernel/HAL/STM32/PinMuxTarget_STM32H7.h>

enum class SDMMC_ID : int;

struct SDMMCDriverParameters : SDMMCBaseDriverParameters
{
    static constexpr char DRIVER_NAME[] = "sdmmc";

    SDMMCDriverParameters() = default;
    SDMMCDriverParameters(
        const PString&  devicePath,
        SDMMC_ID        portID,
        DigitalPinID    pinCardDetect,
        PinMuxTarget    pinD0 = PINMUX_NONE,
        PinMuxTarget    pinD1 = PINMUX_NONE,
        PinMuxTarget    pinD2 = PINMUX_NONE,
        PinMuxTarget    pinD3 = PINMUX_NONE,
        PinMuxTarget    pinD4 = PINMUX_NONE,
        PinMuxTarget    pinD5 = PINMUX_NONE,
        PinMuxTarget    pinD6 = PINMUX_NONE,
        PinMuxTarget    pinD7 = PINMUX_NONE,
        PinMuxTarget    pinCMD = PINMUX_NONE,
        PinMuxTarget    pinCK = PINMUX_NONE,
        PinMuxTarget    pinCKIN = PINMUX_NONE,
        PinMuxTarget    pinCDIR = PINMUX_NONE,
        PinMuxTarget    pinD0DIR = PINMUX_NONE,
        PinMuxTarget    pinD123DIR = PINMUX_NONE,
        uint32_t        clockFrequency = 0,
        uint32_t        clockCap = 0
    )
        : SDMMCBaseDriverParameters(devicePath, pinCardDetect)
        , PortID(portID)
        , PinD0(pinD0)
        , PinD1(pinD1)
        , PinD2(pinD2)
        , PinD3(pinD3)
        , PinD4(pinD4)
        , PinD5(pinD5)
        , PinD6(pinD6)
        , PinD7(pinD7)
        , PinCMD(pinCMD)
        , PinCK(pinCK)
        , PinCKIN(pinCKIN)
        , PinCDIR(pinCDIR)
        , PinD0DIR(pinD0DIR)
        , PinD123DIR(pinD123DIR)
        , ClockFrequency(clockFrequency)
        , ClockCap(clockCap)
    {}

    SDMMC_ID        PortID;

    PinMuxTarget    PinD0       = PINMUX_NONE;
    PinMuxTarget    PinD1       = PINMUX_NONE;
    PinMuxTarget    PinD2       = PINMUX_NONE;
    PinMuxTarget    PinD3       = PINMUX_NONE;
    PinMuxTarget    PinD4       = PINMUX_NONE;
    PinMuxTarget    PinD5       = PINMUX_NONE;
    PinMuxTarget    PinD6       = PINMUX_NONE;
    PinMuxTarget    PinD7       = PINMUX_NONE;
    PinMuxTarget    PinCMD      = PINMUX_NONE;
    PinMuxTarget    PinCK       = PINMUX_NONE;
    PinMuxTarget    PinCKIN     = PINMUX_NONE;
    PinMuxTarget    PinCDIR     = PINMUX_NONE;
    PinMuxTarget    PinD0DIR    = PINMUX_NONE;
    PinMuxTarget    PinD123DIR  = PINMUX_NONE;

    uint32_t        ClockFrequency  = 0;
    uint32_t        ClockCap        = 0;

    friend void to_json(Pjson& data, const SDMMCDriverParameters& value)
    {
        to_json(data, static_cast<const SDMMCBaseDriverParameters&>(value));
        data.update(Pjson{
            {"port_id",         value.PortID},
            {"pin_d0",          value.PinD0},
            {"pin_d1",          value.PinD1},
            {"pin_d2",          value.PinD2},
            {"pin_d3",          value.PinD3},
            {"pin_d4",          value.PinD4},
            {"pin_d5",          value.PinD5},
            {"pin_d6",          value.PinD6},
            {"pin_d7",          value.PinD7},
            {"pin_cmd",         value.PinCMD},
            {"pin_ck",          value.PinCK},
            {"pin_ckin",        value.PinCKIN},
            {"pin_cdir",        value.PinCDIR},
            {"pin_d0dir",       value.PinD0DIR},
            {"pin_d123dir",     value.PinD123DIR},
            {"clock_frequency", value.ClockFrequency},
            {"clock_cap",       value.ClockCap}
        });
    }
    friend void from_json(const Pjson& data, SDMMCDriverParameters& outValue)
    {
        from_json(data, static_cast<SDMMCBaseDriverParameters&>(outValue));

        data.at("port_id").get_to(outValue.PortID);
        data.at("pin_d0").get_to(outValue.PinD0);
        data.at("pin_d1").get_to(outValue.PinD1);
        data.at("pin_d2").get_to(outValue.PinD2);
        data.at("pin_d3").get_to(outValue.PinD3);
        data.at("pin_d4").get_to(outValue.PinD4);
        data.at("pin_d5").get_to(outValue.PinD5);
        data.at("pin_d6").get_to(outValue.PinD6);
        data.at("pin_d7").get_to(outValue.PinD7);
        data.at("pin_cmd").get_to(outValue.PinCMD);
        data.at("pin_ck").get_to(outValue.PinCK);
        data.at("pin_ckin").get_to(outValue.PinCKIN);
        data.at("pin_cdir").get_to(outValue.PinCDIR);
        data.at("pin_d0dir").get_to(outValue.PinD0DIR);
        data.at("pin_d123dir").get_to(outValue.PinD123DIR);
        data.at("clock_frequency").get_to(outValue.ClockFrequency);
        data.at("clock_cap").get_to(outValue.ClockCap);
    }
};

namespace kernel
{

enum class WakeupReason : int
{
    None,
    DataComplete,
    Error,
    Event
};

class SDMMCDriver_STM32 : public SDMMCDriver
{
public:
    SDMMCDriver_STM32(const SDMMCDriverParameters& parameters);
    ~SDMMCDriver_STM32();

    virtual size_t   Read(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position) override;
    virtual size_t   Write(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position) override;

private:
    static constexpr size_t TRANSFER_BUFFER_SIZE = 64 * 1024;
    static constexpr size_t MAX_DATA_TRANSFER_SIZE = SDMMC_DLEN_DATALENGTH_Msk & ~(BLOCK_SIZE - 1);
    static constexpr size_t MAX_IDMA_BUFFER_SIZE =
        ((SDMMC_IDMABSIZE_IDMABNDT_Msk >> SDMMC_IDMABSIZE_IDMABNDT_Pos) * 32) & ~(BLOCK_SIZE - 1);
    static_assert((TRANSFER_BUFFER_SIZE % BLOCK_SIZE) == 0);

    static constexpr uint32_t DATA_IRQ_FLAGS =
        SDMMC_MASK_DATAENDIE | SDMMC_MASK_DABORTIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_DCRCFAILIE
        | SDMMC_MASK_TXUNDERRIE | SDMMC_MASK_RXOVERRIE;

    enum class DMATransferError
    {
        None,
        UnexpectedBuffer,
        AddressUpdateRejected,
        CompletionCountMismatch
    };

    struct TransferRequest
    {
        off64_t Position = 0;
        size_t Length = 0;
        bool LockCardState = false;
    };

    struct IOVectorCursor
    {
        IOVectorCursor(const iovec_t* segments, size_t segmentCount, size_t length);

        void Normalize();
        size_t GetCurrentLength() const;
        uint8_t* GetCurrentAddress() const;
        void Advance(size_t length);
        void CopyTo(void* destination, size_t length) const;
        void CopyFrom(const void* source, size_t length) const;

        const iovec_t* Segments = nullptr;
        size_t SegmentCount = 0;
        size_t SegmentIndex = 0;
        size_t SegmentOffset = 0;
        size_t RemainingLength = 0;
    };

    bool ExecuteCmd(uint32_t extraCmdRFlags, uint32_t cmd, uint32_t arg);

    virtual bool        SendCmd(uint32_t cmd, uint32_t arg) override;
    virtual uint32_t    GetResponse() override;
    virtual void        GetResponse128(uint8_t* response) override;
    virtual bool        StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint32_t blockCount, void* buffer) override;
    bool StartDataTransfer(
        uint32_t cmd,
        uint32_t arg,
        uint32_t blockSizePower,
        uint32_t blockCount,
        const IOVectorCursor& transfer);
    virtual bool        StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg) override;
    virtual void        ApplySpeedAndBusWidth() override;

    TransferRequest PrepareTransferRequest(const Ptr<KFileNode>& file, const iovec_t* segments, size_t segmentCount, off64_t position) const;
    IOVectorCursor PrepareDirectTransfer(IOVectorCursor& cursor, bool isWrite) const;
    static size_t GetDMABufferSize(const IOVectorCursor& transfer);
    uintptr_t GetNextDMABufferAddress();
    void ReadBlocks(uint32_t firstBlock, const IOVectorCursor& transfer);
    void WriteBlocks(uint32_t firstBlock, const IOVectorCursor& transfer);

    static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    static void      DeferredIRQCallback(IRQn_Type irq, void* userData);
    IRQResult        HandleIRQ();
    void             HandleDeferredIRQ();
    DMATransferError HandleDMABufferComplete();
    IRQResult        FailDMATransfer(DMATransferError error);

    bool     WaitIRQ(uint32_t flags);

    virtual void     Reset() override;
    virtual void     SetClockFrequency(uint32_t frequency) override;
    virtual void     SendClock() override;

    SDMMC_TypeDef*  m_SDMMC;
    const IRQn_Type m_IRQ;
    uint32_t        m_PeripheralClockFrequency = 0;
    uint32_t        m_ClockCap = 0;

    const iovec_t* m_TransferSegments = nullptr;
    size_t m_TransferSegmentIndex = 0;
    size_t m_TransferSegmentOffset = 0;
    size_t m_DMABufferSize = 0;
    size_t m_DMABufferCount = 0;
    size_t m_CompletedDMABufferCount = 0;
    size_t m_QueuedDMABufferCount = 0;
    DMATransferError m_DMATransferError = DMATransferError::None;

    volatile uint32_t m_PendingIRQFlags = 0;
    volatile WakeupReason m_WakeupReason = WakeupReason::None;
};

} // namespace
