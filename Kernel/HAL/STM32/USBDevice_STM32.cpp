// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.07.2022 20:30

#include <Utils/Utils.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/HAL/STM32/USBDevice_STM32.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/HAL/STM32/ResetAndClockControl.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/IRQDispatcher.h>
#include <System/TimeValue.h>

namespace kernel
{

static constexpr uint32_t USB_DEVICE_IN_ENDPOINT_INTERRUPT_CLEAR_MASK =
    USB_OTG_DIEPINT_XFRC | USB_OTG_DIEPINT_EPDISD | USB_OTG_DIEPINT_AHBERR | USB_OTG_DIEPINT_TOC | USB_OTG_DIEPINT_ITTXFE
    | USB_OTG_DIEPINT_INEPNM | USB_OTG_DIEPINT_INEPNE | USB_OTG_DIEPINT_TXFIFOUDRN | USB_OTG_DIEPINT_BNA
    | USB_OTG_DIEPINT_PKTDRPSTS | USB_OTG_DIEPINT_BERR | USB_OTG_DIEPINT_NAK;

static constexpr uint32_t USB_DEVICE_OUT_ENDPOINT_INTERRUPT_CLEAR_MASK =
    USB_OTG_DOEPINT_XFRC | USB_OTG_DOEPINT_EPDISD | USB_OTG_DOEPINT_AHBERR | USB_OTG_DOEPINT_STUP | USB_OTG_DOEPINT_OTEPDIS
    | USB_OTG_DOEPINT_OTEPSPR | USB_OTG_DOEPINT_B2BSTUP | USB_OTG_DOEPINT_OUTPKTERR | USB_OTG_DOEPINT_BERR
    | USB_OTG_DOEPINT_NAK | USB_OTG_DOEPINT_NYET | USB_OTG_DOEPINT_STPKTRX;

static constexpr uint32_t USB_DEVICE_ENDPOINT0_SETUP_TRANSFER_CONFIG =
    sizeof(USB_ControlRequest) * 3
    | (1 << USB_OTG_DOEPTSIZ_PKTCNT_Pos)
    | (3 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);

static constexpr uint32_t USB_DEVICE_IN_ENDPOINT0_RESET_CLEAR_MASK =
    USB_OTG_DIEPCTL_STALL | USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_EPENA;

static constexpr uint32_t USB_DEVICE_OUT_ENDPOINT0_RESET_CLEAR_MASK =
    USB_OTG_DOEPCTL_STALL | USB_OTG_DOEPCTL_SNAK | USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_EPENA;

static constexpr uint32_t USB_DEVICE_ALL_TX_FIFOS = 0x10;

static constexpr TimeValNanos USB_DEVICE_ENDPOINT_DISABLE_TIMEOUT = TimeValNanos::FromMilliseconds(100);
static constexpr uint32_t USB_DEVICE_MAX_RX_FIFO_PACKETS_PER_IRQ = 64;
static constexpr uint32_t USB_DEVICE_IRQ_REGISTER_WAIT_ITERATIONS = 100000;

static bool WaitForRegisterBitsSet(const volatile uint32_t& deviceRegister, uint32_t bitMask)
{
    for (TimeValNanos endTime = kget_monotonic_time() + USB_DEVICE_ENDPOINT_DISABLE_TIMEOUT; (deviceRegister & bitMask) == 0; ) {
        if (kget_monotonic_time() > endTime) {
            return false;
        }
    }
    return true;
}

static bool WaitForRegisterBitsSetFromIRQ(const volatile uint32_t& deviceRegister, uint32_t bitMask)
{
    for (uint32_t retry = 0; retry < USB_DEVICE_IRQ_REGISTER_WAIT_ITERATIONS; ++retry) {
        if ((deviceRegister & bitMask) == bitMask) {
            return true;
        }
    }
    return false;
}

static bool WaitForRegisterBitsClearFromIRQ(const volatile uint32_t& deviceRegister, uint32_t bitMask)
{
    for (uint32_t retry = 0; retry < USB_DEVICE_IRQ_REGISTER_WAIT_ITERATIONS; ++retry) {
        if ((deviceRegister & bitMask) == 0) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense, bool useSOF)
{
    m_Driver = driver;
    m_Port = get_usb_from_id(portID);

    m_Device = reinterpret_cast<USB_OTG_DeviceTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_DEVICE_BASE);
    m_OutEndpoints = reinterpret_cast<USB_OTG_OUTEndpointTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_OUT_ENDPOINT_BASE);
    m_InEndpoints = reinterpret_cast<USB_OTG_INEndpointTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_IN_ENDPOINT_BASE);

    if (enableVBusSense)
    {
        m_Port->GCCFG |= USB_OTG_GCCFG_VBDEN;
        m_Port->GOTGCTL &= ~(USB_OTG_GOTGCTL_BVALOEN | USB_OTG_GOTGCTL_BVALOVAL);
    }
    else
    {
        m_Port->GCCFG &= ~USB_OTG_GCCFG_VBDEN;
        m_Port->GOTGCTL |= USB_OTG_GOTGCTL_BVALOEN;
        m_Port->GOTGCTL |= USB_OTG_GOTGCTL_BVALOVAL;
    }
    const uint32_t interruptMask = USB_OTG_GINTMSK_MMISM | USB_OTG_GINTMSK_OTGINT | USB_OTG_GINTMSK_RXFLVLM
        | USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_USBSUSPM | USB_OTG_GINTMSK_WUIM | (useSOF ? USB_OTG_GINTMSK_SOFM : 0);
    m_Port->GINTMSK = interruptMask;

    // Full speed using internal FS PHY.
    SetSpeed(USB_Speed::FULL);
    // Send a STALL handshake on a nonzero-length status OUT transaction.
    m_Device->DCFG |= USB_OTG_DCFG_NZLSOHSK;

    IRQn_Type irq = get_usb_irq(portID);
    register_irq_handler(irq, &USBDevice_STM32::IRQCallback, this);

    DeviceConnect();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_Speed USBDevice_STM32::DeviceGetSpeed() const
{
    const uint32_t enumeratedSpeed = (m_Device->DSTS & USB_OTG_DSTS_ENUMSPD_Msk) >> USB_OTG_DSTS_ENUMSPD_Pos;
    return (enumeratedSpeed == USB_OTG_ENUMERATED_SPEED_HIGH) ? USB_Speed::HIGH : USB_Speed::FULL;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointStall(uint8_t endpointAddr)
{
    EndpointDisable(endpointAddr, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointClearStall(uint8_t endpointAddr)
{
    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointAddr);

    // Clear stall and reset data toggle.
    if (endpointAddr & USB_ADDRESS_DIR_IN)
    {
        m_InEndpoints[epNum].DIEPCTL &= ~USB_OTG_DIEPCTL_STALL;
        m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM;
    }
    else
    {
        m_OutEndpoints[epNum].DOEPCTL &= ~USB_OTG_DOEPCTL_STALL;
        m_OutEndpoints[epNum].DOEPCTL |= USB_OTG_DOEPCTL_SD0PID_SEVNFRM;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::EndpointOpen(const USB_DescEndpoint& endpointDescriptor)
{
    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointDescriptor.bEndpointAddress);

    EndpointTransferState* xfer = GetEndpointTranferState(endpointDescriptor.bEndpointAddress);

    if (xfer == nullptr) {
        return false;
    }

    xfer->EndpointMaxSize = endpointDescriptor.GetMaxPacketSize();
    xfer->Interval = endpointDescriptor.bInterval;

    const uint32_t fifoSizeWords = std::max(16ul, (xfer->EndpointMaxSize * 2 + 3) / 4);

    const USB_TransferType transferType = endpointDescriptor.GetTransferType();

    if (endpointDescriptor.bEndpointAddress & USB_ADDRESS_DIR_IN)
    {
        // Check if enough FIFO space is available.
        if (m_AllocatedTXFIFOWords + fifoSizeWords + m_Port->GRXFSIZ > USB_OTG_FIFO_SIZE / 4) {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "failed to allocate {} FIFO bytes ({}).", fifoSizeWords * 4, m_AllocatedTXFIFOWords * 4);
            return false;
        }

        m_AllocatedTXFIFOWords += fifoSizeWords;

        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Allocated {} FIFO bytes at offset {} for EP {}.", fifoSizeWords * 4, USB_OTG_FIFO_SIZE - m_AllocatedTXFIFOWords * 4, USB_ADDRESS_EPNUM(endpointDescriptor.bEndpointAddress));

        m_Port->DIEPTXF[epNum - 1] = (fifoSizeWords << USB_OTG_DIEPTXF_INEPTXFD_Pos) | (USB_OTG_FIFO_SIZE / 4 - m_AllocatedTXFIFOWords);

        m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_USBAEP |
            (epNum << USB_OTG_DIEPCTL_TXFNUM_Pos) |
            (uint32_t(transferType) << USB_OTG_DIEPCTL_EPTYP_Pos) |
            (transferType != USB_TransferType::ISOCHRONOUS ? USB_OTG_DIEPCTL_SD0PID_SEVNFRM : 0) |
            (xfer->EndpointMaxSize << USB_OTG_DIEPCTL_MPSIZ_Pos);

        m_Device->DAINTMSK |= (1 << (USB_OTG_DAINTMSK_IEPM_Pos + epNum));
    }
    else
    {
        // Check if the RX FIFO must be expanded.
        const uint32_t size = CalculateRXFIFOSize(fifoSizeWords * 4);
        if (m_Port->GRXFSIZ < size)
        {
            if (size + m_AllocatedTXFIFOWords > USB_OTG_FIFO_SIZE / 4) {
                return false;
            }
            m_Port->GRXFSIZ = size;
        }
        m_OutEndpoints[epNum].DOEPCTL |=
            USB_OTG_DOEPCTL_USBAEP
            | (uint32_t(transferType) << USB_OTG_DOEPCTL_EPTYP_Pos)
            | (transferType != USB_TransferType::ISOCHRONOUS ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM : 0)
            | (xfer->EndpointMaxSize << USB_OTG_DOEPCTL_MPSIZ_Pos);

        m_Device->DAINTMSK |= 1 << (USB_OTG_DAINTMSK_OEPM_Pos + epNum);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointClose(uint8_t endpointAddr)
{
    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointAddr);

    EndpointDisable(endpointAddr, false);

    if (endpointAddr & USB_ADDRESS_DIR_IN)
    {
        m_TransferStatusIn[epNum].Reset();

        const uint32_t fifoSize = (m_Port->DIEPTXF[epNum - 1] & USB_OTG_DIEPTXF_INEPTXFD_Msk) >> USB_OTG_DIEPTXF_INEPTXFD_Pos;
        const uint32_t fifoStart = (m_Port->DIEPTXF[epNum - 1] & USB_OTG_DIEPTXF_INEPTXSA_Msk) >> USB_OTG_DIEPTXF_INEPTXSA_Pos;

        // FIXME: support closing endpoints in any order.
        if (fifoStart == USB_OTG_FIFO_SIZE / 4 - m_AllocatedTXFIFOWords)
        {
            m_AllocatedTXFIFOWords -= fifoSize;
        }
        else
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "USB_STM32::EndpointClose({:02x}) called on non-last endpoint. Leaking {} FIFO bytes.", endpointAddr, fifoSize * 4);
        }
    }
    else
    {
        m_TransferStatusOut[epNum].Reset();
        m_UpdateRXFIFOSize = true;  // Update RX FIFO size when empty.
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///
/// Close all non-control endpoints, cancel any pending transfers.
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointCloseAll()
{
    // Disable interrupts for non-control endpoints.
    m_Device->DAINTMSK = (0x01 << USB_OTG_DAINTMSK_OEPM_Pos) | (0x01 << USB_OTG_DAINTMSK_IEPM_Pos);

    for (int i = 1; i < ENDPOINT_COUNT; ++i)
    {
        // Disable out endpoint.
        m_OutEndpoints[i].DOEPCTL = 0;
        m_TransferStatusOut[i].Reset();

        // Disable in endpoint.
        m_InEndpoints[i].DIEPCTL = 0;
        m_TransferStatusIn[i].Reset();
    }
    // Reset TX FIFO allocation.
    m_AllocatedTXFIFOWords = 16;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength)
{
    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointAddr);

    EndpointTransferState* xfer = GetEndpointTranferState(endpointAddr);

    if (xfer == nullptr || xfer->EndpointMaxSize == 0) {
        return false;
    }

    xfer->Buffer = static_cast<uint8_t*>(buffer);
    xfer->BufferSize = totalLength;
    xfer->BytesTransferred = 0;
    xfer->TotalLength = totalLength;

    // Endpoint0 can only handle a single packet.
    if (epNum == 0)
    {
        if (endpointAddr & USB_ADDRESS_DIR_IN) {
            m_Enpoint0InPending = totalLength;
        }
        else {
            m_Enpoint0OutPending = totalLength;
        }
        EndpointSchedulePackets(endpointAddr, 1, totalLength);
    }
    else
    {
        uint32_t packetCount = (totalLength != 0) ? ((totalLength + xfer->EndpointMaxSize - 1) / xfer->EndpointMaxSize) : 1;
        EndpointSchedulePackets(endpointAddr, packetCount, totalLength);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::SetAddress(uint8_t deviceAddr)
{
    set_bit_group(m_Device->DCFG, USB_OTG_DCFG_DAD_Msk, deviceAddr << USB_OTG_DCFG_DAD_Pos);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::ActivateRemoteWakeup(bool activate)
{
    if (activate)
    {
        if (m_Device->DSTS & USB_OTG_DSTS_SUSPSTS) {
            m_Device->DCTL |= USB_OTG_DCTL_RWUSIG; // Activate remote wakeup signaling.
        }
    }
    else
    {
        m_Device->DCTL &= ~USB_OTG_DCTL_RWUSIG;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::SetSpeed(USB_Speed speed)
{
    const uint32_t speedCfg = (speed == USB_Speed::HIGH) ? USB_OTG_ENUMERATED_SPEED_HIGH : USB_OTG_ENUMERATED_SPEED_FULL;
    set_bit_group(m_Device->DCFG, USB_OTG_DCFG_DSPD_Msk, speedCfg << USB_OTG_DCFG_DSPD_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::DeviceConnect()
{
    m_Device->DCTL &= ~USB_OTG_DCTL_SDIS;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::DeviceDisconnect()
{
    m_Device->DCTL |= USB_OTG_DCTL_SDIS;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t USBDevice_STM32::CalculateRXFIFOSize(uint32_t maxEndpointSize) const
{
    return 15 + 2 * (maxEndpointSize / 4) + 2 * ENDPOINT_COUNT;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::ResetReceived()
{
    for (int i = 0; i < ENDPOINT_COUNT; ++i)
    {
        m_TransferStatusIn[i].Reset();
        m_TransferStatusOut[i].Reset();
    }
    m_Enpoint0InPending  = 0;
    m_Enpoint0OutPending = 0;
    m_UpdateRXFIFOSize   = false;
    m_ControlRequestPackage = {};
    m_Endpoint0InTransferActive = false;
    m_Endpoint0OutTransferActive = false;
    m_HasPendingSetupRequest = false;

    // Drop any global NAK state left by an interrupted endpoint-disable sequence.
    m_Device->DCTL |= USB_OTG_DCTL_CGINAK | USB_OTG_DCTL_CGONAK;
    m_Port->GINTSTS = USB_OTG_GINTSTS_GINAKEFF | USB_OTG_GINTSTS_BOUTNAKEFF;

    m_Device->DAINTMSK = 0;
    m_Device->DOEPMSK = 0;
    m_Device->DIEPMSK = 0;
    m_Device->DIEPEMPMSK = 0;

    (void)FlushTxFifoFromIRQ(USB_DEVICE_ALL_TX_FIFOS);
    (void)FlushRxFifoFromIRQ();

    for (uint32_t endpointIndex = 0; endpointIndex < ENDPOINT_COUNT; ++endpointIndex)
    {
        m_InEndpoints[endpointIndex].DIEPINT = USB_DEVICE_IN_ENDPOINT_INTERRUPT_CLEAR_MASK;
        m_OutEndpoints[endpointIndex].DOEPINT = USB_DEVICE_OUT_ENDPOINT_INTERRUPT_CLEAR_MASK;
    }

    for (uint32_t endpointIndex = 1; endpointIndex < ENDPOINT_COUNT; ++endpointIndex)
    {
        m_InEndpoints[endpointIndex].DIEPCTL = 0;
        m_InEndpoints[endpointIndex].DIEPTSIZ = 0;
        m_OutEndpoints[endpointIndex].DOEPCTL = 0;
        m_OutEndpoints[endpointIndex].DOEPTSIZ = 0;
        m_Port->DIEPTXF[endpointIndex - 1] = 0;
    }

    m_InEndpoints[0].DIEPCTL &= ~USB_DEVICE_IN_ENDPOINT0_RESET_CLEAR_MASK;
    m_OutEndpoints[0].DOEPCTL &= ~USB_DEVICE_OUT_ENDPOINT0_RESET_CLEAR_MASK;

    // Clear device address.
    m_Device->DCFG &= ~USB_OTG_DCFG_DAD_Msk;

    // NAK all OUT endpoints.
    for (int i = 0; i < ENDPOINT_COUNT; ++i) {
        m_OutEndpoints[i].DOEPCTL |= USB_OTG_DOEPCTL_SNAK;
    }

    // Enable interrupts.
    m_Device->DAINTMSK = (1 << USB_OTG_DAINTMSK_OEPM_Pos) | (1 << USB_OTG_DAINTMSK_IEPM_Pos);
    m_Device->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM;
    m_Device->DIEPMSK = USB_OTG_DIEPMSK_TOM | USB_OTG_DIEPMSK_XFRCM;
    m_Device->DIEPEMPMSK = 0;

    m_Port->GRXFSIZ = CalculateRXFIFOSize(m_SupportHighSpeed ? 512 : 64);

    m_AllocatedTXFIFOWords = 16;

    // Control IN uses FIFO 0 with 64 bytes.
    m_Port->DIEPTXF0_HNPTXFSIZ = (16 << USB_OTG_TX0FD_Pos) | (USB_OTG_FIFO_SIZE / 4 - m_AllocatedTXFIFOWords);

    // Set control endpoint0 size to 64 bytes.
    m_InEndpoints[0].DIEPCTL &= ~USB_OTG_DIEPCTL_MPSIZ_Msk;
    m_TransferStatusOut[0].EndpointMaxSize = m_TransferStatusIn[0].EndpointMaxSize = 64;

    m_OutEndpoints[0].DOEPTSIZ = USB_DEVICE_ENDPOINT0_SETUP_TRANSFER_CONFIG;

    m_Port->GINTMSK |= USB_OTG_GINTMSK_OEPINT | USB_OTG_GINTMSK_IEPINT;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::UpdateGRXFSIZ()
{
    uint32_t maxEndpointSize = 0;
    for (uint32_t epNum = 0; epNum < ENDPOINT_COUNT; ++epNum) {
        maxEndpointSize = std::max(maxEndpointSize, m_TransferStatusOut[epNum].EndpointMaxSize);
    }
    m_Port->GRXFSIZ = CalculateRXFIFOSize(maxEndpointSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::SetTurnaround(USB_Speed speed)
{
    uint32_t turnaround;
    if (speed == USB_Speed::HIGH)
    {
        turnaround = 9; // In high-speed mode turnaround should always be 9.
    }
    else
    {
        const uint32_t ahbClock = ResetAndClockControl::GetHCLKFrequency();
        // Speed thresholds from the data-sheet.
        if (ahbClock >= 32000000) {
            turnaround = 6;
        } else if (ahbClock >= 27500000) {
            turnaround = 7;
        } else if (ahbClock >= 24000000) {
            turnaround = 8;
        } else if (ahbClock >= 21800000) {
            turnaround = 9;
        } else if (ahbClock >= 20000000) {
            turnaround = 10;
        } else if (ahbClock >= 18500000) {
            turnaround = 11;
        } else if (ahbClock >= 17200000) {
            turnaround = 12;
        } else if (ahbClock >= 16000000) {
            turnaround = 13;
        } else if (ahbClock >= 15000000) {
            turnaround = 14;
        } else {
            turnaround = 15;
        }
    }
    set_bit_group(m_Port->GUSBCFG, USB_OTG_GUSBCFG_TRDT_Msk, turnaround << USB_OTG_GUSBCFG_TRDT_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointDisable(uint8_t endpointAddr, bool stall)
{
    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointAddr);

    if (endpointAddr & USB_ADDRESS_DIR_IN)
    {
        // Only disable currently enabled non-control endpoint.
        if (epNum == 0 || (m_InEndpoints[epNum].DIEPCTL & USB_OTG_DIEPCTL_EPENA) == 0)
        {
            m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_SNAK | (stall ? USB_OTG_DIEPCTL_STALL : 0);
        }
        else
        {
            // Stop transmitting packets and NAK IN transfers.
            m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_INEPNE;
            m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_SNAK;
            if (!WaitForRegisterBitsSet(m_InEndpoints[epNum].DIEPINT, USB_OTG_DIEPINT_INEPNE)) {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for IN endpoint {:02x} NAK.", endpointAddr);
            }

            // Disable the endpoint.
            m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_EPDISD;
            m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_EPDIS | (stall ? USB_OTG_DIEPCTL_STALL : 0);
            if (WaitForRegisterBitsSet(m_InEndpoints[epNum].DIEPINT, USB_OTG_DIEPINT_EPDISD_Msk)) {
                m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_EPDISD;
            } else {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for IN endpoint {:02x} disable.", endpointAddr);
            }
        }

        // Flush the FIFO, and wait until we have confirmed it cleared.
        if (!m_Driver->FlushTxFifo(epNum)) {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout flushing IN endpoint {:02x} TX FIFO.", endpointAddr);
        }
    }
    else
    {
        // Only disable currently enabled non-control endpoint.
        if ((epNum == 0) || !(m_OutEndpoints[epNum].DOEPCTL & USB_OTG_DOEPCTL_EPENA))
        {
            if (stall) {
                m_OutEndpoints[epNum].DOEPCTL |= USB_OTG_DOEPCTL_STALL;
            }
        }
        else
        {
            m_Port->GINTSTS = USB_OTG_GINTSTS_BOUTNAKEFF;
            m_Device->DCTL |= USB_OTG_DCTL_SGONAK;
            if (!WaitForRegisterBitsSet(m_Port->GINTSTS, USB_OTG_GINTSTS_BOUTNAKEFF_Msk)) {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for global OUT NAK before disabling endpoint {:02x}.", endpointAddr);
            }

            // Disable the endpoint.
            m_OutEndpoints[epNum].DOEPINT = USB_OTG_DOEPINT_EPDISD;
            m_OutEndpoints[epNum].DOEPCTL |= USB_OTG_DOEPCTL_EPDIS | (stall ? USB_OTG_DOEPCTL_STALL : 0);
            if (WaitForRegisterBitsSet(m_OutEndpoints[epNum].DOEPINT, USB_OTG_DOEPINT_EPDISD_Msk)) {
                m_OutEndpoints[epNum].DOEPINT = USB_OTG_DOEPINT_EPDISD;
            } else {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for OUT endpoint {:02x} disable.", endpointAddr);
            }

            // Allow other OUT endpoints to keep receiving.
            m_Port->GINTSTS = USB_OTG_GINTSTS_BOUTNAKEFF;
            m_Device->DCTL |= USB_OTG_DCTL_CGONAK;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointSchedulePackets(uint8_t endpointAddr, uint32_t packetCount, uint32_t totalLength)
{
    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointAddr);

    // Enpoint0 is limited to one packet each transfer. We use multiple max-size transactions to complete a transfer.
    if (epNum == 0)
    {
        const EndpointTransferState* xfer = GetEndpointTranferState(endpointAddr);
        uint32_t& pending = (endpointAddr & USB_ADDRESS_DIR_IN) ? m_Enpoint0InPending : m_Enpoint0OutPending;
        totalLength = std::min(pending, xfer->EndpointMaxSize);
        pending -= totalLength;
    }

    // IN and OUT endpoint transfers are interrupt-driven, we just schedule them here.
    if (endpointAddr & USB_ADDRESS_DIR_IN)
    {
        if (epNum == 0) {
            m_Endpoint0InTransferActive = true;
        }
        // A full IN transfer (multiple packets, possibly) triggers XFRC.
        m_InEndpoints[epNum].DIEPTSIZ = (packetCount << USB_OTG_DIEPTSIZ_PKTCNT_Pos) | ((totalLength << USB_OTG_DIEPTSIZ_XFRSIZ_Pos) & USB_OTG_DIEPTSIZ_XFRSIZ_Msk);
        m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK;

        if (USB_TransferType((m_InEndpoints[epNum].DIEPCTL & USB_OTG_DIEPCTL_EPTYP_Msk) >> USB_OTG_DIEPCTL_EPTYP_Pos) == USB_TransferType::ISOCHRONOUS && m_TransferStatusIn[epNum].Interval == 1)
        {
            // For ISOCHRONOUS endpoint set correct odd/even bit for next frame.
            const bool currentFrameOdd = (m_Device->DSTS & (1 << USB_OTG_DSTS_FNSOF_Pos)) != 0; // Frame counter bit 0.
            m_InEndpoints[epNum].DIEPCTL |= currentFrameOdd ? USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DIEPCTL_SODDFRM_Msk;
        }
        if (totalLength != 0) {
            m_Device->DIEPEMPMSK |= (1 << epNum); // Enable fifo empty interrupt.
        }
    }
    else
    {
        if (epNum == 0) {
            m_Endpoint0OutTransferActive = true;
        }
        set_bit_group(
            m_OutEndpoints[epNum].DOEPTSIZ,
            USB_OTG_DOEPTSIZ_PKTCNT_Msk | USB_OTG_DOEPTSIZ_XFRSIZ,
            (packetCount << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | ((totalLength << USB_OTG_DOEPTSIZ_XFRSIZ_Pos) & USB_OTG_DOEPTSIZ_XFRSIZ_Msk)
        );

        m_OutEndpoints[epNum].DOEPCTL |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;
        if (USB_TransferType((m_OutEndpoints[epNum].DOEPCTL & USB_OTG_DOEPCTL_EPTYP) >> USB_OTG_DIEPCTL_EPTYP_Pos) == USB_TransferType::ISOCHRONOUS && m_TransferStatusOut[epNum].Interval == 1)
        {
            // For ISOCHRONOUS endpoint set correct odd/even bit for next frame.
            const bool currentFrameOdd = (m_Device->DSTS & (1 << USB_OTG_DSTS_FNSOF_Pos)) != 0; // Frame counter bit 0.
            m_OutEndpoints[epNum].DOEPCTL |= currentFrameOdd ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DOEPCTL_SODDFRM_Msk;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::DiscardFromFIFO(size_t length)
{
    uint32_t discardBuffer[16];

    while (length != 0)
    {
        const size_t chunkLength = std::min(length, sizeof(discardBuffer));
        m_Driver->ReadFromFIFO(discardBuffer, chunkLength);
        length -= chunkLength;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::FlushTxFifoFromIRQ(uint32_t fifoIndex)
{
    if (!WaitForRegisterBitsSetFromIRQ(m_Port->GRSTCTL, USB_OTG_GRSTCTL_AHBIDL)) {
        return false;
    }
    m_Port->GRSTCTL = USB_OTG_GRSTCTL_TXFFLSH | (fifoIndex << USB_OTG_GRSTCTL_TXFNUM_Pos);
    return WaitForRegisterBitsClearFromIRQ(m_Port->GRSTCTL, USB_OTG_GRSTCTL_TXFFLSH);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::FlushRxFifoFromIRQ()
{
    if (!WaitForRegisterBitsSetFromIRQ(m_Port->GRSTCTL, USB_OTG_GRSTCTL_AHBIDL)) {
        return false;
    }
    m_Port->GRSTCTL = USB_OTG_GRSTCTL_RXFFLSH;
    return WaitForRegisterBitsClearFromIRQ(m_Port->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USBDevice_STM32::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<USBDevice_STM32*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USBDevice_STM32::HandleIRQ()
{
    const uint32_t intStatus = m_Port->GINTSTS & m_Port->GINTMSK;

    if (intStatus & USB_OTG_GINTMSK_MMISM)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_MMIS;
    }
    if (intStatus & USB_OTG_GINTSTS_USBRST)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_USBRST;
        ResetReceived();
        return IRQResult::HANDLED;
    }
    if (intStatus & USB_OTG_GINTSTS_ENUMDNE)   // ENUMDNE indicates the end of reset on the USB. Speed has been detected.
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;
        const USB_Speed speed = DeviceGetSpeed();
        SetTurnaround(speed);
        m_Driver->IRQBusReset(speed);
    }
    if (intStatus & USB_OTG_GINTSTS_USBSUSP)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_USBSUSP;
        m_Driver->IRQSuspend();
    }
    if (intStatus & USB_OTG_GINTSTS_WKUINT)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_WKUINT;
        m_Driver->IRQResume();
    }
    if (intStatus & USB_OTG_GINTSTS_OTGINT)
    {
        const uint32_t otgInt = m_Port->GOTGINT;

        if (otgInt & USB_OTG_GOTGINT_SEDET) {
            m_Driver->IRQSessionEnded();
        }
        m_Port->GOTGINT = otgInt;
    }
    if (intStatus & USB_OTG_GINTSTS_SOF)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_SOF;
        m_Driver->IRQStartOfFrame();
    }
    // RxFIFO non-empty interrupt handling.
    if (intStatus & USB_OTG_GINTSTS_RXFLVL)
    {
        // Disable RXFLVL while reading from FIFO.
        m_Port->GINTMSK &= ~USB_OTG_GINTMSK_RXFLVLM;

        for (uint32_t packetIndex = 0; packetIndex < USB_DEVICE_MAX_RX_FIFO_PACKETS_PER_IRQ; ++packetIndex)
        {
            HandleRxFIFONotEmptyIRQ();
            if ((m_Port->GINTSTS & USB_OTG_GINTSTS_RXFLVL) == 0) {
                break;
            }
        }

        if (m_Port->GINTSTS & USB_OTG_GINTSTS_RXFLVL)
        {
            FlushRxFifoFromIRQ();
        }
        else if (m_UpdateRXFIFOSize)
        {
            UpdateGRXFSIZ();
            m_UpdateRXFIFOSize = false;
        }
        m_Port->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM;
    }
    if (intStatus & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT)   // Incomplete periodic transfer
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT;
    }
    if (intStatus & USB_OTG_GINTSTS_OEPINT) {
        HandleOutEndpointIRQ();
    }
    if (intStatus & USB_OTG_GINTSTS_IEPINT) {
        HandleInEndpointIRQ();
    }
    if (intStatus & USB_OTG_GINTSTS_IISOIXFR)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_IISOIXFR;
        m_Driver->IRQIncompleteIsochronousINTransfer();
    }

    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::HandleRxFIFONotEmptyIRQ()
{
    const uint32_t grxstsp = m_Port->GRXSTSP;

    const uint32_t packetStatus = (grxstsp & USB_OTG_GRXSTSP_PKTSTS_Msk) >> USB_OTG_GRXSTSP_PKTSTS_Pos;
    const uint32_t packetLength = (grxstsp & USB_OTG_GRXSTSP_BCNT_Msk) >> USB_OTG_GRXSTSP_BCNT_Pos;

    const uint32_t epNum = (grxstsp & USB_OTG_GRXSTSP_EPNUM_Msk) >> USB_OTG_GRXSTSP_EPNUM_Pos;
    if (epNum >= ENDPOINT_COUNT)
    {
        DiscardFromFIFO(packetLength);
        return;
    }

    switch (packetStatus)
    {
        case USB_PKTSTS_NAK:
            break;
        case USB_PKTSTS_OUT_DATA_RCV:
        {
            EndpointTransferState& xfer = m_TransferStatusOut[epNum];

            const bool transferFits = xfer.BytesTransferred <= xfer.BufferSize && packetLength <= xfer.BufferSize - xfer.BytesTransferred;
            const bool hasDestination = packetLength == 0 || xfer.Buffer != nullptr;

            if (transferFits && hasDestination)
            {
                if (packetLength != 0) {
                    m_Driver->ReadFromFIFO(xfer.Buffer + xfer.BytesTransferred, packetLength);
                }

                xfer.BytesTransferred += packetLength;

                if (packetLength < xfer.EndpointMaxSize)
                {
                    // Short packet received, transfer done. Adjust transfer length to the actual length received.
                    xfer.TotalLength -= (m_OutEndpoints[epNum].DOEPTSIZ & USB_OTG_DOEPTSIZ_XFRSIZ_Msk) >> USB_OTG_DOEPTSIZ_XFRSIZ_Pos;
                    if (epNum == 0)
                    {
                        xfer.TotalLength -= m_Enpoint0OutPending;
                        m_Enpoint0OutPending = 0;
                    }
                }
            }
            else
            {
                DiscardFromFIFO(packetLength);
            }
            break;
        }
        case USB_PKTSTS_OUT_XFR_DONE:
            break;
        case USB_PKTSTS_SETUP_XFR_DONE:
            m_OutEndpoints[epNum].DOEPTSIZ = USB_DEVICE_ENDPOINT0_SETUP_TRANSFER_CONFIG;
            break;
        case USB_PKTSTS_SETUP_DATA_RCV:
            m_Driver->ReadFromFIFO(&m_ControlRequestPackage, sizeof(m_ControlRequestPackage));
            m_HasPendingSetupRequest = true;
            break;
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::HandleOutEndpointIRQ()
{
    for (uint8_t epNum = 0; epNum < ENDPOINT_COUNT; ++epNum)
    {
        const EndpointTransferState& xfer = m_TransferStatusOut[epNum];

        if (m_Device->DAINT & (1 << (USB_OTG_DAINT_OEPINT_Pos + epNum)))
        {
            const uint32_t endpointInterrupts = m_OutEndpoints[epNum].DOEPINT;
            const uint32_t unhandledInterrupts = endpointInterrupts
                & USB_DEVICE_OUT_ENDPOINT_INTERRUPT_CLEAR_MASK
                & ~(USB_OTG_DOEPINT_STUP | USB_OTG_DOEPINT_XFRC);
            if (unhandledInterrupts != 0) {
                m_OutEndpoints[epNum].DOEPINT = unhandledInterrupts;
            }

            // Setup Phase done.
            if (endpointInterrupts & USB_OTG_DOEPINT_STUP)
            {
                m_OutEndpoints[epNum].DOEPINT = USB_OTG_DOEPINT_STUP;
                if (epNum == 0)
                {
                    m_Enpoint0InPending = 0;
                    m_Enpoint0OutPending = 0;
                    m_Device->DIEPEMPMSK &= ~1u;
                    m_Endpoint0InTransferActive = false;
                    m_Endpoint0OutTransferActive = false;

                    // STUP can be observed again after the setup packet has already been consumed.
                    // Only emit a high-level setup event when RX FIFO delivered fresh setup data.
                    if (m_HasPendingSetupRequest)
                    {
                        m_HasPendingSetupRequest = false;
                        m_Driver->IRQControlRequestReceived(m_ControlRequestPackage);
                    }
                }
            }
            // OUT transfer complete.
            if (endpointInterrupts & USB_OTG_DOEPINT_XFRC)
            {
                m_OutEndpoints[epNum].DOEPINT = USB_OTG_DOEPINT_XFRC;

                if (epNum == 0)
                {
                    if (!m_Endpoint0OutTransferActive) {
                        continue;
                    }
                    m_Endpoint0OutTransferActive = false;
                }

                // Enpoint0 can only handle one packet.
                if ((epNum == 0) && m_Enpoint0OutPending != 0) {
                    // Schedule another packet to be received.
                    EndpointSchedulePackets(USB_MK_OUT_ADDRESS(epNum), 1, m_Enpoint0OutPending);
                } else {
                    m_Driver->IRQTransferComplete(epNum, xfer.TotalLength, USB_TransferResult::Success);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::HandleInEndpointIRQ()
{
    for (uint8_t epNum = 0; epNum < ENDPOINT_COUNT; ++epNum)
    {
        EndpointTransferState& xfer = m_TransferStatusIn[epNum];

        if (m_Device->DAINT & (1 << (USB_OTG_DAINT_IEPINT_Pos + epNum)))
        {
            const uint32_t endpointInterrupts = m_InEndpoints[epNum].DIEPINT;
            const uint32_t unhandledInterrupts = endpointInterrupts
                & USB_DEVICE_IN_ENDPOINT_INTERRUPT_CLEAR_MASK
                & ~(USB_OTG_DIEPINT_TOC | USB_OTG_DIEPINT_XFRC);
            if (unhandledInterrupts != 0) {
                m_InEndpoints[epNum].DIEPINT = unhandledInterrupts;
            }

            if (endpointInterrupts & USB_OTG_DIEPINT_TOC) {
                m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_TOC;
            }

            // Entire IN transfer complete.
            if (endpointInterrupts & USB_OTG_DIEPINT_XFRC)
            {
                m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_XFRC;

                if (epNum == 0)
                {
                    if (!m_Endpoint0InTransferActive) {
                        continue;
                    }
                    m_Endpoint0InTransferActive = false;
                }

                // Enpoint0 can only handle one packet.
                if (epNum == 0 && m_Enpoint0InPending != 0) {
                    EndpointSchedulePackets(USB_MK_IN_ADDRESS(epNum), 1, m_Enpoint0InPending); // Schedule another packet to be transmitted.
                } else {
                    m_Driver->IRQTransferComplete(epNum | USB_ADDRESS_DIR_IN, xfer.TotalLength, USB_TransferResult::Success);
                }
            }

            // TX FIFO below threshold.
            if ((endpointInterrupts & USB_OTG_DIEPINT_TXFE) && (m_Device->DIEPEMPMSK & (1 << epNum)))
            {
                if (xfer.EndpointMaxSize == 0 || (xfer.TotalLength != 0 && xfer.Buffer == nullptr))
                {
                    m_Device->DIEPEMPMSK &= ~(1 << epNum);
                    continue;
                }

                while(xfer.BytesTransferred < xfer.TotalLength)
                {
                    const uint32_t remainingBytes = xfer.TotalLength - xfer.BytesTransferred;
                    const uint32_t packetSize = std::min(remainingBytes, xfer.EndpointMaxSize);
                    const uint32_t fifoWords = m_InEndpoints[epNum].DTXFSTS & USB_OTG_DTXFSTS_INEPTFSAV_Msk;
                    const uint32_t fifoBytes = fifoWords * 4;

                    if (packetSize == 0) {
                        break;
                    }
                    // Only full packets can be written to FIFO, so check that the entire packet will fit.
                    if (packetSize > fifoBytes) {
                        break;
                    }
                    kassert(xfer.BytesTransferred + packetSize <= xfer.BufferSize);
                    m_Driver->WriteToFIFO(epNum, xfer.Buffer + xfer.BytesTransferred, packetSize);

                    xfer.BytesTransferred += packetSize;
                }

                if (xfer.BytesTransferred == xfer.TotalLength)
                {
                    m_Device->DIEPEMPMSK &= ~(1 << epNum);
                }
            }
        }
    }
}

} //namespace kernel
