// This file is part of PadOS.
//
// Copyright (c) 2020-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.05.2020 23:00:00

#include <algorithm>
#include <limits>
#include <numeric>

#include <string.h>
#include <sys/uio.h>

#include <Kernel/KTime.h>
#include <Kernel/KIRQGuard.h>
#include <Kernel/Drivers/STM32/SDMMCDriver_STM32.h>
#include <Kernel/SpinTimer.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Utils/Utils.h>


using namespace sdmmc;

namespace kernel
{

PREGISTER_KERNEL_DRIVER(SDMMCDriver_STM32, SDMMCDriverParameters);


static const uint32_t SDMMC_EVENT_FLAGS = SDMMC_MASK_CMDRENDIE      // Command Response Received Interrupt Enable
                                        | SDMMC_MASK_CMDSENTIE      // Command Sent Interrupt Enable
                                        | SDMMC_MASK_DATAENDIE      // Data End Interrupt Enable
                                        | SDMMC_MASK_DHOLDIE        // Data Hold Interrupt Enable
                                        | SDMMC_MASK_DBCKENDIE      // Data Block End Interrupt Enable
                                        //| SDMMC_MASK_DABORTIE       // Data transfer aborted interrupt enable
                                        | SDMMC_MASK_TXFIFOHEIE     // Tx FIFO Half Empty interrupt Enable
                                        | SDMMC_MASK_RXFIFOHFIE     // Rx FIFO Half Full interrupt Enable
                                        | SDMMC_MASK_RXFIFOFIE      // Rx FIFO Full interrupt Enable
                                        | SDMMC_MASK_TXFIFOEIE      // Tx FIFO Empty interrupt Enable
                                        | SDMMC_MASK_BUSYD0ENDIE    // BUSYD0ENDIE interrupt Enable
                                        | SDMMC_MASK_SDIOITIE       // SDMMC Mode Interrupt Received interrupt Enable
                                        | SDMMC_MASK_VSWENDIE       // Voltage switch critical timing section completion Interrupt Enable
                                        | SDMMC_MASK_CKSTOPIE       // Voltage Switch clock stopped Interrupt Enable
                                        | SDMMC_MASK_IDMABTCIE;     // IDMA buffer transfer complete Interrupt Enable

static constexpr uint32_t SDMMC_ICR_ALL_FLAGS = 
      SDMMC_ICR_CCRCFAILC
    | SDMMC_ICR_DCRCFAILC
    | SDMMC_ICR_CTIMEOUTC
    | SDMMC_ICR_DTIMEOUTC
    | SDMMC_ICR_TXUNDERRC
    | SDMMC_ICR_RXOVERRC
    | SDMMC_ICR_CMDRENDC
    | SDMMC_ICR_CMDSENTC
    | SDMMC_ICR_DATAENDC
    | SDMMC_ICR_DHOLDC
    | SDMMC_ICR_DBCKENDC
    | SDMMC_ICR_DABORTC
    | SDMMC_ICR_BUSYD0ENDC
    | SDMMC_ICR_SDIOITC
    | SDMMC_ICR_ACKFAILC
    | SDMMC_ICR_ACKTIMEOUTC
    | SDMMC_ICR_VSWENDC
    | SDMMC_ICR_CKSTOPC
    | SDMMC_ICR_IDMATEC
    | SDMMC_ICR_IDMABTCC;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::SDMMCDriver_STM32(const SDMMCDriverParameters& parameters)
    : SDMMCDriver(parameters, TRANSFER_BUFFER_SIZE)
    , m_IRQ(get_sdmmc_irq(parameters.PortID))
{
    m_PeripheralClockFrequency = parameters.ClockFrequency;
    m_ClockCap = parameters.ClockCap;
    m_SDMMC = get_sdmmc_from_id(parameters.PortID);

    DigitalPin(parameters.PinD0.PINID).SetPullMode(PinPullMode_e::Up);
    DigitalPin(parameters.PinD1.PINID).SetPullMode(PinPullMode_e::Up);
    DigitalPin(parameters.PinD2.PINID).SetPullMode(PinPullMode_e::Up);
    DigitalPin(parameters.PinD3.PINID).SetPullMode(PinPullMode_e::Up);
    DigitalPin(parameters.PinCMD.PINID).SetPullMode(PinPullMode_e::Up);
    DigitalPin(parameters.PinCK.PINID).SetPullMode(PinPullMode_e::Up);

    DigitalPin(parameters.PinD0.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(parameters.PinD1.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(parameters.PinD2.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(parameters.PinD3.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(parameters.PinCMD.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(parameters.PinCK.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);

    DigitalPin::ActivatePeripheralMux(parameters.PinD0);
    DigitalPin::ActivatePeripheralMux(parameters.PinD1);
    DigitalPin::ActivatePeripheralMux(parameters.PinD2);
    DigitalPin::ActivatePeripheralMux(parameters.PinD3);
    DigitalPin::ActivatePeripheralMux(parameters.PinCMD);
    DigitalPin::ActivatePeripheralMux(parameters.PinCK);

    SetClockFrequency(SDMMC_CLOCK_INIT);
    m_SDMMC->POWER = 3 << SDMMC_POWER_PWRCTRL_Pos;

    register_irq_handler(m_IRQ, IRQCallback, DeferredIRQCallback, this, KIRQ_PRI_LOW_LATENCY3);

    Start_trw(KSpawnThreadFlag::None, PThreadDetachState_Detached);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::~SDMMCDriver_STM32()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SDMMCDriver_STM32::Read(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position)
{
    const TransferRequest request = PrepareTransferRequest(file, segments, segmentCount, position);
    if (request.Length == 0) {
        return 0;
    }

    IOVectorCursor cursor(segments, segmentCount, request.Length);
    off64_t transferPosition = request.Position;

    KScopedLock deviceLock(m_DeviceMutex);
    KUniqueLock cardStateLock(m_Mutex, std::defer_lock);
    if (request.LockCardState) {
        cardStateLock.lock();
    }
    if (!IsReady()) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }

    while (cursor.RemainingLength != 0)
    {
        IOVectorCursor nextCursor = cursor;
        IOVectorCursor transfer = PrepareDirectTransfer(nextCursor, false);
        const bool useTransferBuffer = transfer.RemainingLength == 0;
        iovec_t transferBufferSegment;

        if (useTransferBuffer)
        {
            const size_t transferLength = std::min(cursor.RemainingLength, TRANSFER_BUFFER_SIZE);
            transferBufferSegment = { .iov_base = m_CacheAlignedBuffer, .iov_len = transferLength };
            transfer = IOVectorCursor(&transferBufferSegment, 1, transferLength);
        }

        ReadBlocks(static_cast<uint32_t>(transferPosition / BLOCK_SIZE), transfer);

        if (useTransferBuffer)
        {
            cursor.CopyFrom(m_CacheAlignedBuffer, transfer.RemainingLength);
            cursor.Advance(transfer.RemainingLength);
        }
        else
        {
            cursor = nextCursor;
        }
        transferPosition += transfer.RemainingLength;
    }
    return request.Length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SDMMCDriver_STM32::Write(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position)
{
    const TransferRequest request = PrepareTransferRequest(file, segments, segmentCount, position);
    if (request.Length == 0) {
        return 0;
    }

    IOVectorCursor cursor(segments, segmentCount, request.Length);
    off64_t transferPosition = request.Position;

    KScopedLock deviceLock(m_DeviceMutex);
    KUniqueLock cardStateLock(m_Mutex, std::defer_lock);
    if (request.LockCardState) {
        cardStateLock.lock();
    }
    if (!IsReady()) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }

    while (cursor.RemainingLength != 0)
    {
        IOVectorCursor nextCursor = cursor;
        IOVectorCursor transfer = PrepareDirectTransfer(nextCursor, true);
        const bool useTransferBuffer = transfer.RemainingLength == 0;
        iovec_t transferBufferSegment;

        if (useTransferBuffer)
        {
            const size_t transferLength = std::min(cursor.RemainingLength, TRANSFER_BUFFER_SIZE);
            cursor.CopyTo(m_CacheAlignedBuffer, transferLength);
            transferBufferSegment = { .iov_base = m_CacheAlignedBuffer, .iov_len = transferLength };
            transfer = IOVectorCursor(&transferBufferSegment, 1, transferLength);
        }

        WriteBlocks(static_cast<uint32_t>(transferPosition / BLOCK_SIZE), transfer);
        if (useTransferBuffer) {
            cursor.Advance(transfer.RemainingLength);
        } else {
            cursor = nextCursor;
        }
        transferPosition += transfer.RemainingLength;
    }
    return request.Length;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Send a command
///
/// \param extraCmdRFlags   Extra CMD register bit to use for this command
/// \param cmd              Command definition
/// \param arg              Argument of the command
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::ExecuteCmd(uint32_t extraCmdRFlags, uint32_t cmd, uint32_t arg)
{
    uint32_t commandR = extraCmdRFlags | (SDMMC_CMD_GET_INDEX(cmd) << SDMMC_CMD_CMDINDEX_Pos) | SDMMC_CMD_CPSMEN;

    uint32_t response;

    uint32_t interrupts = SDMMC_MASK_CTIMEOUTIE;

    if (cmd & SDMMC_RESP_PRESENT)
    {
        m_SDMMC->DTIMER = 0xffffffff;
        if (cmd & SDMMC_RESP_136)
        {
            response = 3; // Long response, expect CMDREND or CCRCFAIL flag
            interrupts |= SDMMC_MASK_CCRCFAILIE;
        }
        else if (cmd & SDMMC_RESP_CRC)
        {
            response = 1; // Short response, expect CMDREND or CCRCFAIL flag
            interrupts |= SDMMC_MASK_CCRCFAILIE;
        }
        else
        {
            response = 2; // Short response, expect CMDREND flag (No CRC)
        }
        interrupts |= SDMMC_MASK_CMDRENDIE; // ACKFAILIE | ACKTIMEOUTIE
    }
    else
    {
        response = 0; // No response, expect CMDSENT flag
        interrupts |= SDMMC_MASK_CMDSENTIE;
    }
    commandR |= response << SDMMC_CMD_WAITRESP_Pos;

    uint32_t enabledInterrupts = interrupts;
    if ((cmd & SDMMC_RESP_BUSY) != 0) {
        enabledInterrupts |= SDMMC_MASK_BUSYD0ENDIE;
    }
    if ((extraCmdRFlags & SDMMC_CMD_CMDTRANS) != 0)
    {
        enabledInterrupts |= DATA_IRQ_FLAGS;
        if (m_DMABufferCount != 0) {
            enabledInterrupts |= SDMMC_MASK_IDMABTCIE;
        }
    }
    {
        KIRQGuard irqGuard(m_IRQ);
        m_SDMMC->MASK = 0;
        m_PendingIRQFlags = 0;
        m_IOError = 0;
        m_WakeupReason = WakeupReason::None;
        m_SDMMC->ICR = SDMMC_ICR_ALL_FLAGS;
        m_SDMMC->ARG = arg;
        m_SDMMC->MASK = enabledInterrupts;
        m_SDMMC->CMD = commandR;
    }

    if (!WaitIRQ(interrupts))
    {
        if ((m_IOError & SDMMC_STA_CTIMEOUT) != 0 && m_IOError != ~uint32_t(0)) {
            RestartCard();
        }
        return false;
    }
    if ((cmd & SDMMC_RESP_BUSY) != 0 && (m_SDMMC->STA & SDMMC_STA_BUSYD0) != 0)
    {
        if (!WaitIRQ(SDMMC_MASK_BUSYD0ENDIE | SDMMC_MASK_CTIMEOUTIE))
        {
            if ((m_IOError & SDMMC_STA_CTIMEOUT) != 0 && m_IOError != ~uint32_t(0)) {
                RestartCard();
            }
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::SendCmd(uint32_t cmd, uint32_t arg)
{
    m_SDMMC->DLEN = 0;
    return ExecuteCmd(0, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t SDMMCDriver_STM32::GetResponse()
{
    return m_SDMMC->RESP1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::GetResponse128(uint8_t* response)
{
    for (int i = 0; i < 4; ++i)
    {
        uint32_t response32 = (&m_SDMMC->RESP1)[i];
        *response++ = uint8_t((response32 >> 24) & 0xff);
        *response++ = uint8_t((response32 >> 16) & 0xff);
        *response++ = uint8_t((response32 >> 8) & 0xff);
        *response++ = uint8_t((response32 >> 0) & 0xff);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint32_t blockCount, void* buffer)
{
    const size_t blockSize = size_t(1) << blockSizePower;
    const size_t transferLength = blockSize * blockCount;
    const bool isWrite = (cmd & SDMMC_CMD_WRITE) != 0;
    const uintptr_t addressAlignmentMask = isWrite ? sizeof(uint32_t) - 1 : DCACHE_LINE_SIZE_MASK;
    const bool useTransferBuffer =
        buffer != m_CacheAlignedBuffer
        && ((reinterpret_cast<uintptr_t>(buffer) & addressAlignmentMask) != 0
            || (!isWrite && (transferLength & DCACHE_LINE_SIZE_MASK) != 0));

    if (useTransferBuffer && transferLength > TRANSFER_BUFFER_SIZE)
    {
        set_last_error(EINVAL);
        return false;
    }

    void* dmaBuffer = useTransferBuffer ? m_CacheAlignedBuffer : buffer;
    if (useTransferBuffer && isWrite) {
        memmove(dmaBuffer, buffer, transferLength);
    }

    const iovec_t segment = { .iov_base = dmaBuffer, .iov_len = transferLength };
    const bool result = StartDataTransfer(cmd, arg, blockSizePower, blockCount, IOVectorCursor(&segment, 1, transferLength));

    if (result && useTransferBuffer && !isWrite) {
        memmove(buffer, dmaBuffer, transferLength);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::StartDataTransfer(
    uint32_t cmd,
    uint32_t arg,
    uint32_t blockSizePower,
    uint32_t blockCount,
    const IOVectorCursor& transfer)
{
    const size_t blockSize = size_t(1) << blockSizePower;
    const size_t byteLength = blockSize * blockCount;

    if (transfer.SegmentIndex >= transfer.SegmentCount || transfer.RemainingLength != byteLength
        || byteLength == 0 || byteLength > MAX_DATA_TRANSFER_SIZE)
    {
        set_last_error(EINVAL);
        return false;
    }

    const size_t segmentCount = transfer.SegmentCount - transfer.SegmentIndex;
    uint32_t dataControl = (blockSizePower << SDMMC_DCTRL_DBLOCKSIZE_Pos);
    const size_t dmaBufferSize = (segmentCount > 1) ? GetDMABufferSize(transfer) : 0;
    if ((cmd & SDMMC_CMD_WRITE) == 0) {
        dataControl |= SDMMC_DCTRL_DTDIR; // From card to host (Read).
    }
    IOVectorCursor cursor = transfer;
    while (cursor.RemainingLength != 0)
    {
        const size_t length = cursor.GetCurrentLength();
        const uintptr_t bufferAddress = reinterpret_cast<uintptr_t>(cursor.GetCurrentAddress());
        const uintptr_t cacheAddress = align_down(bufferAddress, DCACHE_LINE_SIZE);
        const size_t cacheLength = align_up(bufferAddress + length, DCACHE_LINE_SIZE) - cacheAddress;

        if ((cmd & SDMMC_CMD_WRITE) != 0) {
            SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(cacheAddress), cacheLength);
        } else {
            SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(cacheAddress), cacheLength);
        }
        cursor.Advance(length);
    }
    if (cmd & SDMMC_CMD_SDIO_BYTE)
    {
        dataControl |= 1 << SDMMC_DCTRL_DTMODE_Pos; // SDIO multibyte data transfer.
    }
    else
    {
        if (cmd & SDMMC_CMD_SDIO_BLOCK) {
            dataControl |= 0 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending on block count.
        } else if (cmd & SDMMC_CMD_STREAM) {
            dataControl |= 2 << SDMMC_DCTRL_DTMODE_Pos; // eMMC Stream data transfer. (WIDBUS shall select 1-bit wide bus mode)
        } else if (cmd & SDMMC_CMD_SINGLE_BLOCK) {
            dataControl |= 0 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending on block count.
        } else if (cmd & SDMMC_CMD_MULTI_BLOCK) {
            dataControl |= 0 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending on block count.
        } else {
            kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "StartAddressedDataTransCmd() invalid command flags: {:x}", cmd);
            return false;
        }
    }
    {
        KIRQGuard irqGuard(m_IRQ);
        m_SDMMC->MASK = 0;
        m_TransferSegments = transfer.Segments;
        m_TransferSegmentIndex = transfer.SegmentIndex;
        m_TransferSegmentOffset = transfer.SegmentOffset;
        m_DMABufferSize = dmaBufferSize;
        m_DMABufferCount = (dmaBufferSize != 0) ? byteLength / dmaBufferSize : 0;
        m_CompletedDMABufferCount = 0;
        m_QueuedDMABufferCount = 0;
        m_DMATransferError = DMATransferError::None;

        m_SDMMC->DTIMER = 0xffffffff;
        m_SDMMC->CLKCR |= SDMMC_CLKCR_HWFC_EN;

        uint32_t idmaControl = SDMMC_IDMA_IDMAEN;
        if (dmaBufferSize != 0)
        {
            m_SDMMC->IDMABASE0 = GetNextDMABufferAddress();
            m_SDMMC->IDMABASE1 = GetNextDMABufferAddress();
            m_SDMMC->IDMABSIZE = (dmaBufferSize / 32) << SDMMC_IDMABSIZE_IDMABNDT_Pos;
            idmaControl |= SDMMC_IDMA_IDMABMODE;
        }
        else
        {
            m_SDMMC->IDMABASE0 = reinterpret_cast<uintptr_t>(transfer.GetCurrentAddress());
        }
        m_SDMMC->DLEN = byteLength;
        m_SDMMC->DCTRL = dataControl;
        m_SDMMC->IDMACTRL = idmaControl;
    }

    bool result = ExecuteCmd(SDMMC_CMD_CMDTRANS, cmd, arg);
    if (result) {
        result = WaitIRQ(DATA_IRQ_FLAGS);
    } else {
        kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "SDMMC data command {} failed.", SDMMC_CMD_GET_INDEX(cmd));
    }

    {
        KIRQGuard irqGuard(m_IRQ);
        m_SDMMC->MASK = 0;
        m_SDMMC->CMD &= ~SDMMC_CMD_CMDTRANS;
        m_SDMMC->DLEN = 0;
        m_SDMMC->DCTRL = 0;
        m_SDMMC->IDMACTRL = 0;
        m_SDMMC->ICR = SDMMC_ICR_ALL_FLAGS;
        m_TransferSegments = nullptr;
        m_DMABufferCount = 0;
    }

    if (result && (cmd & SDMMC_CMD_WRITE) == 0)
    {
        cursor = transfer;
        while (cursor.RemainingLength != 0)
        {
            const size_t length = cursor.GetCurrentLength();
            const size_t cacheLength = (length + DCACHE_LINE_SIZE - 1) & ~DCACHE_LINE_SIZE_MASK;
            SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(cursor.GetCurrentAddress()), cacheLength);
            cursor.Advance(length);
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg)
{
    return ExecuteCmd(SDMMC_CMD_CMDSTOP, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Configures the driver with the selected card configuration
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::ApplySpeedAndBusWidth()
{

    if (m_HighSpeed) {
        m_SDMMC->CLKCR |= SDMMC_CLKCR_NEGEDGE;
    } else {
        m_SDMMC->CLKCR &= ~SDMMC_CLKCR_NEGEDGE;
    }

    SetClockFrequency(m_Clock);

    uint32_t CLKCR = m_SDMMC->CLKCR;
    CLKCR &= ~SDMMC_CLKCR_WIDBUS_Msk;

    switch (m_BusWidth)
    {
        case 1: CLKCR |= 0 << SDMMC_CLKCR_WIDBUS_Pos; break;
        case 4: CLKCR |= 1 << SDMMC_CLKCR_WIDBUS_Pos; break;
        case 8: CLKCR |= 2 << SDMMC_CLKCR_WIDBUS_Pos; break;
        default:
            kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "SDMMCDriver invalid bus width ({}) using 1-bit.", m_BusWidth);
            break;
    }
    m_SDMMC->CLKCR = CLKCR;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::IOVectorCursor::IOVectorCursor(const iovec_t* segments, size_t segmentCount, size_t length)
    : Segments(segments)
    , SegmentCount(segmentCount)
    , RemainingLength(length)
{
    Normalize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::IOVectorCursor::Normalize()
{
    while (SegmentIndex < SegmentCount && SegmentOffset == Segments[SegmentIndex].iov_len)
    {
        ++SegmentIndex;
        SegmentOffset = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SDMMCDriver_STM32::IOVectorCursor::GetCurrentLength() const
{
    kassert(RemainingLength != 0);
    kassert(SegmentIndex < SegmentCount);
    return std::min(Segments[SegmentIndex].iov_len - SegmentOffset, RemainingLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t* SDMMCDriver_STM32::IOVectorCursor::GetCurrentAddress() const
{
    kassert(RemainingLength != 0);
    kassert(SegmentIndex < SegmentCount);
    return static_cast<uint8_t*>(Segments[SegmentIndex].iov_base) + SegmentOffset;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::IOVectorCursor::Advance(size_t length)
{
    kassert(length <= RemainingLength);

    while (length != 0)
    {
        const size_t advanceLength = std::min(length, GetCurrentLength());
        SegmentOffset += advanceLength;
        RemainingLength -= advanceLength;
        length -= advanceLength;
        Normalize();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::IOVectorCursor::CopyTo(void* destination, size_t length) const
{
    kassert(length <= RemainingLength);

    IOVectorCursor cursor = *this;
    uint8_t* output = static_cast<uint8_t*>(destination);

    while (length != 0)
    {
        const size_t copyLength = std::min(length, cursor.GetCurrentLength());
        memcpy(output, cursor.GetCurrentAddress(), copyLength);
        output += copyLength;
        length -= copyLength;
        cursor.Advance(copyLength);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::IOVectorCursor::CopyFrom(const void* source, size_t length) const
{
    kassert(length <= RemainingLength);

    IOVectorCursor cursor = *this;
    const uint8_t* input = static_cast<const uint8_t*>(source);

    while (length != 0)
    {
        const size_t copyLength = std::min(length, cursor.GetCurrentLength());
        memcpy(cursor.GetCurrentAddress(), input, copyLength);
        input += copyLength;
        length -= copyLength;
        cursor.Advance(copyLength);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::TransferRequest SDMMCDriver_STM32::PrepareTransferRequest(
    const Ptr<KFileNode>& file,
    const iovec_t* segments,
    size_t segmentCount,
    off64_t position) const
{
    if (position < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    TransferRequest request;
    request.Position = position;

    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
    {
        if (segments[segmentIndex].iov_len > std::numeric_limits<size_t>::max() - request.Length) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        request.Length += segments[segmentIndex].iov_len;
    }
    if (request.Length == 0) {
        return request;
    }

    if (file != nullptr)
    {
        const Ptr<SDMMCInode> inode = ptr_static_cast<SDMMCInode>(file->GetInode());
        request.LockCardState = true;

        if (position >= inode->bi_nSize)
        {
            request.Length = 0;
            return request;
        }
        if (request.Length > static_cast<size_t>(inode->bi_nSize - position)) {
            request.Length = static_cast<size_t>(inode->bi_nSize - position);
        }

        if (inode->bi_nStart > std::numeric_limits<off64_t>::max() - request.Position) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        request.Position += inode->bi_nStart;
    }

    if ((request.Position % BLOCK_SIZE) != 0 || (request.Length % BLOCK_SIZE) != 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (request.Length != 0
        && request.Length - 1 > static_cast<size_t>(std::numeric_limits<off64_t>::max() - request.Position))
    {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    if (request.Length != 0
        && (request.Position + request.Length - 1) / BLOCK_SIZE > std::numeric_limits<uint32_t>::max())
    {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    return request;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::IOVectorCursor SDMMCDriver_STM32::PrepareDirectTransfer(IOVectorCursor& cursor, bool isWrite) const
{
    const uintptr_t addressAlignmentMask = isWrite ? sizeof(uint32_t) - 1 : DCACHE_LINE_SIZE_MASK;
    IOVectorCursor transfer = cursor;
    transfer.SegmentCount = cursor.SegmentIndex;
    transfer.RemainingLength = 0;
    size_t remainingLength = MAX_DATA_TRANSFER_SIZE;

    // Advance the working cursor so the caller can retain the endpoint after successful I/O.
    while (cursor.RemainingLength != 0 && remainingLength != 0)
    {
        const uintptr_t address = reinterpret_cast<uintptr_t>(cursor.GetCurrentAddress());
        size_t length = std::min(cursor.GetCurrentLength(), remainingLength);
        length -= length % BLOCK_SIZE;
        if (length == 0 || (address & addressAlignmentMask) != 0) {
            break;
        }
        transfer.SegmentCount = cursor.SegmentIndex + 1;
        transfer.RemainingLength += length;
        remainingLength -= length;
        cursor.Advance(length);

        // A partial segment or skipped empty entries end the transaction.
        if (cursor.SegmentIndex != transfer.SegmentCount) {
            break;
        }
    }
    return transfer;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SDMMCDriver_STM32::GetDMABufferSize(const IOVectorCursor& transfer)
{
    IOVectorCursor cursor = transfer;
    size_t commonLength = 0;
    while (cursor.RemainingLength != 0)
    {
        const size_t length = cursor.GetCurrentLength();
        commonLength = std::gcd(commonLength, length);
        cursor.Advance(length);
    }
    size_t bufferSize = std::min(commonLength, MAX_IDMA_BUFFER_SIZE);
    while ((commonLength % bufferSize) != 0) {
        bufferSize -= BLOCK_SIZE;
    }
    return bufferSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uintptr_t SDMMCDriver_STM32::GetNextDMABufferAddress()
{
    const iovec_t& segment = m_TransferSegments[m_TransferSegmentIndex];
    const uintptr_t address = reinterpret_cast<uintptr_t>(segment.iov_base) + m_TransferSegmentOffset;
    m_TransferSegmentOffset += m_DMABufferSize;
    if (m_TransferSegmentOffset == segment.iov_len)
    {
        ++m_TransferSegmentIndex;
        m_TransferSegmentOffset = 0;
    }
    ++m_QueuedDMABufferCount;
    return address;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::ReadBlocks(uint32_t firstBlock, const IOVectorCursor& transfer)
{
    const uint32_t blockCount = static_cast<uint32_t>(transfer.RemainingLength / BLOCK_SIZE);
    const uint32_t cmd = (blockCount > 1) ? SDMMC_CMD18_READ_MULTIPLE_BLOCK : SDMMC_CMD17_READ_SINGLE_BLOCK;

    for (int retry = 0; retry < 10; ++retry)
    {
        if (!Cmd13_sdmmc()) {
            continue;
        }

        uint32_t start = firstBlock;
        if ((m_CardType & SDMMCCardType::HC) == 0) {
            start *= BLOCK_SIZE;
        }

        if (!StartDataTransfer(cmd, start, get_first_bit_index(BLOCK_SIZE), blockCount, transfer)) {
            continue;
        }

        const uint32_t response = GetResponse();
        if ((response & CARD_STATUS_ERR_RD_WR) != 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "SDMMCDriver_STM32::Read() CMD{} response 0x{:08x} CARD_STATUS_ERR_RD_WR.", int(SDMMC_CMD_GET_INDEX(cmd)), response);
            continue;
        }

        // WORKAROUND for non-compliant cards: Ignore errors and retry CMD12 once.
        if (blockCount > 1 && !StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
            StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0);
        }
        return;
    }
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::WriteBlocks(uint32_t firstBlock, const IOVectorCursor& transfer)
{
    const uint32_t blockCount = static_cast<uint32_t>(transfer.RemainingLength / BLOCK_SIZE);
    const uint32_t cmd = (blockCount > 1) ? SDMMC_CMD25_WRITE_MULTIPLE_BLOCK : SDMMC_CMD24_WRITE_BLOCK;

    for (int retry = 0; retry < 10; ++retry)
    {
        uint32_t start = firstBlock;
        if ((m_CardType & SDMMCCardType::HC) == 0) {
            start *= BLOCK_SIZE;
        }

        if (!StartDataTransfer(cmd, start, get_first_bit_index(BLOCK_SIZE), blockCount, transfer))
        {
            kernel_log<PLogSeverity::INFO_HIGH_VOL>(
                LogCategorySDMMCDriver,
                "SDMMCDriver_STM32::Write() attempt {} CMD{} start {} blocks {} segments {} failed during the data-transfer phase (error {}).",
                retry + 1,
                SDMMC_CMD_GET_INDEX(cmd),
                start,
                blockCount,
                transfer.SegmentCount - transfer.SegmentIndex,
                get_last_error());
            continue;
        }

        const uint32_t response = GetResponse();
        if ((response & CARD_STATUS_ERR_RD_WR) != 0)
        {
            kernel_log<PLogSeverity::INFO_HIGH_VOL>(
                LogCategorySDMMCDriver,
                "SDMMCDriver_STM32::Write() attempt {} CMD{} response {:08x} reports write errors {:08x}.",
                retry + 1,
                SDMMC_CMD_GET_INDEX(cmd),
                response,
                response & CARD_STATUS_ERR_RD_WR);
            kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "SDMMCDriver_STM32::Write() CMD{} response 0x{:08x} CARD_STATUS_ERR_RD_WR.", int(SDMMC_CMD_GET_INDEX(cmd)), response);
            continue;
        }

        // SPI multi-block writes terminate using a special token, not CMD12.
        if (blockCount > 1 && !StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0))
        {
            const int stopError = get_last_error();
            const uint32_t stopResponse = GetResponse();
            kernel_log<PLogSeverity::INFO_HIGH_VOL>(
                LogCategorySDMMCDriver,
                "SDMMCDriver_STM32::Write() attempt {} CMD12 failed after CMD{} start {} blocks {} (error {}, response {:08x}).",
                retry + 1,
                SDMMC_CMD_GET_INDEX(cmd),
                start,
                blockCount,
                stopError,
                stopResponse);
            continue;
        }
        return;
    }
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SDMMCDriver_STM32::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<SDMMCDriver_STM32*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::DeferredIRQCallback(IRQn_Type irq, void* userData)
{
    static_cast<SDMMCDriver_STM32*>(userData)->HandleDeferredIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SDMMCDriver_STM32::HandleIRQ()
{
    // IDMATE has no separate interrupt mask bit; inspect it when servicing the accompanying SDMMC event.
    const uint32_t status = m_SDMMC->STA & (m_SDMMC->MASK | SDMMC_STA_IDMATE);
    if (status == 0) {
        return IRQResult::UNHANDLED;
    }

    const uint32_t errorFlags = status & ~SDMMC_EVENT_FLAGS;
    if (errorFlags != 0)
    {
        m_SDMMC->MASK = 0;
        m_SDMMC->ICR = status;
        m_IOError = errorFlags;
        m_WakeupReason = WakeupReason::Error;
        return IRQResult::HANDLED_DEFERRED;
    }

    if ((status & SDMMC_STA_IDMABTC) != 0)
    {
        const DMATransferError error = HandleDMABufferComplete();
        if (error != DMATransferError::None) {
            return FailDMATransfer(error);
        }
    }
    if ((status & SDMMC_STA_DATAEND) != 0)
    {
        // A sticky completion flag can hide multiple switches. Never accept a short completion count.
        if (m_DMABufferCount != 0 && m_CompletedDMABufferCount != m_DMABufferCount) {
            return FailDMATransfer(DMATransferError::CompletionCountMismatch);
        }
        m_SDMMC->MASK = 0;
        m_SDMMC->CMD &= ~SDMMC_CMD_CMDTRANS;
        m_WakeupReason = WakeupReason::DataComplete;
    }

    const uint32_t completedEvents = status & ~SDMMC_STA_IDMABTC;
    if (completedEvents != 0)
    {
        m_SDMMC->ICR = completedEvents;
        m_SDMMC->MASK &= ~completedEvents;
        m_PendingIRQFlags = m_PendingIRQFlags | completedEvents;
        if ((status & SDMMC_STA_DATAEND) == 0) {
            m_WakeupReason = WakeupReason::Event;
        }
        return IRQResult::HANDLED_DEFERRED;
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::HandleDeferredIRQ()
{
    // The waiter checks retained status, so coalesced or stale wakeups cannot lose a completion.
    m_IOCondition.Wakeup(0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::DMATransferError SDMMCDriver_STM32::HandleDMABufferComplete()
{
    m_SDMMC->ICR = SDMMC_ICR_IDMABTCC;
    if (m_CompletedDMABufferCount >= m_DMABufferCount) {
        return DMATransferError::CompletionCountMismatch;
    }

    const uint32_t expectedActiveBuffer = ((m_CompletedDMABufferCount % 2) == 0) ? SDMMC_IDMA_IDMABACT : 0;
    if ((m_SDMMC->IDMACTRL & SDMMC_IDMA_IDMABACT) != expectedActiveBuffer) {
        return DMATransferError::UnexpectedBuffer;
    }
    ++m_CompletedDMABufferCount;

    if (m_QueuedDMABufferCount < m_DMABufferCount)
    {
        volatile uint32_t* const baseRegister = (expectedActiveBuffer != 0) ? &m_SDMMC->IDMABASE0 : &m_SDMMC->IDMABASE1;
        const uintptr_t address = GetNextDMABufferAddress();
        *baseRegister = address;
        // Writes to the active buffer are discarded by hardware.
        if (*baseRegister != address) {
            return DMATransferError::AddressUpdateRejected;
        }
    }
    return DMATransferError::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SDMMCDriver_STM32::FailDMATransfer(DMATransferError error)
{
    m_SDMMC->MASK = 0;
    m_DMATransferError = error;
    m_IOError = ~uint32_t(0);
    m_WakeupReason = WakeupReason::Error;

    // Use the allocation-free panic overload; transfer counters remain available to the debugger.
    switch (error)
    {
        case DMATransferError::UnexpectedBuffer:
            panic("SDMMC: unexpected active IDMA buffer.");
            break;
        case DMATransferError::AddressUpdateRejected:
            panic("SDMMC: IDMA buffer address update missed its deadline.");
            break;
        case DMATransferError::CompletionCountMismatch:
            panic("SDMMC: IDMA buffer completion count mismatch.");
            break;
        default:
            panic("SDMMC: invalid IDMA transfer state.");
            break;
    }
    return IRQResult::HANDLED_DEFERRED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::WaitIRQ(uint32_t flags)
{
    const TimeValNanos deadline = kget_monotonic_time() + TimeValNanos::FromMilliseconds(500);
    PErrorCode waitResult = PErrorCode::Success;
    {
        KIRQGuard irqGuard(m_IRQ);
        while ((m_PendingIRQFlags & flags) == 0 && m_IOError == 0)
        {
            waitResult = m_IOCondition.IRQWaitDeadline(irqGuard, deadline);
            if (waitResult != PErrorCode::Success && waitResult != PErrorCode::INTR)
            {
                m_SDMMC->MASK = 0;
                m_IOError = ~uint32_t(0);
                break;
            }
            waitResult = PErrorCode::Success;
        }
        m_PendingIRQFlags = m_PendingIRQFlags & ~flags;
    }

    if (waitResult != PErrorCode::Success)
    {
        set_last_error(waitResult);
        return false;
    }
    if (m_IOError != 0)
    {
        set_last_error(EIO);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Reset the SDMMC peripheral
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::Reset()
{
    RCC->AHB3RSTR |= RCC_AHB3RSTR_SDMMC1RST;
    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_SDMMC1RST;
    ApplySpeedAndBusWidth();
    m_SDMMC->POWER = 3 << SDMMC_POWER_PWRCTRL_Pos;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Set SDMMC clock frequency.
///
/// \param frequency    SDMMC clock frequency in Hz.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::SetClockFrequency(uint32_t frequency)
{
    if (m_ClockCap != 0 && frequency > m_ClockCap) frequency = m_ClockCap;

    const uint32_t divider = (m_PeripheralClockFrequency + (frequency * 2) - 1) / (frequency * 2);

    uint32_t CLKCR = m_SDMMC->CLKCR;
    CLKCR &= ~SDMMC_CLKCR_CLKDIV_Msk;
    CLKCR |= divider << SDMMC_CLKCR_CLKDIV_Pos;
    m_SDMMC->CLKCR = CLKCR;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::SendClock()
{
    uint32_t CLKCR = m_SDMMC->CLKCR;

    m_SDMMC->CLKCR &= ~SDMMC_CLKCR_PWRSAV;  // Disable power-save to make sure the clock is running.
    TimeValMicros delay = TimeValMicros::FromMicroseconds((TimeValMicros::TicksPerSecond * 74 + m_Clock - 1) / m_Clock);    // Sleep for at least 74 SDMMC clock cycles.
    if (delay < TimeValMicros::zero) delay = TimeValMicros::FromMicroseconds(1);
    SpinTimer::SleepuS(uint32_t(delay.AsMicroseconds()));

    m_SDMMC->CLKCR = CLKCR; // Restore power-save.
}

} // namespace kernel
