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

#include <malloc.h>
#include <string.h>

#include <Kernel/Drivers/STM32/SDMMCDriver_STM32.h>
#include <Kernel/SpinTimer.h>
#include <Kernel/VFS/FileIO.h>

using namespace kernel;
using namespace os;
using namespace sdmmc;

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

SDMMCDriver_STM32::SDMMCDriver_STM32()
{
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

bool SDMMCDriver_STM32::Setup(const os::String& devicePath, SDMMC_TypeDef* port, uint32_t peripheralClockFrequency, uint32_t clockCap, DigitalPinID pinCD, IRQn_Type irqNum)
{
    m_PeripheralClockFrequency = peripheralClockFrequency;
    m_ClockCap = clockCap;
    m_SDMMC = port;

    SetClockFrequency(SDMMC_CLOCK_INIT);
    m_SDMMC->POWER = 3 << SDMMC_POWER_PWRCTRL_Pos;

    if (!SetupBase(devicePath, pinCD)) return false;
    kernel::register_irq_handler(irqNum, IRQCallback, this);
    return true;
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

    if (!WaitIRQ(interrupts))
    {
        if (m_SDMMC->STA && SDMMC_STA_CTIMEOUT) {
            RestartCard();
        }
        return false;
    }
    if ((cmd & SDMMC_RESP_BUSY) && (m_SDMMC->STA & SDMMC_STA_BUSYD0))
    {
        if (!WaitIRQ(SDMMC_MASK_BUSYD0ENDIE | SDMMC_MASK_CTIMEOUTIE))
        {
            if (m_SDMMC->STA && SDMMC_STA_CTIMEOUT) {
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

bool SDMMCDriver_STM32::StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint32_t blockCount, const os::IOSegment* segments, size_t segmentCount)
{
    const uint32_t blockSize = 1 << blockSizePower;
    const uint32_t byteLength = blockSize * blockCount;

    void* dmaTarget = nullptr;

    uint32_t dataControl = (blockSizePower << SDMMC_DCTRL_DBLOCKSIZE_Pos);
    if ((cmd & SDMMC_CMD_WRITE) == 0) {
        dataControl |= SDMMC_DCTRL_DTDIR; // From card to host (Read).
    }
    m_TransferSegments  = segments;
    m_SegmentCount      = segmentCount;
    m_CurrentSegment    = 0;

    if (segmentCount == 1)
    {
        const void* buffer = segments[0].Buffer;
        dmaTarget = const_cast<void*>(buffer);
        if (cmd & SDMMC_CMD_WRITE)
        {
            uint32_t* cacheStart = reinterpret_cast<uint32_t*>(intptr_t(dmaTarget) & ~DCACHE_LINE_SIZE_MASK);
            uint32_t  cacheLength = byteLength + intptr_t(dmaTarget) - intptr_t(cacheStart);
            cacheLength = ((cacheLength + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE;
            SCB_CleanDCache_by_Addr(cacheStart, cacheLength);
        }
        else
        {
            if ((intptr_t(buffer) & DCACHE_LINE_SIZE_MASK) || (byteLength & DCACHE_LINE_SIZE_MASK))
            {
                if (byteLength <= BLOCK_SIZE)
                {
                    dmaTarget = m_CacheAlignedBuffer;
                }
                else
                {
                    kprintf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() called with unaligned buffer or size larger than 512 bytes.\n");
                    set_last_error(EINVAL);
                    return false;
                }
            }
        }
    }
    else
    {
        if (cmd & SDMMC_CMD_WRITE)
        {
            for (size_t i = 0; i < segmentCount; ++i)
            {
                if ((intptr_t(segments[i].Buffer) & DCACHE_LINE_SIZE_MASK) != 0)
                {
                    kprintf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() multi segment write with unaligned buffer.\n");
                    set_last_error(EINVAL);
                    return false;
                }
                SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(segments[i].Buffer), segments[i].Length);
            }
        }
        else
        {
            for (size_t i = 0; i < segmentCount; ++i)
            {
                if ((intptr_t(segments[i].Buffer) & DCACHE_LINE_SIZE_MASK) != 0)
                {
                    kprintf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() multi segment write with unaligned buffer.\n");
                    set_last_error(EINVAL);
                    return false;
                }
            }

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
            dataControl |= 3 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending with STOP_TRANSMISSION command (not to be used with DTEN initiated data transfers).
            //dataControl |= 0 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending with STOP_TRANSMISSION command (not to be used with DTEN initiated data transfers).
        } else {
            kprintf("ERROR: StartAddressedDataTransCmd() invalid command flags: %lx\n", cmd);
            return false;
        }
    }
    m_SDMMC->DTIMER = 0xffffffff;
    m_SDMMC->CLKCR |= SDMMC_CLKCR_HWFC_EN; // Hardware flow-control enabled.

    if (dmaTarget == nullptr && m_SegmentCount > 1)
    {
        m_SDMMC->IDMABASE0 = intptr_t(m_TransferSegments[0].Buffer);
        m_SDMMC->IDMABASE1 = intptr_t(m_TransferSegments[1].Buffer);

        m_SDMMC->IDMABSIZE = ((blockSize / 32) << SDMMC_IDMABSIZE_IDMABNDT_Pos) & SDMMC_IDMABSIZE_IDMABNDT_Msk;
        m_SDMMC->IDMACTRL = SDMMC_IDMA_IDMAEN | SDMMC_IDMA_IDMABMODE;
        m_CurrentSegment = 2;
    }
    else
    {
        m_SDMMC->IDMABASE0 = (dmaTarget != nullptr) ? intptr_t(dmaTarget) : intptr_t(m_TransferSegments[0].Buffer);

        m_SDMMC->IDMACTRL = SDMMC_IDMA_IDMAEN;
        m_CurrentSegment = 1;
    }
    m_SDMMC->DLEN = byteLength;
    m_SDMMC->DCTRL = dataControl;

    bool result = ExecuteCmd(SDMMC_CMD_CMDTRANS, cmd, arg);

    if (result)
    {
        TimeValMicros startTime = get_system_time();
        do
        {
            result = WaitIRQ(SDMMC_MASK_DATAENDIE | SDMMC_MASK_IDMABTCIE | SDMMC_MASK_DABORTIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_DCRCFAILIE);
            if (!result) break;
            if (get_system_time() - startTime > TimeValMicros::FromMilliseconds(500))
            {
                result = false;
                printf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() could only read %u of %u blocks.\n", m_CurrentSegment, m_SegmentCount);
                break;
            }
        } while (m_CurrentSegment != m_SegmentCount);
    } else {
        printf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() Failed to start cmd %lu:%u (%d)\n", arg, m_SegmentCount, int(m_WakeupReason));
    }
    m_SDMMC->IDMACTRL = 0;
//    m_SDMMC->CLKCR &= ~SDMMC_CLKCR_HWFC_EN; // Hardware flow-control disabled.

    if (m_CurrentSegment != m_SegmentCount) {
        printf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() Only %u of %u blocks where transfered at %lu (%d)\n", m_CurrentSegment, m_SegmentCount, arg, int(m_WakeupReason));
        result = false;
    }

    if (result)
    {
        if ((cmd & SDMMC_CMD_WRITE) == 0)
        {
            if (dmaTarget != nullptr)
            {
                SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(dmaTarget), ((byteLength + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE);
                memcpy(segments[0].Buffer, dmaTarget, segments[0].Length);
            }
            else
            {
                for (size_t i = 0; i < segmentCount; ++i)
                {
                    if ((intptr_t(segments[i].Buffer) & DCACHE_LINE_SIZE_MASK) != 0)
                    {
                        kprintf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() multi segment read with unaligned buffer.\n");
                        set_last_error(EINVAL);
                        return false;
                    }
                    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(segments[i].Buffer), segments[i].Length);
                }
            }
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
            kprintf("ERROR: SDMMCDriver invalid bus width (%d) using 1-bit.\n", m_BusWidth);
            break;
    }
    m_SDMMC->CLKCR = CLKCR;
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
    else if (status & (SDMMC_EVENT_FLAGS & ~SDMMC_MASK_IDMABTCIE))
    {
        m_SDMMC->MASK = 0;
        m_IOError = 0;
        m_WakeupReason = WakeupReason::Event;
        m_IOCondition.Wakeup(0);
    }
    else if (status & SDMMC_MASK_IDMABTCIE)
    {
        m_SDMMC->ICR = SDMMC_ICR_IDMABTCC;
        if (m_CurrentSegment < m_SegmentCount)
        {
            if (m_SDMMC->IDMACTRL & SDMMC_IDMA_IDMABACT) {
                m_SDMMC->IDMABASE0 = intptr_t(m_TransferSegments[m_CurrentSegment++].Buffer);
            } else {
                m_SDMMC->IDMABASE1 = intptr_t(m_TransferSegments[m_CurrentSegment++].Buffer);
            }
        }
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::WaitIRQ(uint32_t flags)
{
    static constexpr uint32_t errorFlags = ~SDMMC_EVENT_FLAGS;
    uint32_t status = m_SDMMC->STA & (flags & ~SDMMC_STA_IDMABTC);

    if (status & errorFlags)
    {
        set_last_error(EIO);
        kprintf("SDMMC: ERROR already flagged: %x\n", status);
        return false;
    }
    if (status & flags) {
        return true;
    }

    m_WakeupReason = WakeupReason::None;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        m_SDMMC->MASK = flags;
        while (!m_IOCondition.IRQWaitTimeout(TimeValMicros::FromMilliseconds(500)))
        {
            if (get_last_error() != EINTR)
            {
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
                kprintf("SDMMC: ERROR SDMMC_STA_CTIMEOUT\n");
            }
            if (m_IOError & SDMMC_STA_DTIMEOUT) {
                kprintf("SDMMC: ERROR SDMMC_STA_DTIMEOUT\n");
            }
            if (m_IOError & SDMMC_STA_CCRCFAIL) {
                kprintf("SDMMC: ERROR SDMMC_STA_CCRCFAIL\n");
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
    SpinTimer::SleepuS(uint32_t(delay.AsMicroSeconds()));

    m_SDMMC->CLKCR = CLKCR; // Restore power-save.
}

