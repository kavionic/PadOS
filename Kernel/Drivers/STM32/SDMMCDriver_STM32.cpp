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
// Created: 20.05.2020 23:00:00

#include <algorithm>
#include <limits>

#include <string.h>
#include <sys/uio.h>

#include <Kernel/KTime.h>
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
                                        | SDMMC_MASK_CKSTOPIE;      // Voltage Switch clock stopped Interrupt Enable

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

SDMMCDriver_STM32::SDMMCDriver_STM32(const SDMMCDriverParameters& parameters) : SDMMCDriver(parameters, TRANSFER_BUFFER_SIZE)
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

    register_irq_handler(get_sdmmc_irq(parameters.PortID), IRQCallback, this);

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
        iovec_t transferSegments[2];
        size_t transferSegmentCount = PrepareDirectTransfer(cursor, transferSegments);
        size_t transferLength = 0;
        const bool useTransferBuffer = transferSegmentCount == 0;

        if (useTransferBuffer)
        {
            transferLength = std::min(cursor.RemainingLength, TRANSFER_BUFFER_SIZE);
            transferSegments[0].iov_base = m_CacheAlignedBuffer;
            transferSegments[0].iov_len = transferLength;
            transferSegmentCount = 1;
        }
        else
        {
            for (size_t segmentIndex = 0; segmentIndex < transferSegmentCount; ++segmentIndex) {
                transferLength += transferSegments[segmentIndex].iov_len;
            }
        }

        ReadBlocks(static_cast<uint32_t>(transferPosition / BLOCK_SIZE), transferSegments, transferSegmentCount);

        if (useTransferBuffer) {
            cursor.CopyFrom(m_CacheAlignedBuffer, transferLength);
        }
        cursor.Advance(transferLength);
        transferPosition += transferLength;
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
        iovec_t transferSegments[2];
        size_t transferSegmentCount = PrepareDirectTransfer(cursor, transferSegments);
        size_t transferLength = 0;

        if (transferSegmentCount == 0)
        {
            transferLength = std::min(cursor.RemainingLength, TRANSFER_BUFFER_SIZE);
            cursor.CopyTo(m_CacheAlignedBuffer, transferLength);
            transferSegments[0].iov_base = m_CacheAlignedBuffer;
            transferSegments[0].iov_len = transferLength;
            transferSegmentCount = 1;
        }
        else
        {
            for (size_t segmentIndex = 0; segmentIndex < transferSegmentCount; ++segmentIndex) {
                transferLength += transferSegments[segmentIndex].iov_len;
            }
        }

        WriteBlocks(static_cast<uint32_t>(transferPosition / BLOCK_SIZE), transferSegments, transferSegmentCount);
        cursor.Advance(transferLength);
        transferPosition += transferLength;
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

    m_SDMMC->ICR = SDMMC_ICR_ALL_FLAGS;
    m_SDMMC->ARG = arg;
    m_SDMMC->CMD = commandR;

    if ((extraCmdRFlags & SDMMC_CMD_CMDTRANS) != 0)
    {
        const TimeValNanos commandDeadline =
            kget_monotonic_time() + TimeValNanos::FromMilliseconds(500);
        uint32_t status;
        do
        {
            status = m_SDMMC->STA & interrupts;
        } while (status == 0 && kget_monotonic_time() < commandDeadline);

        m_SDMMC->ICR =
            SDMMC_ICR_CCRCFAILC
            | SDMMC_ICR_CTIMEOUTC
            | SDMMC_ICR_CMDRENDC
            | SDMMC_ICR_CMDSENTC;

        if (status == 0)
        {
            m_IOError = ~0L;
            set_last_error(PErrorCode::TIMEDOUT);
            return false;
        }

        const uint32_t commandErrorFlags =
            SDMMC_STA_CCRCFAIL | SDMMC_STA_CTIMEOUT;
        if ((status & commandErrorFlags) != 0)
        {
            m_IOError = status & commandErrorFlags;
            if ((status & SDMMC_STA_CTIMEOUT) != 0) {
                RestartCard();
            }
            set_last_error(EIO);
            return false;
        }
        m_IOError = 0;
    }
    else if (!WaitIRQ(interrupts))
    {
        if ((m_SDMMC->STA & SDMMC_STA_CTIMEOUT) != 0) {
            RestartCard();
        }
        return false;
    }
    if ((cmd & SDMMC_RESP_BUSY) && (m_SDMMC->STA & SDMMC_STA_BUSYD0))
    {
        if (!WaitIRQ(SDMMC_MASK_BUSYD0ENDIE | SDMMC_MASK_CTIMEOUTIE))
        {
            if ((m_SDMMC->STA & SDMMC_STA_CTIMEOUT) != 0) {
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
    const bool useTransferBuffer =
        buffer != m_CacheAlignedBuffer
        && ((reinterpret_cast<uintptr_t>(buffer) & DCACHE_LINE_SIZE_MASK) != 0
            || (transferLength & DCACHE_LINE_SIZE_MASK) != 0);

    if (useTransferBuffer && transferLength > TRANSFER_BUFFER_SIZE)
    {
        set_last_error(EINVAL);
        return false;
    }

    void* dmaBuffer = useTransferBuffer ? m_CacheAlignedBuffer : buffer;
    if (useTransferBuffer && (cmd & SDMMC_CMD_WRITE) != 0) {
        memmove(dmaBuffer, buffer, transferLength);
    }

    const iovec_t segment = { .iov_base = dmaBuffer, .iov_len = transferLength };
    const bool result = StartDataTransfer(cmd, arg, blockSizePower, blockCount, &segment, 1);

    if (result && useTransferBuffer && (cmd & SDMMC_CMD_WRITE) == 0) {
        memmove(buffer, dmaBuffer, transferLength);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::StartDataTransfer(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint32_t blockCount, const iovec_t* segments, size_t segmentCount)
{
    const size_t blockSize = size_t(1) << blockSizePower;
    const size_t byteLength = blockSize * blockCount;

    if (segmentCount == 0 || segmentCount > 2 || byteLength == 0 || byteLength > MAX_DATA_TRANSFER_SIZE)
    {
        set_last_error(EINVAL);
        return false;
    }

    size_t segmentLength = 0;
    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
    {
        const iovec_t& segment = segments[segmentIndex];
        if ((reinterpret_cast<uintptr_t>(segment.iov_base) & DCACHE_LINE_SIZE_MASK) != 0
            || ((segment.iov_len & DCACHE_LINE_SIZE_MASK) != 0 && segment.iov_base != m_CacheAlignedBuffer))
        {
            set_last_error(EINVAL);
            return false;
        }
        segmentLength += segment.iov_len;
    }
    if (segmentLength != byteLength)
    {
        set_last_error(EINVAL);
        return false;
    }
    if (segmentCount == 2
        && (segments[0].iov_len != segments[1].iov_len
            || segments[0].iov_len > MAX_IDMA_BUFFER_SIZE
            || (segments[0].iov_len % 32) != 0))
    {
        set_last_error(EINVAL);
        return false;
    }

    uint32_t dataControl = (blockSizePower << SDMMC_DCTRL_DBLOCKSIZE_Pos);
    uint32_t idmaControl;
    if ((cmd & SDMMC_CMD_WRITE) == 0) {
        dataControl |= SDMMC_DCTRL_DTDIR; // From card to host (Read).
    }
    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
    {
        const size_t cacheLength =
            (segments[segmentIndex].iov_len + DCACHE_LINE_SIZE - 1) & ~DCACHE_LINE_SIZE_MASK;
        uint32_t* const cacheAddress = reinterpret_cast<uint32_t*>(segments[segmentIndex].iov_base);

        if ((cmd & SDMMC_CMD_WRITE) != 0) {
            SCB_CleanDCache_by_Addr(cacheAddress, cacheLength);
        } else {
            SCB_InvalidateDCache_by_Addr(cacheAddress, cacheLength);
        }
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
    m_SDMMC->DTIMER = 0xffffffff;
    m_SDMMC->CLKCR |= SDMMC_CLKCR_HWFC_EN; // Hardware flow-control enabled.

    if (segmentCount == 2)
    {
        m_SDMMC->IDMABASE0 = reinterpret_cast<uintptr_t>(segments[0].iov_base);
        m_SDMMC->IDMABASE1 = reinterpret_cast<uintptr_t>(segments[1].iov_base);

        m_SDMMC->IDMABSIZE =
            ((segments[0].iov_len / 32) << SDMMC_IDMABSIZE_IDMABNDT_Pos)
            & SDMMC_IDMABSIZE_IDMABNDT_Msk;
        idmaControl = SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE;
    }
    else
    {
        m_SDMMC->IDMABASE0 = reinterpret_cast<uintptr_t>(segments[0].iov_base);
        idmaControl = SDMMC_IDMA_IDMAEN;
    }
    m_SDMMC->DLEN = byteLength;
    m_SDMMC->DCTRL = dataControl;
    m_SDMMC->CMD |= SDMMC_CMD_CMDTRANS;
    m_SDMMC->IDMACTRL = idmaControl;

    bool result = ExecuteCmd(SDMMC_CMD_CMDTRANS, cmd, arg);

    if (result) {
        result = WaitIRQ(
            SDMMC_MASK_DATAENDIE
            | SDMMC_MASK_DABORTIE
            | SDMMC_MASK_DTIMEOUTIE
            | SDMMC_MASK_DCRCFAILIE
            | SDMMC_MASK_TXUNDERRIE
            | SDMMC_MASK_RXOVERRIE);
    } else {
        kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "SDMMCDriver_STM32::StartDataTransfer() failed to start cmd {} ({})", arg, int(m_WakeupReason));
    }

    m_SDMMC->CMD &= ~SDMMC_CMD_CMDTRANS;
    m_SDMMC->DLEN = 0;
    m_SDMMC->DCTRL = 0;
    m_SDMMC->IDMACTRL = 0;
    m_SDMMC->ICR = SDMMC_ICR_ALL_FLAGS;
//    m_SDMMC->CLKCR &= ~SDMMC_CLKCR_HWFC_EN; // Hardware flow-control disabled.

    if (result && (cmd & SDMMC_CMD_WRITE) == 0)
    {
        for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
        {
            const size_t cacheLength =
                (segments[segmentIndex].iov_len + DCACHE_LINE_SIZE - 1) & ~DCACHE_LINE_SIZE_MASK;
            SCB_InvalidateDCache_by_Addr(
                reinterpret_cast<uint32_t*>(segments[segmentIndex].iov_base),
                cacheLength);
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

size_t SDMMCDriver_STM32::IOVectorCursor::GetRemainingSegmentCount(size_t maximumCount) const
{
    IOVectorCursor cursor = *this;
    size_t segmentCount = 0;

    while (cursor.RemainingLength != 0)
    {
        ++segmentCount;
        if (segmentCount > maximumCount) {
            break;
        }
        cursor.Advance(cursor.GetCurrentLength());
    }
    return segmentCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SDMMCDriver_STM32::IOVectorCursor::PeekSegments(iovec_t* segments, size_t segmentCount) const
{
    IOVectorCursor cursor = *this;
    size_t outputSegmentCount = 0;

    while (cursor.RemainingLength != 0 && outputSegmentCount < segmentCount)
    {
        const size_t segmentLength = cursor.GetCurrentLength();
        segments[outputSegmentCount].iov_base = cursor.GetCurrentAddress();
        segments[outputSegmentCount].iov_len = segmentLength;
        ++outputSegmentCount;
        cursor.Advance(segmentLength);
    }
    return outputSegmentCount;
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

size_t SDMMCDriver_STM32::PrepareDirectTransfer(const IOVectorCursor& cursor, iovec_t* transferSegments) const
{
    const size_t remainingSegmentCount = cursor.GetRemainingSegmentCount(2);
    if (remainingSegmentCount == 0 || remainingSegmentCount > 2) {
        return 0;
    }

    const size_t segmentCount = cursor.PeekSegments(transferSegments, remainingSegmentCount);
    kassert(segmentCount == remainingSegmentCount);

    if (segmentCount == 1)
    {
        transferSegments[0].iov_len = std::min(transferSegments[0].iov_len, MAX_DATA_TRANSFER_SIZE);
        transferSegments[0].iov_len -= transferSegments[0].iov_len % BLOCK_SIZE;

        if (transferSegments[0].iov_len != 0
            && (reinterpret_cast<uintptr_t>(transferSegments[0].iov_base) & DCACHE_LINE_SIZE_MASK) == 0) {
            return 1;
        }
    }
    else if (transferSegments[0].iov_len == transferSegments[1].iov_len
        && transferSegments[0].iov_len <= MAX_IDMA_BUFFER_SIZE
        && (transferSegments[0].iov_len % BLOCK_SIZE) == 0
        && (reinterpret_cast<uintptr_t>(transferSegments[0].iov_base) & DCACHE_LINE_SIZE_MASK) == 0
        && (reinterpret_cast<uintptr_t>(transferSegments[1].iov_base) & DCACHE_LINE_SIZE_MASK) == 0)
    {
        return 2;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::ReadBlocks(uint32_t firstBlock, const iovec_t* segments, size_t segmentCount)
{
    size_t transferLength = 0;
    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
        transferLength += segments[segmentIndex].iov_len;
    }

    const uint32_t blockCount = static_cast<uint32_t>(transferLength / BLOCK_SIZE);
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

        if (!StartDataTransfer(cmd, start, get_first_bit_index(BLOCK_SIZE), blockCount, segments, segmentCount)) {
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

void SDMMCDriver_STM32::WriteBlocks(uint32_t firstBlock, const iovec_t* segments, size_t segmentCount)
{
    size_t transferLength = 0;
    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
        transferLength += segments[segmentIndex].iov_len;
    }

    const uint32_t blockCount = static_cast<uint32_t>(transferLength / BLOCK_SIZE);
    const uint32_t cmd = (blockCount > 1) ? SDMMC_CMD25_WRITE_MULTIPLE_BLOCK : SDMMC_CMD24_WRITE_BLOCK;

    for (int retry = 0; retry < 10; ++retry)
    {
        uint32_t start = firstBlock;
        if ((m_CardType & SDMMCCardType::HC) == 0) {
            start *= BLOCK_SIZE;
        }

        if (!StartDataTransfer(cmd, start, get_first_bit_index(BLOCK_SIZE), blockCount, segments, segmentCount))
        {
            kernel_log<PLogSeverity::INFO_HIGH_VOL>(
                LogCategorySDMMCDriver,
                "SDMMCDriver_STM32::Write() attempt {} CMD{} start {} blocks {} segments {} failed during the data-transfer phase (error {}).",
                retry + 1,
                SDMMC_CMD_GET_INDEX(cmd),
                start,
                blockCount,
                segmentCount,
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

IRQResult SDMMCDriver_STM32::HandleIRQ()
{
    uint32_t status = m_SDMMC->STA & m_SDMMC->MASK;

    static constexpr uint32_t errorFlags = ~SDMMC_EVENT_FLAGS;

    if (status & errorFlags)
    {
        m_SDMMC->MASK = 0;
        m_IOError = status & errorFlags;
        m_WakeupReason = WakeupReason::Error;
        m_IOCondition.Wakeup(0);
    }
    else if (status & SDMMC_MASK_DATAENDIE)
    {
        m_SDMMC->ICR = SDMMC_ICR_DATAENDC;
        m_SDMMC->MASK = 0;
        m_SDMMC->CMD &= ~SDMMC_CMD_CMDTRANS;
        m_IOError = 0;
        m_WakeupReason = WakeupReason::DataComplete;
        m_IOCondition.Wakeup(0);
    }
    else if (status & SDMMC_EVENT_FLAGS)
    {
        m_SDMMC->MASK = 0;
        m_IOError = 0;
        m_WakeupReason = WakeupReason::Event;
        m_IOCondition.Wakeup(0);
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::WaitIRQ(uint32_t flags)
{
    static constexpr uint32_t errorFlags = ~SDMMC_EVENT_FLAGS;
    uint32_t status = m_SDMMC->STA & flags;

    if (status & errorFlags)
    {
        set_last_error(EIO);
        kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "{}: ERROR already flagged: {:x}", __PRETTY_FUNCTION__, status);
        return false;
    }
    if (status & flags) {
        return true;
    }

    m_WakeupReason = WakeupReason::None;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        m_SDMMC->MASK = flags;
        const PErrorCode result = m_IOCondition.IRQWaitTimeout(TimeValNanos::FromMilliseconds(500));
        while (result != PErrorCode::Success)
        {
            if (result != PErrorCode::INTR)
            {
                set_last_error(result);
                m_SDMMC->MASK = 0;
                m_IOError = ~0L; // get_last_error();
                break;
            }
        }
    } CRITICAL_END;
    if (m_IOError != 0)
    {
        //      Reset();
        if (m_IOError != ~0L)
        {
            if (m_IOError & SDMMC_STA_CTIMEOUT) {
                kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "{}: ERROR SDMMC_STA_CTIMEOUT", __PRETTY_FUNCTION__);
            }
            if (m_IOError & SDMMC_STA_DTIMEOUT) {
                kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "{}: ERROR SDMMC_STA_DTIMEOUT", __PRETTY_FUNCTION__);
            }
            if (m_IOError & SDMMC_STA_CCRCFAIL) {
                kernel_log<PLogSeverity::ERROR>(LogCategorySDMMCDriver, "{}: ERROR SDMMC_STA_CCRCFAIL", __PRETTY_FUNCTION__);
            }
            set_last_error(EIO);
        }
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
