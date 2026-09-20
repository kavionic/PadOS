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

#include <malloc.h>
#include <cstring>

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

static constexpr uint32_t USB_DEVICE_IN_ENDPOINT_DMA_ERROR_MASK =
    USB_OTG_DIEPINT_AHBERR | USB_OTG_DIEPINT_TOC | USB_OTG_DIEPINT_TXFIFOUDRN | USB_OTG_DIEPINT_BNA | USB_OTG_DIEPINT_BERR;

static constexpr uint32_t USB_DEVICE_OUT_ENDPOINT_DMA_ERROR_MASK =
    USB_OTG_DOEPINT_AHBERR | USB_OTG_DOEPINT_OUTPKTERR | USB_OTG_DOEPINT_BERR;

static constexpr uint32_t USB_DEVICE_MINIMUM_DMA_RX_FIFO_WORD_COUNT = 128;

static constexpr uint32_t USB_DEVICE_CORE_ID_300A = 0x4f54300a;

static constexpr uint32_t USB_DEVICE_IN_ZLP_ENABLE_DELAY_CPU_CYCLES = 30;

static constexpr size_t USB_DEVICE_ENDPOINT0_SETUP_DMA_BUFFER_SIZE = 12 * sizeof(uint32_t);

static constexpr uint32_t USB_DEVICE_ENDPOINT0_SETUP_TRANSFER_CONFIG =
    sizeof(USB_ControlRequest) * 3
    | (1 << USB_OTG_DOEPTSIZ_PKTCNT_Pos)
    | (3 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);

static constexpr uint32_t USB_DEVICE_OUT_ENDPOINT0_RESET_CLEAR_MASK =
    USB_OTG_DOEPCTL_STALL | USB_OTG_DOEPCTL_SNAK | USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_EPENA;

static constexpr uint32_t USB_DEVICE_ALL_TX_FIFOS = 0x10;

static constexpr TimeValNanos USB_DEVICE_ENDPOINT_DISABLE_TIMEOUT = TimeValNanos::FromMilliseconds(100);
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

USBDevice_STM32::~USBDevice_STM32()
{
    free(m_DMABounceBuffers);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense, bool useSOF)
{
    m_Driver = driver;
    m_Port = get_usb_from_id(portID);

    if (m_Driver == nullptr)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USB device mode requires a controller driver.");
        return false;
    }

    const size_t dmaBufferAllocationSize = DMA_BUFFER_COUNT * DMA_BOUNCE_BUFFER_SIZE;
    if (m_DMABounceBuffers == nullptr) {
        m_DMABounceBuffers = static_cast<uint8_t*>(memalign(__SCB_DCACHE_LINE_SIZE, dmaBufferAllocationSize));
    }
    if (m_DMABounceBuffers == nullptr)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Failed to allocate USB device DMA bounce buffers.");
        return false;
    }
    if (!USB_STM32::IsDirectDMAReceiveBuffer(m_DMABounceBuffers, dmaBufferAllocationSize))
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USB device DMA bounce buffers are not DMA accessible.");
        free(m_DMABounceBuffers);
        m_DMABounceBuffers = nullptr;
        return false;
    }

    m_Device = reinterpret_cast<USB_OTG_DeviceTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_DEVICE_BASE);
    m_OutEndpoints = reinterpret_cast<USB_OTG_OUTEndpointTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_OUT_ENDPOINT_BASE);
    m_InEndpoints = reinterpret_cast<USB_OTG_INEndpointTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_IN_ENDPOINT_BASE);

    m_EnableVBusSense = enableVBusSense;
    m_UseSOF = useSOF;
    ConfigureDevice();

    IRQn_Type irq = get_usb_irq(portID);
    register_irq_handler(irq, &USBDevice_STM32::IRQCallback, this);

    DeviceConnect();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::CompleteDeviceReset(uint32_t generation)
{
    USBIRQDisabler irqDisabler(*m_Driver);
    if (generation != m_DeviceGeneration || !m_ResetComplete || m_RecoveryPending) {
        return false;
    }
    m_DeviceReady = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::Recover()
{
    USBIRQDisabler irqDisabler(*m_Driver);
    if (!m_RecoveryPending) {
        return true;
    }
    // Remain disconnected long enough for the host to observe removal. This runs in the device thread, never an IRQ.
    snooze_ms(10);
    if (!m_Driver->ResetDeviceCore())
    {
        m_Driver->HoldCoreInReset();
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USB device restart failed; controller held in reset.");
        return false;
    }
    DeviceDisconnect();
    ConfigureDevice();
    m_RecoveryPending = false;
    m_Driver->EnableIRQ(true);
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
    USBIRQDisabler irqDisabler(*m_Driver);

    if (m_DeviceReady)
    {
        if (!EndpointDisable(endpointAddr, true))
        {
            RequestRecovery();
            return;
        }
        CancelEndpointTransfer(endpointAddr);
        if (USB_ADDRESS_EPNUM(endpointAddr) == 0) {
            PrepareSetupPackets();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::EndpointClearStall(uint8_t endpointAddr)
{
    USBIRQDisabler irqDisabler(*m_Driver);

    if (!m_DeviceReady) {
        return false;
    }

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
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::EndpointOpen(const USB_DescEndpoint& endpointDescriptor)
{
    USBIRQDisabler irqDisabler(*m_Driver);

    if (!m_DeviceReady) {
        return false;
    }

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
        if (m_AllocatedTXFIFOWords + fifoSizeWords + m_Port->GRXFSIZ > USB_STM32::DMA_FIFO_USABLE_WORD_COUNT) {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "failed to allocate {} FIFO bytes ({}).", fifoSizeWords * 4, m_AllocatedTXFIFOWords * 4);
            return false;
        }

        m_AllocatedTXFIFOWords += fifoSizeWords;
        const uint32_t fifoStart = USB_STM32::DMA_FIFO_USABLE_WORD_COUNT - m_AllocatedTXFIFOWords;

        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Allocated {} FIFO bytes at offset {} for EP {}.", fifoSizeWords * 4, fifoStart * sizeof(uint32_t), USB_ADDRESS_EPNUM(endpointDescriptor.bEndpointAddress));

        m_Port->DIEPTXF[epNum - 1] = (fifoSizeWords << USB_OTG_DIEPTXF_INEPTXFD_Pos) | fifoStart;

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
            if (size + m_AllocatedTXFIFOWords > USB_STM32::DMA_FIFO_USABLE_WORD_COUNT) {
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
    USBIRQDisabler irqDisabler(*m_Driver);

    if (!m_ResetComplete) {
        return;
    }

    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointAddr);

    if (GetEndpointTranferState(endpointAddr)->EndpointMaxSize == 0) {
        return;
    }
    if (!EndpointDisable(endpointAddr, false))
    {
        RequestRecovery();
        return;
    }
    CancelEndpointTransfer(endpointAddr);

    if (endpointAddr & USB_ADDRESS_DIR_IN)
    {
        m_TransferStatusIn[epNum].Reset();

        const uint32_t fifoSize = (m_Port->DIEPTXF[epNum - 1] & USB_OTG_DIEPTXF_INEPTXFD_Msk) >> USB_OTG_DIEPTXF_INEPTXFD_Pos;
        const uint32_t fifoStart = (m_Port->DIEPTXF[epNum - 1] & USB_OTG_DIEPTXF_INEPTXSA_Msk) >> USB_OTG_DIEPTXF_INEPTXSA_Pos;

        // FIXME: support closing endpoints in any order.
        if (fifoStart == USB_STM32::DMA_FIFO_USABLE_WORD_COUNT - m_AllocatedTXFIFOWords)
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
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///
/// Close all non-control endpoints, cancel any pending transfers.
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointCloseAll()
{
    USBIRQDisabler irqDisabler(*m_Driver);

    if (!m_ResetComplete) {
        return;
    }

    // Disable interrupts for non-control endpoints.
    m_Device->DAINTMSK = (0x01 << USB_OTG_DAINTMSK_OEPM_Pos) | (0x01 << USB_OTG_DAINTMSK_IEPM_Pos);

    for (uint8_t endpointNumber = 1; endpointNumber < ENDPOINT_COUNT; ++endpointNumber)
    {
        if (m_TransferStatusOut[endpointNumber].EndpointMaxSize != 0
            && !EndpointDisable(USB_MK_OUT_ADDRESS(endpointNumber), false))
        {
            RequestRecovery();
            return;
        }
        CancelEndpointTransfer(USB_MK_OUT_ADDRESS(endpointNumber));
        m_OutEndpoints[endpointNumber].DOEPCTL = 0;
        m_OutEndpoints[endpointNumber].DOEPTSIZ = 0;
        m_OutEndpoints[endpointNumber].DOEPDMA = 0;
        m_TransferStatusOut[endpointNumber].Reset();

        if (m_TransferStatusIn[endpointNumber].EndpointMaxSize != 0
            && !EndpointDisable(USB_MK_IN_ADDRESS(endpointNumber), false))
        {
            RequestRecovery();
            return;
        }
        CancelEndpointTransfer(USB_MK_IN_ADDRESS(endpointNumber));
        m_InEndpoints[endpointNumber].DIEPCTL = 0;
        m_InEndpoints[endpointNumber].DIEPTSIZ = 0;
        m_InEndpoints[endpointNumber].DIEPDMA = 0;
        m_TransferStatusIn[endpointNumber].Reset();
    }

    // Reset TX FIFO allocation.
    m_AllocatedTXFIFOWords = 16;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength, size_t receiveCapacity)
{
    EndpointTransferState* transfer = GetEndpointTranferState(endpointAddr);

    if (totalLength != 0 && buffer == nullptr) {
        return false;
    }
    if ((endpointAddr & USB_ADDRESS_DIR_IN) == 0 && receiveCapacity != 0 && receiveCapacity < totalLength) {
        return false;
    }

    uint32_t transferGeneration;
    {
        USBIRQDisabler irqDisabler(*m_Driver);

        if (!m_DeviceReady || transfer == nullptr || transfer->EndpointMaxSize == 0 || transfer->TransferActive) {
            return false;
        }

        transfer->ResetTransfer();
        transfer->Buffer = static_cast<uint8_t*>(buffer);
        transfer->BufferSize = totalLength;
        transfer->ReceiveCapacity = receiveCapacity;
        transfer->TransferActive = true;
        transferGeneration = transfer->Generation;
    }

    if (!StartDMATransfer(endpointAddr, transferGeneration))
    {
        USBIRQDisabler irqDisabler(*m_Driver);
        if (transfer->Generation == transferGeneration) {
            transfer->ResetTransfer();
        }
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::SetAddress(uint8_t deviceAddr)
{
    USBIRQDisabler irqDisabler(*m_Driver);

    if (m_DeviceReady) {
        set_bit_group(m_Device->DCFG, USB_OTG_DCFG_DAD_Msk, deviceAddr << USB_OTG_DCFG_DAD_Pos);
    }
    return m_DeviceReady;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::ActivateRemoteWakeup(bool activate)
{
    USBIRQDisabler irqDisabler(*m_Driver);
    if (!m_DeviceReady) {
        return false;
    }

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

void USBDevice_STM32::ConfigureDevice()
{
    if (m_EnableVBusSense)
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
    const uint32_t interruptMask = USB_OTG_GINTMSK_MMISM | USB_OTG_GINTMSK_OTGINT
        | USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_USBSUSPM | USB_OTG_GINTMSK_WUIM | (m_UseSOF ? USB_OTG_GINTMSK_SOFM : 0);
    m_Port->GINTMSK = interruptMask;

    // Full speed using internal FS PHY.
    SetSpeed(USB_Speed::FULL);
    // Send a STALL handshake on a nonzero-length status OUT transaction.
    m_Device->DCFG |= USB_OTG_DCFG_NZLSOHSK;

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::RequestRecovery()
{
    USBIRQDisabler irqDisabler(*m_Driver);
    if (!m_RecoveryPending)
    {
        ++m_DeviceGeneration;
        m_DeviceReady = false;
        m_RecoveryPending = true;
        m_ResetComplete = false;
        DeviceDisconnect();
        m_Port->GINTMSK = 0;
        // The RCC reset is the ownership barrier when an endpoint cannot confirm that DMA has stopped.
        m_Driver->HoldCoreInReset();
        CancelAllEndpointTransfers();
        for (size_t i = 0; i < ENDPOINT_COUNT; ++i)
        {
            m_TransferStatusIn[i].Reset();
            m_TransferStatusOut[i].Reset();
        }
        m_Driver->IRQDeviceRecoveryNeeded();
    }
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
    const uint32_t calculatedWordCount = 15 + 2 * (maxEndpointSize / 4) + 2 * ENDPOINT_COUNT;
    return std::max(USB_DEVICE_MINIMUM_DMA_RX_FIFO_WORD_COUNT, calculatedWordCount);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::ResetReceived()
{
    // Reject old thread-context work immediately, including the interval before ENUMDNE.
    ++m_DeviceGeneration;
    m_DeviceReady = false;
    m_ResetComplete = false;
    m_Driver->IRQBusResetStarted();

    // Drop any global NAK state left by an interrupted endpoint-disable sequence.
    m_Device->DCTL |= USB_OTG_DCTL_CGINAK | USB_OTG_DCTL_CGONAK;
    m_Port->GINTSTS = USB_OTG_GINTSTS_GINAKEFF | USB_OTG_GINTSTS_BOUTNAKEFF;

    m_Device->DAINTMSK = 0;
    m_Device->DOEPMSK = 0;
    m_Device->DIEPMSK = 0;
    m_Device->DIEPEMPMSK = 0;

    // Stop accepting OUT data while terminating the previous IN transfers.
    for (uint32_t endpointIndex = 0; endpointIndex < ENDPOINT_COUNT; ++endpointIndex) {
        m_OutEndpoints[endpointIndex].DOEPCTL |= USB_OTG_DOEPCTL_SNAK;
    }

    // RM0433: stop IN transfers before flushing or reprogramming their FIFOs.
    if (!DisableInEndpointsFromIRQ())
    {
        RequestRecovery();
        return;
    }

    CancelAllEndpointTransfers();

    for (uint8_t endpointNumber = 0; endpointNumber < ENDPOINT_COUNT; ++endpointNumber)
    {
        m_TransferStatusIn[endpointNumber].Reset();
        m_TransferStatusOut[endpointNumber].Reset();
    }
    m_ControlRequestPackage = {};

    if (!FlushTxFifoFromIRQ(USB_DEVICE_ALL_TX_FIFOS))
    {
        RequestRecovery();
        return;
    }
    if (!FlushRxFifoFromIRQ())
    {
        RequestRecovery();
        return;
    }

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

    // EPENA has already been cleared by the completed IN endpoint-disable handshake.
    m_InEndpoints[0].DIEPCTL &= ~USB_OTG_DIEPCTL_STALL;
    m_OutEndpoints[0].DOEPCTL &= ~USB_DEVICE_OUT_ENDPOINT0_RESET_CLEAR_MASK;

    // Clear device address.
    m_Device->DCFG &= ~USB_OTG_DCFG_DAD_Msk;

    // NAK all OUT endpoints.
    for (uint32_t endpointIndex = 0; endpointIndex < ENDPOINT_COUNT; ++endpointIndex) {
        m_OutEndpoints[endpointIndex].DOEPCTL |= USB_OTG_DOEPCTL_SNAK;
    }

    // Enable interrupts.
    m_Device->DAINTMSK = (1 << USB_OTG_DAINTMSK_OEPM_Pos) | (1 << USB_OTG_DAINTMSK_IEPM_Pos);
    m_Device->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM | USB_OTG_DOEPMSK_AHBERRM
        | USB_OTG_DOEPMSK_OPEM | USB_OTG_DOEPMSK_BERRM;
    m_Device->DIEPMSK = USB_OTG_DIEPMSK_TOM | USB_OTG_DIEPMSK_XFRCM | USB_OTG_DIEPMSK_TXFURM | USB_OTG_DIEPMSK_BIM;
    m_Device->DIEPEMPMSK = 0;

    m_Port->GRXFSIZ = CalculateRXFIFOSize(m_SupportHighSpeed ? 512 : 64);

    m_AllocatedTXFIFOWords = 16;

    // Control IN uses FIFO 0 with 64 bytes.
    m_Port->DIEPTXF0_HNPTXFSIZ = (16 << USB_OTG_TX0FD_Pos)
        | (USB_STM32::DMA_FIFO_USABLE_WORD_COUNT - m_AllocatedTXFIFOWords);

    // Set control endpoint0 size to 64 bytes.
    m_InEndpoints[0].DIEPCTL &= ~USB_OTG_DIEPCTL_MPSIZ_Msk;
    m_TransferStatusOut[0].EndpointMaxSize = 64;
    m_TransferStatusIn[0].EndpointMaxSize = 64;

    PrepareSetupPackets();

    m_Port->GINTMSK |= USB_OTG_GINTMSK_OEPINT | USB_OTG_GINTMSK_IEPINT;
    m_ResetComplete = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::DisableInEndpointsFromIRQ()
{
    uint32_t enabledEndpoints = 0;
    for (uint8_t endpointNumber = 0; endpointNumber < ENDPOINT_COUNT; ++endpointNumber)
    {
        USB_OTG_INEndpointTypeDef& endpoint = m_InEndpoints[endpointNumber];
        if ((endpoint.DIEPCTL & USB_OTG_DIEPCTL_EPENA) != 0)
        {
            enabledEndpoints |= 1u << endpointNumber;
            endpoint.DIEPINT = USB_OTG_DIEPINT_EPDISD;
            endpoint.DIEPCTL |= USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_EPDIS;
        }
    }

    for (uint8_t endpointNumber = 0; endpointNumber < ENDPOINT_COUNT; ++endpointNumber)
    {
        if ((enabledEndpoints & (1u << endpointNumber)) != 0)
        {
            USB_OTG_INEndpointTypeDef& endpoint = m_InEndpoints[endpointNumber];
            if (!WaitForRegisterBitsSetFromIRQ(endpoint.DIEPINT, USB_OTG_DIEPINT_EPDISD)
                || (endpoint.DIEPCTL & USB_OTG_DIEPCTL_EPENA) != 0) {
                return false;
            }
            endpoint.DIEPINT = USB_OTG_DIEPINT_EPDISD;
        }
    }

    return true;
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

bool USBDevice_STM32::EndpointDisable(uint8_t endpointAddr, bool stall)
{
    const uint8_t epNum = USB_ADDRESS_EPNUM(endpointAddr);

    if (endpointAddr & USB_ADDRESS_DIR_IN)
    {
        // IN endpoint 0 supports the same disable handshake as the other IN endpoints.
        if ((m_InEndpoints[epNum].DIEPCTL & USB_OTG_DIEPCTL_EPENA) == 0)
        {
            m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_SNAK | (stall ? USB_OTG_DIEPCTL_STALL : 0);
        }
        else
        {
            // Stop transmitting packets and NAK IN transfers.
            m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_INEPNE;
            m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_SNAK;
            if (!WaitForRegisterBitsSet(m_InEndpoints[epNum].DIEPINT, USB_OTG_DIEPINT_INEPNE))
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for IN endpoint {:02x} NAK.", endpointAddr);
                return false;
            }

            // Disable the endpoint.
            m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_EPDISD;
            m_InEndpoints[epNum].DIEPCTL |= USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK | (stall ? USB_OTG_DIEPCTL_STALL : 0);
            if (WaitForRegisterBitsSet(m_InEndpoints[epNum].DIEPINT, USB_OTG_DIEPINT_EPDISD_Msk))
            {
                m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_EPDISD;
            }
            else
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for IN endpoint {:02x} disable.", endpointAddr);
                return false;
            }
        }

        // Flush the FIFO, and wait until we have confirmed it cleared.
        if (!m_Driver->FlushTxFifo(epNum))
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout flushing IN endpoint {:02x} TX FIFO.", endpointAddr);
            return false;
        }
        // Discard completion/error status belonging to the transfer that was stopped.
        m_InEndpoints[epNum].DIEPINT = USB_OTG_DIEPINT_XFRC | USB_DEVICE_IN_ENDPOINT_DMA_ERROR_MASK;
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
            if (!WaitForRegisterBitsSet(m_Port->GINTSTS, USB_OTG_GINTSTS_BOUTNAKEFF_Msk))
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for global OUT NAK before disabling endpoint {:02x}.", endpointAddr);
                return false;
            }

            // Disable the endpoint.
            m_OutEndpoints[epNum].DOEPINT = USB_OTG_DOEPINT_EPDISD;
            m_OutEndpoints[epNum].DOEPCTL |= USB_OTG_DOEPCTL_EPDIS | (stall ? USB_OTG_DOEPCTL_STALL : 0);
            if (WaitForRegisterBitsSet(m_OutEndpoints[epNum].DOEPINT, USB_OTG_DOEPINT_EPDISD_Msk))
            {
                m_OutEndpoints[epNum].DOEPINT = USB_OTG_DOEPINT_EPDISD;
            }
            else
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Timeout waiting for OUT endpoint {:02x} disable.", endpointAddr);
                return false;
            }

            // Allow other OUT endpoints to keep receiving.
            m_Port->GINTSTS = USB_OTG_GINTSTS_BOUTNAKEFF;
            m_Device->DCTL |= USB_OTG_DCTL_CGONAK;
        }
        // Preserve SETUP status on EP0 while discarding the canceled transfer status.
        m_OutEndpoints[epNum].DOEPINT = USB_OTG_DOEPINT_XFRC | USB_DEVICE_OUT_ENDPOINT_DMA_ERROR_MASK;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::StartDMATransfer(uint8_t endpointAddr, uint32_t transferGeneration)
{
    const uint8_t endpointNumber = USB_ADDRESS_EPNUM(endpointAddr);
    const bool directionIn = (endpointAddr & USB_ADDRESS_DIR_IN) != 0;
    EndpointTransferState* transfer = GetEndpointTranferState(endpointAddr);

    size_t endpointMaxSize;
    size_t remainingLength;
    size_t remainingCapacity;
    uint8_t* callerBuffer;
    {
        USBIRQDisabler irqDisabler(*m_Driver);

        if (!m_DeviceReady || transfer == nullptr || transfer->Generation != transferGeneration || !transfer->TransferActive
            || transfer->DMATransferActive || transfer->EndpointMaxSize == 0 || transfer->BytesTransferred > transfer->BufferSize)
        {
            return false;
        }

        endpointMaxSize = transfer->EndpointMaxSize;
        remainingLength = transfer->BufferSize - transfer->BytesTransferred;
        remainingCapacity = (transfer->ReceiveCapacity > transfer->BytesTransferred)
            ? transfer->ReceiveCapacity - transfer->BytesTransferred : 0;
        callerBuffer = (remainingLength != 0) ? transfer->Buffer + transfer->BytesTransferred : nullptr;
    }

    size_t requestedPacketCount = 1;
    if (remainingLength != 0) {
        requestedPacketCount = 1 + (remainingLength - 1) / endpointMaxSize;
    }

    const size_t hardwarePacketCount = directionIn
        ? USB_OTG_DIEPTSIZ_PKTCNT_Msk >> USB_OTG_DIEPTSIZ_PKTCNT_Pos
        : USB_OTG_DOEPTSIZ_PKTCNT_Msk >> USB_OTG_DOEPTSIZ_PKTCNT_Pos;
    const size_t hardwareTransferSize = directionIn
        ? USB_OTG_DIEPTSIZ_XFRSIZ_Msk >> USB_OTG_DIEPTSIZ_XFRSIZ_Pos
        : USB_OTG_DOEPTSIZ_XFRSIZ_Msk >> USB_OTG_DOEPTSIZ_XFRSIZ_Pos;

    size_t maximumPacketCount = std::min(hardwarePacketCount, hardwareTransferSize / endpointMaxSize);
    if (endpointNumber == 0) {
        maximumPacketCount = std::min<size_t>(maximumPacketCount, 1);
    }
    if (maximumPacketCount == 0) {
        return false;
    }

    size_t directDataLength = std::min(remainingLength, maximumPacketCount * endpointMaxSize);
    if (!directionIn) {
        directDataLength = USB_STM32::GetDirectDMAReceiveLength(
            callerBuffer, directDataLength, endpointMaxSize, remainingCapacity);
    }
    const bool useDirectDMA = directDataLength != 0
        && (!directionIn || USB_STM32::IsDirectDMATransmitBuffer(callerBuffer, directDataLength));

    size_t packetCount;
    if (useDirectDMA)
    {
        packetCount = 1 + (directDataLength - 1) / endpointMaxSize;
    }
    else
    {
        const size_t bouncePacketCount = DMA_BOUNCE_BUFFER_SIZE / endpointMaxSize;
        if (bouncePacketCount == 0) {
            return false;
        }
        packetCount = std::min(requestedPacketCount, std::min(maximumPacketCount, bouncePacketCount));
    }
    const size_t transferDataLength = std::min(remainingLength, packetCount * endpointMaxSize);
    const size_t dmaTransferSize = directionIn ? transferDataLength : packetCount * endpointMaxSize;

    uint8_t* dmaTransferBuffer = useDirectDMA ? callerBuffer : GetDMABounceBuffer(endpointAddr);
    if (!useDirectDMA && directionIn && transferDataLength != 0) {
        std::memcpy(dmaTransferBuffer, callerBuffer, transferDataLength);
    }

    const size_t cacheLength = align_up(dmaTransferSize, __SCB_DCACHE_LINE_SIZE);
    if (cacheLength != 0) {
        if (directionIn) {
            USB_STM32::CleanDMATransmitBuffer(dmaTransferBuffer, dmaTransferSize);
        } else {
            SCB_CleanInvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(dmaTransferBuffer), static_cast<int32_t>(cacheLength));
        }
    }

    {
        USBIRQDisabler irqDisabler(*m_Driver);

        if (!m_DeviceReady || transfer->Generation != transferGeneration || !transfer->TransferActive
            || transfer->DMATransferActive) {
            return false;
        }

        transfer->DMATransferBuffer = dmaTransferBuffer;
        transfer->DMATransferSize = dmaTransferSize;
        transfer->DMATransferDataLength = transferDataLength;
        transfer->DMATransferActive = true;
        transfer->DMAUsesBounceBuffer = !useDirectDMA;

        EndpointSchedulePackets(endpointAddr, static_cast<uint32_t>(packetCount), static_cast<uint32_t>(dmaTransferSize));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice_STM32::FinishDMATransfer(uint8_t endpointAddr, bool commitTransfer, bool* shortPacketReceived)
{
    EndpointTransferState* transfer = GetEndpointTranferState(endpointAddr);
    if (shortPacketReceived != nullptr) {
        *shortPacketReceived = false;
    }
    if (transfer == nullptr || !transfer->DMATransferActive) {
        return false;
    }

    const uint8_t endpointNumber = USB_ADDRESS_EPNUM(endpointAddr);
    const bool directionIn = (endpointAddr & USB_ADDRESS_DIR_IN) != 0;
    const size_t dmaTransferSize = transfer->DMATransferSize;
    const size_t transferDataLength = transfer->DMATransferDataLength;
    uint8_t* dmaTransferBuffer = transfer->DMATransferBuffer;
    const bool usesBounceBuffer = transfer->DMAUsesBounceBuffer;

    if (!directionIn && dmaTransferSize != 0) {
        SCB_InvalidateDCache_by_Addr(
            reinterpret_cast<uint32_t*>(dmaTransferBuffer),
            static_cast<int32_t>(align_up(dmaTransferSize, __SCB_DCACHE_LINE_SIZE))
        );
    }

    transfer->DMATransferBuffer = nullptr;
    transfer->DMATransferSize = 0;
    transfer->DMATransferDataLength = 0;
    transfer->DMATransferActive = false;
    transfer->DMAUsesBounceBuffer = false;

    if (!commitTransfer) {
        return true;
    }
    uint32_t residualLength;
    if (directionIn) {
        residualLength = (m_InEndpoints[endpointNumber].DIEPTSIZ & USB_OTG_DIEPTSIZ_XFRSIZ_Msk) >> USB_OTG_DIEPTSIZ_XFRSIZ_Pos;
    } else {
        residualLength = (m_OutEndpoints[endpointNumber].DOEPTSIZ & USB_OTG_DOEPTSIZ_XFRSIZ_Msk) >> USB_OTG_DOEPTSIZ_XFRSIZ_Pos;
    }

    if (residualLength > dmaTransferSize) {
        return false;
    }

    // An OUT ZLP reserves one max-packet DMA window, but no payload bytes belong to the logical transfer.
    const size_t transferredLength = (transferDataLength == 0) ? 0 : dmaTransferSize - residualLength;
    if ((directionIn && transferredLength != transferDataLength) || transferredLength > transferDataLength) {
        return false;
    }
    if (transfer->BytesTransferred > transfer->BufferSize
        || transferredLength > transfer->BufferSize - transfer->BytesTransferred)
    {
        return false;
    }

    if (!directionIn && usesBounceBuffer && transferredLength != 0) {
        std::memcpy(transfer->Buffer + transfer->BytesTransferred, dmaTransferBuffer, transferredLength);
    }
    transfer->BytesTransferred += transferredLength;

    if (shortPacketReceived != nullptr) {
        *shortPacketReceived = !directionIn && transferredLength < dmaTransferSize;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::CancelEndpointTransfer(uint8_t endpointAddr)
{
    USBIRQDisabler irqDisabler(*m_Driver);

    EndpointTransferState* transfer = GetEndpointTranferState(endpointAddr);
    if (transfer == nullptr) {
        return;
    }
    if (transfer->DMATransferActive) {
        (void)FinishDMATransfer(endpointAddr, false, nullptr);
    }
    transfer->ResetTransfer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::CancelAllEndpointTransfers()
{
    for (uint8_t endpointNumber = 0; endpointNumber < ENDPOINT_COUNT; ++endpointNumber)
    {
        CancelEndpointTransfer(USB_MK_IN_ADDRESS(endpointNumber));
        CancelEndpointTransfer(USB_MK_OUT_ADDRESS(endpointNumber));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::PrepareSetupPackets()
{
    // Newer DWC2 cores retain the active EP0 setup receive across conditions
    // where software may try to rearm it. Do not rewrite active DMA registers.
    if (m_Port->GSNPSID > USB_DEVICE_CORE_ID_300A
        && (m_OutEndpoints[0].DOEPCTL & USB_OTG_DOEPCTL_EPENA) != 0) {
        return;
    }

    uint8_t* setupBuffer = GetDMABounceBuffer(USB_MK_OUT_ADDRESS(0));
    constexpr size_t setupCacheLength = align_up(USB_DEVICE_ENDPOINT0_SETUP_DMA_BUFFER_SIZE, __SCB_DCACHE_LINE_SIZE);

    SCB_CleanInvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(setupBuffer), static_cast<int32_t>(setupCacheLength));

    m_OutEndpoints[0].DOEPTSIZ = USB_DEVICE_ENDPOINT0_SETUP_TRANSFER_CONFIG;
    m_OutEndpoints[0].DOEPDMA = reinterpret_cast<uintptr_t>(setupBuffer);
    m_OutEndpoints[0].DOEPCTL |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_USBAEP;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::EndpointSchedulePackets(uint8_t endpointAddr, uint32_t packetCount, uint32_t totalLength)
{
    const uint8_t endpointNumber = USB_ADDRESS_EPNUM(endpointAddr);
    EndpointTransferState* transfer = GetEndpointTranferState(endpointAddr);
    kassert(transfer != nullptr);

    if (endpointAddr & USB_ADDRESS_DIR_IN)
    {
        m_InEndpoints[endpointNumber].DIEPTSIZ = (packetCount << USB_OTG_DIEPTSIZ_PKTCNT_Pos)
            | ((totalLength << USB_OTG_DIEPTSIZ_XFRSIZ_Pos) & USB_OTG_DIEPTSIZ_XFRSIZ_Msk);
        m_InEndpoints[endpointNumber].DIEPDMA = reinterpret_cast<uintptr_t>(transfer->DMATransferBuffer);

        if (totalLength == 0)
        {
            // STM32H743 erratum 2.25.7 requires a delayed SNAK/CNAK sequence for DMA IN ZLP transfers.
            uint32_t endpointControl = m_InEndpoints[endpointNumber].DIEPCTL;
            endpointControl &= ~(USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_CNAK);

            m_InEndpoints[endpointNumber].DIEPCTL = endpointControl | USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_SNAK;
            __DSB();
            const uint32_t delayStart = DWT->CYCCNT;
            while (DWT->CYCCNT - delayStart < USB_DEVICE_IN_ZLP_ENABLE_DELAY_CPU_CYCLES) {
                __NOP();
            }
            m_InEndpoints[endpointNumber].DIEPCTL = endpointControl | USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK;
        }
        else
        {
            m_InEndpoints[endpointNumber].DIEPCTL |= USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK;
        }

        if (USB_TransferType((m_InEndpoints[endpointNumber].DIEPCTL & USB_OTG_DIEPCTL_EPTYP_Msk) >> USB_OTG_DIEPCTL_EPTYP_Pos) == USB_TransferType::ISOCHRONOUS && transfer->Interval == 1)
        {
            // For ISOCHRONOUS endpoint set correct odd/even bit for next frame.
            const bool currentFrameOdd = (m_Device->DSTS & (1 << USB_OTG_DSTS_FNSOF_Pos)) != 0; // Frame counter bit 0.
            m_InEndpoints[endpointNumber].DIEPCTL |= currentFrameOdd ? USB_OTG_DIEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DIEPCTL_SODDFRM_Msk;
        }
    }
    else
    {
        m_OutEndpoints[endpointNumber].DOEPTSIZ = (packetCount << USB_OTG_DOEPTSIZ_PKTCNT_Pos)
            | ((totalLength << USB_OTG_DOEPTSIZ_XFRSIZ_Pos) & USB_OTG_DOEPTSIZ_XFRSIZ_Msk);
        m_OutEndpoints[endpointNumber].DOEPDMA = reinterpret_cast<uintptr_t>(transfer->DMATransferBuffer);
        m_OutEndpoints[endpointNumber].DOEPCTL |= USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK;

        if (USB_TransferType((m_OutEndpoints[endpointNumber].DOEPCTL & USB_OTG_DOEPCTL_EPTYP) >> USB_OTG_DIEPCTL_EPTYP_Pos) == USB_TransferType::ISOCHRONOUS && transfer->Interval == 1)
        {
            // For ISOCHRONOUS endpoint set correct odd/even bit for next frame.
            const bool currentFrameOdd = (m_Device->DSTS & (1 << USB_OTG_DSTS_FNSOF_Pos)) != 0; // Frame counter bit 0.
            m_OutEndpoints[endpointNumber].DOEPCTL |= currentFrameOdd ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DOEPCTL_SODDFRM_Msk;
        }
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
    if (m_RecoveryPending) {
        return IRQResult::HANDLED;
    }
    const uint32_t intStatus = m_Port->GINTSTS & m_Port->GINTMSK;

    if (intStatus & USB_OTG_GINTMSK_MMISM)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_MMIS;
    }
    if (intStatus & USB_OTG_GINTSTS_USBRST)
    {
        ResetReceived();
        if (m_RecoveryPending) {
            return IRQResult::HANDLED;
        }
        m_Port->GINTSTS = USB_OTG_GINTSTS_USBRST;
        return IRQResult::HANDLED;
    }
    if (intStatus & USB_OTG_GINTSTS_ENUMDNE)   // ENUMDNE indicates the end of reset on the USB. Speed has been detected.
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;
        const USB_Speed speed = DeviceGetSpeed();
        SetTurnaround(speed);
        if (m_ResetComplete) {
            m_Driver->IRQBusReset(speed, m_DeviceGeneration);
        }
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
        m_Port->GOTGINT = otgInt;

        // Ignore a stale session-end interrupt if VBUS has already returned.
        if ((otgInt & USB_OTG_GOTGINT_SEDET) != 0 && (m_Port->GOTGCTL & USB_OTG_GOTGCTL_BSESVLD) == 0)
        {
            RequestRecovery();
            return IRQResult::HANDLED;
        }
    }
    if (intStatus & USB_OTG_GINTSTS_SOF)
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_SOF;
        m_Driver->IRQStartOfFrame();
    }
    if (intStatus & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT)   // Incomplete periodic transfer
    {
        m_Port->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT;
    }
    if (intStatus & USB_OTG_GINTSTS_OEPINT) {
        HandleOutEndpointIRQ();
    }
    if (m_RecoveryPending) {
        return IRQResult::HANDLED;
    }
    if (intStatus & USB_OTG_GINTSTS_IEPINT) {
        HandleInEndpointIRQ();
    }
    if (m_RecoveryPending) {
        return IRQResult::HANDLED;
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

void USBDevice_STM32::HandleOutEndpointIRQ()
{
    for (uint8_t endpointNumber = 0; endpointNumber < ENDPOINT_COUNT; ++endpointNumber)
    {
        if (m_Device->DAINT & (1 << (USB_OTG_DAINT_OEPINT_Pos + endpointNumber)))
        {
            EndpointTransferState& transfer = m_TransferStatusOut[endpointNumber];
            const uint32_t endpointInterrupts = m_OutEndpoints[endpointNumber].DOEPINT;
            const uint32_t unhandledInterrupts = endpointInterrupts
                & USB_DEVICE_OUT_ENDPOINT_INTERRUPT_CLEAR_MASK
                & ~(USB_OTG_DOEPINT_STUP | USB_OTG_DOEPINT_XFRC);
            if (unhandledInterrupts != 0) {
                m_OutEndpoints[endpointNumber].DOEPINT = unhandledInterrupts;
            }

            if (endpointInterrupts & USB_OTG_DOEPINT_STUP)
            {
                // XFRC can belong to the setup transaction. Clear it before scheduling a new EP0 transfer.
                m_OutEndpoints[endpointNumber].DOEPINT =
                    endpointInterrupts & (USB_OTG_DOEPINT_STUP | USB_OTG_DOEPINT_XFRC);
                if (endpointNumber == 0)
                {
                    CancelEndpointTransfer(USB_MK_IN_ADDRESS(0));
                    CancelEndpointTransfer(USB_MK_OUT_ADDRESS(0));

                    uint8_t* setupBuffer = GetDMABounceBuffer(USB_MK_OUT_ADDRESS(0));
                    constexpr size_t setupCacheLength = align_up(USB_DEVICE_ENDPOINT0_SETUP_DMA_BUFFER_SIZE, __SCB_DCACHE_LINE_SIZE);
                    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(setupBuffer), static_cast<int32_t>(setupCacheLength));

                    std::memcpy(&m_ControlRequestPackage, setupBuffer, sizeof(m_ControlRequestPackage));

                    if (m_ControlRequestPackage.wLength == 0) {
                        PrepareSetupPackets();
                    }
                    m_Driver->IRQControlRequestReceived(m_ControlRequestPackage);
                }
                continue;
            }

            if (endpointInterrupts & USB_DEVICE_OUT_ENDPOINT_DMA_ERROR_MASK)
            {
                if (transfer.TransferActive)
                {
                    RequestRecovery();
                    return;
                }
                continue;
            }

            if (endpointInterrupts & USB_OTG_DOEPINT_XFRC)
            {
                m_OutEndpoints[endpointNumber].DOEPINT = USB_OTG_DOEPINT_XFRC;
                if (!transfer.TransferActive || !transfer.DMATransferActive)
                {
                    // An orphan EP0 completion still consumes its receive window.
                    if (endpointNumber == 0) {
                        PrepareSetupPackets();
                    }
                    continue;
                }

                bool shortPacketReceived;
                if (!FinishDMATransfer(USB_MK_OUT_ADDRESS(endpointNumber), true, &shortPacketReceived))
                {
                    const uint32_t transferLength = static_cast<uint32_t>(transfer.BytesTransferred);
                    transfer.ResetTransfer();
                    if (endpointNumber == 0) {
                        PrepareSetupPackets();
                    }
                    m_Driver->IRQTransferComplete(USB_MK_OUT_ADDRESS(endpointNumber), transferLength, USB_TransferResult::Failed);
                    continue;
                }

                if (!shortPacketReceived && transfer.BytesTransferred < transfer.BufferSize)
                {
                    if (!StartDMATransfer(USB_MK_OUT_ADDRESS(endpointNumber), transfer.Generation))
                    {
                        const uint32_t transferLength = static_cast<uint32_t>(transfer.BytesTransferred);
                        transfer.ResetTransfer();
                        if (endpointNumber == 0) {
                            PrepareSetupPackets();
                        }
                        m_Driver->IRQTransferComplete(USB_MK_OUT_ADDRESS(endpointNumber), transferLength, USB_TransferResult::Failed);
                    }
                    continue;
                }

                const uint32_t transferLength = static_cast<uint32_t>(transfer.BytesTransferred);
                transfer.ResetTransfer();
                if (endpointNumber == 0) {
                    PrepareSetupPackets();
                }
                m_Driver->IRQTransferComplete(USB_MK_OUT_ADDRESS(endpointNumber), transferLength, USB_TransferResult::Success);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice_STM32::HandleInEndpointIRQ()
{
    for (uint8_t endpointNumber = 0; endpointNumber < ENDPOINT_COUNT; ++endpointNumber)
    {
        if (m_Device->DAINT & (1 << (USB_OTG_DAINT_IEPINT_Pos + endpointNumber)))
        {
            EndpointTransferState& transfer = m_TransferStatusIn[endpointNumber];
            const uint32_t endpointInterrupts = m_InEndpoints[endpointNumber].DIEPINT;
            const uint32_t unhandledInterrupts = endpointInterrupts
                & USB_DEVICE_IN_ENDPOINT_INTERRUPT_CLEAR_MASK
                & ~USB_OTG_DIEPINT_XFRC;
            if (unhandledInterrupts != 0) {
                m_InEndpoints[endpointNumber].DIEPINT = unhandledInterrupts;
            }

            if (endpointInterrupts & USB_DEVICE_IN_ENDPOINT_DMA_ERROR_MASK)
            {
                if (transfer.TransferActive)
                {
                    RequestRecovery();
                    return;
                }
                continue;
            }

            if (endpointInterrupts & USB_OTG_DIEPINT_XFRC)
            {
                m_InEndpoints[endpointNumber].DIEPINT = USB_OTG_DIEPINT_XFRC;
                if (!transfer.TransferActive || !transfer.DMATransferActive) {
                    continue;
                }

                if (!FinishDMATransfer(USB_MK_IN_ADDRESS(endpointNumber), true, nullptr))
                {
                    const uint32_t transferLength = static_cast<uint32_t>(transfer.BytesTransferred);
                    transfer.ResetTransfer();
                    m_Driver->IRQTransferComplete(USB_MK_IN_ADDRESS(endpointNumber), transferLength, USB_TransferResult::Failed);
                    continue;
                }

                if (transfer.BytesTransferred < transfer.BufferSize)
                {
                    if (!StartDMATransfer(USB_MK_IN_ADDRESS(endpointNumber), transfer.Generation))
                    {
                        const uint32_t transferLength = static_cast<uint32_t>(transfer.BytesTransferred);
                        transfer.ResetTransfer();
                        m_Driver->IRQTransferComplete(USB_MK_IN_ADDRESS(endpointNumber), transferLength, USB_TransferResult::Failed);
                    }
                    continue;
                }

                const bool rearmSetupPackets = endpointNumber == 0 && transfer.BufferSize == 0;
                const uint32_t transferLength = static_cast<uint32_t>(transfer.BytesTransferred);
                transfer.ResetTransfer();
                if (rearmSetupPackets) {
                    PrepareSetupPackets();
                }
                m_Driver->IRQTransferComplete(USB_MK_IN_ADDRESS(endpointNumber), transferLength, USB_TransferResult::Success);
            }
        }
    }
}

} //namespace kernel
