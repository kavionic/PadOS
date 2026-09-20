// This file is part of PadOS.
//
// Copyright (C) 2022-2026 Kurt Skauen <http://kavionic.com/>
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

#include <algorithm>
#include <cstring>
#include <malloc.h>
#include <utility>
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
#include <iterator>

#include <Utils/String.h>
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
#include <Utils/Utils.h>
#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/HAL/STM32/USBHost_STM32.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/IRQDispatcher.h>

namespace kernel
{

// Each physical controller owns one lazily allocated buffer set for the kernel lifetime.
// Shutdown does not quiesce DMA or unregister IRQ callbacks, so published sets are never freed.
static uint8_t (*g_USBHostSTM32DMABounceBufferSets[2])[USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE] = {};
static_assert((USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE % __SCB_DCACHE_LINE_SIZE) == 0);

static bool IsUSBHostSTM32NonSplitPeriodicDMAChannel(const USB_OTG_HostChannelTypeDef& channelRegs)
{
    if ((channelRegs.HCSPLT & USB_OTG_HCSPLT_SPLITEN) != 0) {
        return false;
    }

    const USB_TransferType endpointType = USB_TransferType(
        (channelRegs.HCCHAR & USB_OTG_HCCHAR_EPTYP) >> USB_OTG_HCCHAR_EPTYP_Pos);
    return endpointType == USB_TransferType::INTERRUPT
        || endpointType == USB_TransferType::ISOCHRONOUS;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHost_STM32::USBHost_STM32()
    : m_ChannelHaltCondition("usb_host_halt")
{
}

#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
enum class USBHostSTM32DebugEntry : size_t
{
    Direction,
    EndpointType,
    Speed,
    MaxPacketSize,
    URBState,
    ChannelState,
    XferSize,
    RequestedTransferLength,
    BytesTransferred,
    ErrorCount,
    ToggleIn,
    ToggleOut,
    HCCHAR,
    HCSPLT,
    HCINT,
    HCINTMSK,
    HCTSIZ,
    HCDMA,
    SubmitRequestCount,
    StartTransferCount,
    TransferCompleteIRQCount,
    NakNyetIRQCount,
    ChannelHaltIRQCount,
    FrameOverrunIRQCount,
    AckIRQCount,
    StallIRQCount,
    TransactionErrorIRQCount,
    NotifyDoneCount,
    NotifyNotReadyCount,
    NotifyStallCount,
    NotifyErrorCount,
    Count
};

static constexpr const char* USBHOST_STM32_DEBUG_LABELS[] =
{
    "stm32.direction",
    "stm32.endpointType",
    "stm32.speed",
    "stm32.maxPacketSize",
    "stm32.urbState",
    "stm32.channelState",
    "stm32.xferSize",
    "stm32.requestedTransferLength",
    "stm32.bytesTransferred",
    "stm32.errorCount",
    "stm32.toggleIn",
    "stm32.toggleOut",
    "stm32.HCCHAR",
    "stm32.HCSPLT",
    "stm32.HCINT",
    "stm32.HCINTMSK",
    "stm32.HCTSIZ",
    "stm32.HCDMA",
    "stm32.submitRequestCount",
    "stm32.startTransferCount",
    "stm32.transferCompleteIRQCount",
    "stm32.nakNyetIRQCount",
    "stm32.channelHaltIRQCount",
    "stm32.frameOverrunIRQCount",
    "stm32.ackIRQCount",
    "stm32.stallIRQCount",
    "stm32.transactionErrorIRQCount",
    "stm32.notifyDoneCount",
    "stm32.notifyNotReadyCount",
    "stm32.notifyStallCount",
    "stm32.notifyErrorCount"
};

static constexpr size_t USBHOST_STM32_DEBUG_ENTRY_COUNT = std::size(USBHOST_STM32_DEBUG_LABELS);
static_assert(USBHOST_STM32_DEBUG_ENTRY_COUNT == std::to_underlying(USBHostSTM32DebugEntry::Count));

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool IsUSBHostSTM32PipeIndexValid(USB_PipeIndex pipeIndex)
{
    return pipeIndex >= 0 && static_cast<size_t>(pipeIndex) < USBHost_STM32::CHANNEL_COUNT;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static const char* GetUSBHostSTM32BoolName(bool value)
{
    return value ? "yes" : "no";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static const char* GetUSBHostSTM32RequestDirectionName(USB_RequestDirection direction)
{
    switch (direction)
    {
        case USB_RequestDirection::HOST_TO_DEVICE: return "host-to-device";
        case USB_RequestDirection::DEVICE_TO_HOST: return "device-to-host";
    }
    return "unknown";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static const char* GetUSBHostSTM32TransferTypeName(USB_TransferType transferType)
{
    switch (transferType)
    {
        case USB_TransferType::CONTROL:     return "control";
        case USB_TransferType::ISOCHRONOUS: return "isochronous";
        case USB_TransferType::BULK:        return "bulk";
        case USB_TransferType::INTERRUPT:   return "interrupt";
    }
    return "unknown";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static const char* GetUSBHostSTM32SpeedName(USB_Speed speed)
{
    switch (speed)
    {
        case USB_Speed::LOW:  return "low";
        case USB_Speed::FULL: return "full";
        case USB_Speed::HIGH: return "high";
    }
    return "unknown";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static const char* GetUSBHostSTM32URBStateName(USB_URBState state)
{
    switch (state)
    {
        case USB_URBState::Idle:     return "idle";
        case USB_URBState::Done:     return "done";
        case USB_URBState::NotReady: return "not-ready";
        case USB_URBState::Stall:    return "stall";
        case USB_URBState::Error:    return "error";
    }
    return "unknown";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static const char* GetUSBHostSTM32ChannelStateName(USB_HostChannelState state)
{
    switch (state)
    {
        case USB_HostChannelState::IDLE:       return "idle";
        case USB_HostChannelState::XFRC:       return "transfer-complete";
        case USB_HostChannelState::NAK:        return "nak";
        case USB_HostChannelState::STALL:      return "stall";
        case USB_HostChannelState::XACTERR:    return "transaction-error";
        case USB_HostChannelState::BBLERR:     return "babble-error";
        case USB_HostChannelState::DATATGLERR: return "data-toggle-error";
    }
    return "unknown";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void CountUSBHostSTM32ChannelInterrupts(USBHostChannelData& channel, uint32_t interrupts)
{
    if ((interrupts & USB_OTG_HCINT_XFRC) != 0) {
        ++channel.Diagnostics.TransferCompleteIRQCount;
    }
    if ((interrupts & (USB_OTG_HCINT_NAK | USB_OTG_HCINT_NYET)) != 0) {
        ++channel.Diagnostics.NakNyetIRQCount;
    }
    if ((interrupts & USB_OTG_HCINT_CHH) != 0) {
        ++channel.Diagnostics.ChannelHaltIRQCount;
    }
    if ((interrupts & USB_OTG_HCINT_FRMOR) != 0) {
        ++channel.Diagnostics.FrameOverrunIRQCount;
    }
    if ((interrupts & USB_OTG_HCINT_ACK) != 0) {
        ++channel.Diagnostics.AckIRQCount;
    }
    if ((interrupts & USB_OTG_HCINT_STALL) != 0) {
        ++channel.Diagnostics.StallIRQCount;
    }
    if ((interrupts & (USB_OTG_HCINT_AHBERR | USB_OTG_HCINT_BBERR | USB_OTG_HCINT_DTERR | USB_OTG_HCINT_TXERR)) != 0) {
        ++channel.Diagnostics.TransactionErrorIRQCount;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void IncrementUSBHostSTM32NotifyCounter(USBHostChannelDiagnostics& diagnostics, USB_URBState state)
{
    switch (state)
    {
        case USB_URBState::Done:
            ++diagnostics.NotifyDoneCount;
            break;
        case USB_URBState::NotReady:
            ++diagnostics.NotifyNotReadyCount;
            break;
        case USB_URBState::Stall:
            ++diagnostics.NotifyStallCount;
            break;
        case USB_URBState::Error:
            ++diagnostics.NotifyErrorCount;
            break;
        case USB_URBState::Idle:
            break;
    }
}
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense)
{
    if (driver == nullptr) {
        return false;
    }

    size_t dmaBufferSet;
    switch (portID)
    {
        case USB_OTG_ID::USB1_HS:
            dmaBufferSet = 0;
            break;
        case USB_OTG_ID::USB2_FS:
            dmaBufferSet = 1;
            break;
        default:
            return false;
    }
    if (m_Driver != nullptr && (m_Driver != driver || m_Port != get_usb_from_id(portID))) {
        return false;
    }

    m_Driver = driver;
    m_Port          = get_usb_from_id(portID);
    m_Host          = reinterpret_cast<USB_OTG_HostTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_HOST_BASE);
    m_HPRT          = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(m_Port) + USB_OTG_HOST_PORT_BASE);
    m_HostChannels  = reinterpret_cast<USB_OTG_HostChannelTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_HOST_CHANNEL_BASE);
    m_PCGCCTL       = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(m_Port) + USB_OTG_PCGCCTL_BASE);

    if (!InitializeController()) {
        return false;
    }

    auto& dmaBuffers = g_USBHostSTM32DMABounceBufferSets[dmaBufferSet];
    if (dmaBuffers == nullptr)
    {
        const size_t allocationSize = CHANNEL_COUNT * DMA_BOUNCE_BUFFER_SIZE;
        auto* allocation = static_cast<uint8_t (*)[DMA_BOUNCE_BUFFER_SIZE]>(memalign(__SCB_DCACHE_LINE_SIZE, allocationSize));
        if (allocation == nullptr)
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to allocate USB host DMA bounce buffers.");
            return false;
        }
        if (!USB_STM32::IsDirectDMAReceiveBuffer(allocation, allocationSize))
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "USB host DMA bounce buffers are not DMA accessible.");
            free(allocation);
            return false;
        }
        dmaBuffers = allocation;
    }
    m_DMABounceBuffers = dmaBuffers;

    if (m_IRQHandle < 0)
    {
        m_IRQHandle = register_irq_handler(get_usb_irq(portID), &USBHost_STM32::IRQCallback, this);
        if (m_IRQHandle < 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to register the USB host interrupt handler.");
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::InitializeController()
{
    bool result = true;

    m_Driver->EnableIRQ(false);

    // Restart the Phy clock.
    *m_PCGCCTL = 0;

    // Match the DWC2 host init path: use maximum FS timeout calibration.
    set_bit_group(m_Port->GUSBCFG, USB_OTG_GUSBCFG_TOCAL_Msk, 7u << USB_OTG_GUSBCFG_TOCAL_Pos);

    // Disable VBUS sensing.
    m_Port->GCCFG &= ~USB_OTG_GCCFG_VBDEN;

    // Disable battery charging detector.
    m_Port->GCCFG &= ~USB_OTG_GCCFG_BCDEN;

    if (m_Driver->GetConfigSpeed() == USB_Speed::LOW || m_Driver->GetConfigSpeed() == USB_Speed::FULL) {
        m_Host->HCFG |= USB_OTG_HCFG_FSLSS; // Force device enumeration to FS/LS mode only.
    } else {
        m_Host->HCFG &= ~USB_OTG_HCFG_FSLSS; // Default max speed support.
    }
    if (!m_Driver->FlushTxFifo(16)) {
        result = false;
    }
    if (!m_Driver->FlushRxFifo()) {
        result = false;
    }
    // Clear all pending host channel interrupts.
    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
        m_HostChannels[i].HCINT     = ~0u;
        m_HostChannels[i].HCINTMSK  = 0;
    }

    // Disable all interrupts.
    m_Port->GINTMSK = 0;

    // Clear any pending interrupts.
    m_Port->GINTSTS = ~0u;

    // Set FIFO ranges in 32-bit words: RX 0-511, non-periodic TX 512-767,
    // and periodic TX 768-991. The final 32 words include the DMA reservation.
    m_Port->GRXFSIZ = 2048 / 4;
    m_Port->DIEPTXF0_HNPTXFSIZ
        = ((2048 / 4) << USB_OTG_NPTXFSA_Pos)
        | ((1024 / 4) << USB_OTG_NPTXFD_Pos);
    m_Port->HPTXFSIZ
        = ((3072 / 4) << USB_OTG_HPTXFSIZ_PTXSA_Pos)
        | ((896 / 4) << USB_OTG_HPTXFSIZ_PTXFD_Pos);

    // Enable host mode only interrupts.
    m_Port->GINTMSK |= USB_OTG_GINTMSK_PRTIM | USB_OTG_GINTMSK_HCIM | USB_OTG_GINTMSK_SOFM | USB_OTG_GINTSTS_DISCINT | USB_OTG_GINTMSK_PXFRM_IISOOXFRM | USB_OTG_GINTMSK_WUIM;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::Shutdown()
{
    m_Driver->EnableIRQ(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_Speed USBHost_STM32::HostGetSpeed() const
{
    const uint32_t speed = (m_HPRT[0] & USB_OTG_HPRT_PSPD_Msk) >> USB_OTG_HPRT_PSPD_Pos;

    switch (speed)
    {
        case USB_OTG_CON_DEVICE_SPEED_HIGH: return USB_Speed::HIGH;
        case USB_OTG_CON_DEVICE_SPEED_FULL: return USB_Speed::FULL;
        case USB_OTG_CON_DEVICE_SPEED_LOW:  return USB_Speed::LOW;
        default: return USB_Speed::LOW;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::StartHost()
{
    if (m_RecoveryPending)
    {
        if (!m_Driver->ResetHostCore() || !InitializeController())
        {
            m_Driver->HoldCoreInReset();
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to recover the USB host controller.");
            return false;
        }
        for (USBHostChannelData& channel : m_ChannelStates) {
            channel = USBHostChannelData();
        }
        m_RecoveryPending = false;
    }
    DriveVBus(true);
    m_Driver->EnableIRQ(true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::StopHost()
{
    if (m_RecoveryPending) {
        return true; // RequestRecovery already stopped DMA and released every transfer.
    }
    bool result = true;
    bool controllerResetRequired = false;

    m_Driver->EnableIRQ(false);
    m_Port->GINTMSK &= ~USB_OTG_GINTMSK_HCIM;
    m_Host->HAINTMSK = 0;

    if (!m_Driver->FlushTxFifo(16))
    {
        result = false;
    }
    if (!m_Driver->FlushRxFifo())
    {
        result = false;
    }

    // Flush out any leftover queued requests.
    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
        USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[i];
        if (!IsUSBHostSTM32NonSplitPeriodicDMAChannel(channelRegs)) {
            set_bit_group(
                channelRegs.HCCHAR,
                USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS | USB_OTG_HCCHAR_EPDIR,
                USB_OTG_HCCHAR_CHDIS
            );
        }
    }

    // Halt all channels to put them into a known state.
    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
        USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[i];
        if (!IsUSBHostSTM32NonSplitPeriodicDMAChannel(channelRegs))
        {
            set_bit_group(
                channelRegs.HCCHAR,
                USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS | USB_OTG_HCCHAR_EPDIR,
                USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS
            );
        }

        for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100);
            kget_monotonic_time() < endTime && (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) != 0; ) {}

        if ((channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) != 0)
        {
            controllerResetRequired = true;
        }
    }

    if (controllerResetRequired && !m_Driver->ResetHostCore())
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to reset the USB host core while stopping active periodic DMA channels.");
        RequestRecovery();
        return false;
    }

    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
        USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[i];
        FinishDMATransfer(static_cast<USB_PipeIndex>(i), false, false, nullptr);

        channelRegs.HCINTMSK = 0;
        channelRegs.HCINT = ~0u;
        channelRegs.HCTSIZ = 0;
        channelRegs.HCDMA = 0;
        m_ChannelStates[i] = USBHostChannelData();
    }
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    m_ChannelErrorSnapshot = USBHostChannelErrorSnapshot();
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    m_ChannelHaltCondition.WakeupAll();

    if (controllerResetRequired) {
        result = InitializeController() && result;
    }
    if (!result)
    {
        RequestRecovery();
        return false;
    }

    // Clear any pending host interrupts.
    m_Host->HAINT   = ~0u;
    m_Port->GINTSTS = ~0u;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::SetPortReset(bool resetActive)
{
    uint32_t hprt0 = m_HPRT[0];

    if (resetActive && (hprt0 & USB_OTG_HPRT_PCSTS) == 0) {
        return false;
    }

    hprt0 &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);

    if (resetActive) {
        m_HPRT[0] = USB_OTG_HPRT_PRST | hprt0;
    } else {
        m_HPRT[0] = ~USB_OTG_HPRT_PRST & hprt0;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::SubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, const USB_TransferSegment* segments, size_t segmentCount, size_t length)
{
    if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT) {
        return false;
    }
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    USBHostChannelErrorSnapshot errorSnapshot;
    {
        USBIRQDisabler irqDisabler(*m_Driver);
        if (m_ChannelErrorSnapshot.Pending)
        {
            errorSnapshot = m_ChannelErrorSnapshot;
            m_ChannelErrorSnapshot.Pending = false;
        }
    }
    if (errorSnapshot.Pending)
    {
        const uint32_t currentHCCHAR = m_HostChannels[errorSnapshot.PipeIndex].HCCHAR;
        const uint32_t currentHCINT = m_HostChannels[errorSnapshot.PipeIndex].HCINT;
        kernel_log<PLogSeverity::ERROR>(
            LogCategoryUSBHost,
            "STM32 host channel error: pipe={}, irq=0x{:08x}, channel-state={}, transferred={}/{}, chunk={}/{}, "
            "error-HCCHAR=0x{:08x}, error-HCINT=0x{:08x}, error-HCTSIZ=0x{:08x}, error-HCDMA=0x{:08x}, "
            "current-HCCHAR=0x{:08x}, current-HCINT=0x{:08x}, cancel-pending={}, dma-active={}.",
            errorSnapshot.PipeIndex,
            errorSnapshot.Interrupts,
            std::to_underlying(errorSnapshot.ChannelState),
            errorSnapshot.BytesTransferred,
            errorSnapshot.RequestedTransferLength,
            errorSnapshot.TransferDataLength,
            errorSnapshot.XferSize,
            errorSnapshot.HCCHAR,
            errorSnapshot.HCINT,
            errorSnapshot.HCTSIZ,
            errorSnapshot.HCDMA,
            currentHCCHAR,
            currentHCINT,
            errorSnapshot.CancelHaltPending,
            errorSnapshot.DMATransferActive
        );
    }
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    if (direction == USB_RequestDirection::DEVICE_TO_HOST)
    {
        size_t remainingLength = length;
        for (size_t i = 0; i < segmentCount && remainingLength != 0; ++i)
        {
            if (segments[i].ReceiveCapacity != 0 && segments[i].ReceiveCapacity < segments[i].Length) {
                return false;
            }
            remainingLength -= std::min(remainingLength, segments[i].Length);
        }
    }

    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
    if (channel.CancelHaltPending || channel.DMATransferActive) {
        return false;
    }
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    ++channel.Diagnostics.SubmitRequestCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    channel.Direction    = direction;
    channel.EndpointType = endpointType;

    if (initialPID == USBH_InitialTransactionPID::Setup)
    {
        channel.InitialDataPID = USB_OTG_DATA_PID_SETUP;
        channel.ToggleOut = true;
    }
    else
    {
        channel.InitialDataPID = USB_OTG_DATA_PID_DATA1;
    }

    switch (endpointType)
    {
        case USB_TransferType::CONTROL:
            if (initialPID == USBH_InitialTransactionPID::Data && direction == USB_RequestDirection::HOST_TO_DEVICE)
            {
                if (length == 0) {
                    channel.ToggleOut = true; // For zero length status OUT stage PID is 1.
                }
                channel.InitialDataPID = (channel.ToggleOut) ? USB_OTG_DATA_PID_DATA1 : USB_OTG_DATA_PID_DATA0;
            }
            else if (initialPID == USBH_InitialTransactionPID::Data)
            {
                channel.ToggleIn = true;
                channel.InitialDataPID = USB_OTG_DATA_PID_DATA1;
            }
            break;
        case USB_TransferType::BULK:
        case USB_TransferType::INTERRUPT:
            if (direction == USB_RequestDirection::HOST_TO_DEVICE) {
                channel.InitialDataPID = (channel.ToggleOut) ? USB_OTG_DATA_PID_DATA1 : USB_OTG_DATA_PID_DATA0;
            } else {
                channel.InitialDataPID = (channel.ToggleIn) ? USB_OTG_DATA_PID_DATA1 : USB_OTG_DATA_PID_DATA0;
            }
            break;
        case USB_TransferType::ISOCHRONOUS:
            channel.InitialDataPID = USB_OTG_DATA_PID_DATA0;
            break;
        default:
            break;
    }

    channel.TransferSegments         = (segmentCount == 1) ? nullptr : segments;
    channel.TransferSegmentIndex     = 0;
    channel.TransferBuffer           = static_cast<uint8_t*>(segments[0].Buffer);
    channel.TransferDataLength      = 0;
    channel.RequestedTransferLength = length;
    channel.ReceiveCapacity         = (segmentCount == 1) ? segments[0].ReceiveCapacity : 0;
    channel.URBState                = USB_URBState::Idle;
    channel.PendingHaltURBState     = USB_URBState::Idle;
    channel.BytesTransferred        = 0;
    channel.TransferPacketCount     = 0;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.LastInterrupts          = 0;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.ErrorCount              = 0;
    channel.TransferActive          = true;
    channel.DMATransferActive       = false;
    channel.DMATransferBuffer       = nullptr;
    channel.DMAUsesBounceBuffer     = false;
    channel.ShortPacketReceived     = false;
    channel.StartOnNextSOF          = false;
    channel.RetryOnNextSOF          = false;
    channel.ChannelState            = USB_HostChannelState::IDLE;

    StartTransfer(pipeIndex);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::SetDataToggle(USB_PipeIndex pipeIndex, bool toggle)
{
    if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT) {
        return false;
    }
    if (m_ChannelStates[pipeIndex].Direction == USB_RequestDirection::DEVICE_TO_HOST) {
        m_ChannelStates[pipeIndex].ToggleIn = toggle;
    } else {
        m_ChannelStates[pipeIndex].ToggleOut = toggle;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::GetDataToggle(USB_PipeIndex pipeIndex) const
{
    if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT) {
        return false;
    }
    if (m_ChannelStates[pipeIndex].Direction == USB_RequestDirection::DEVICE_TO_HOST) {
        return m_ChannelStates[pipeIndex].ToggleIn;
    } else {
        return m_ChannelStates[pipeIndex].ToggleOut;
    }
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
size_t USBHost_STM32::GetPipeDebugEntryCount(USB_PipeIndex pipeIndex) const
{
    if (!IsUSBHostSTM32PipeIndexValid(pipeIndex) || m_HostChannels == nullptr) {
        return 0;
    }
    return USBHOST_STM32_DEBUG_ENTRY_COUNT;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::GetPipeDebugEntryLabel(USB_PipeIndex pipeIndex, size_t entryIndex, PString* outLabel) const
{
    if (outLabel == nullptr) {
        return false;
    }
    if (!IsUSBHostSTM32PipeIndexValid(pipeIndex) || m_HostChannels == nullptr || entryIndex >= USBHOST_STM32_DEBUG_ENTRY_COUNT) {
        return false;
    }

    *outLabel = USBHOST_STM32_DEBUG_LABELS[entryIndex];
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::GetPipeDebugEntryValue(USB_PipeIndex pipeIndex, size_t entryIndex, PString* outValue) const
{
    if (outValue == nullptr) {
        return false;
    }
    if (!IsUSBHostSTM32PipeIndexValid(pipeIndex) || m_HostChannels == nullptr || entryIndex >= USBHOST_STM32_DEBUG_ENTRY_COUNT) {
        return false;
    }

    const size_t channelIndex = static_cast<size_t>(pipeIndex);
    const USBHostChannelData& channel = m_ChannelStates[channelIndex];
    const USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[channelIndex];

    switch (static_cast<USBHostSTM32DebugEntry>(entryIndex))
    {
        case USBHostSTM32DebugEntry::Direction:
            *outValue = GetUSBHostSTM32RequestDirectionName(channel.Direction);
            break;
        case USBHostSTM32DebugEntry::EndpointType:
            *outValue = GetUSBHostSTM32TransferTypeName(channel.EndpointType);
            break;
        case USBHostSTM32DebugEntry::Speed:
            *outValue = GetUSBHostSTM32SpeedName(channel.Speed);
            break;
        case USBHostSTM32DebugEntry::MaxPacketSize:
            *outValue = PString::format_string("{}", channel.MaxPacketSize);
            break;
        case USBHostSTM32DebugEntry::URBState:
            *outValue = GetUSBHostSTM32URBStateName(channel.URBState);
            break;
        case USBHostSTM32DebugEntry::ChannelState:
            *outValue = GetUSBHostSTM32ChannelStateName(channel.ChannelState);
            break;
        case USBHostSTM32DebugEntry::XferSize:
            *outValue = PString::format_string("{}", channel.XferSize);
            break;
        case USBHostSTM32DebugEntry::RequestedTransferLength:
            *outValue = PString::format_string("{}", channel.RequestedTransferLength);
            break;
        case USBHostSTM32DebugEntry::BytesTransferred:
            *outValue = PString::format_string("{}", channel.BytesTransferred);
            break;
        case USBHostSTM32DebugEntry::ErrorCount:
            *outValue = PString::format_string("{}", channel.ErrorCount);
            break;
        case USBHostSTM32DebugEntry::ToggleIn:
            *outValue = GetUSBHostSTM32BoolName(channel.ToggleIn);
            break;
        case USBHostSTM32DebugEntry::ToggleOut:
            *outValue = GetUSBHostSTM32BoolName(channel.ToggleOut);
            break;
        case USBHostSTM32DebugEntry::HCCHAR:
            *outValue = PString::format_string("0x{:08x}", static_cast<uint32_t>(channelRegs.HCCHAR));
            break;
        case USBHostSTM32DebugEntry::HCSPLT:
            *outValue = PString::format_string("0x{:08x}", static_cast<uint32_t>(channelRegs.HCSPLT));
            break;
        case USBHostSTM32DebugEntry::HCINT:
            *outValue = PString::format_string("0x{:08x}", static_cast<uint32_t>(channelRegs.HCINT));
            break;
        case USBHostSTM32DebugEntry::HCINTMSK:
            *outValue = PString::format_string("0x{:08x}", static_cast<uint32_t>(channelRegs.HCINTMSK));
            break;
        case USBHostSTM32DebugEntry::HCTSIZ:
            *outValue = PString::format_string("0x{:08x}", static_cast<uint32_t>(channelRegs.HCTSIZ));
            break;
        case USBHostSTM32DebugEntry::HCDMA:
            *outValue = PString::format_string("0x{:08x}", static_cast<uint32_t>(channelRegs.HCDMA));
            break;
        case USBHostSTM32DebugEntry::SubmitRequestCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.SubmitRequestCount);
            break;
        case USBHostSTM32DebugEntry::StartTransferCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.StartTransferCount);
            break;
        case USBHostSTM32DebugEntry::TransferCompleteIRQCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.TransferCompleteIRQCount);
            break;
        case USBHostSTM32DebugEntry::NakNyetIRQCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.NakNyetIRQCount);
            break;
        case USBHostSTM32DebugEntry::ChannelHaltIRQCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.ChannelHaltIRQCount);
            break;
        case USBHostSTM32DebugEntry::FrameOverrunIRQCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.FrameOverrunIRQCount);
            break;
        case USBHostSTM32DebugEntry::AckIRQCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.AckIRQCount);
            break;
        case USBHostSTM32DebugEntry::StallIRQCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.StallIRQCount);
            break;
        case USBHostSTM32DebugEntry::TransactionErrorIRQCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.TransactionErrorIRQCount);
            break;
        case USBHostSTM32DebugEntry::NotifyDoneCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.NotifyDoneCount);
            break;
        case USBHostSTM32DebugEntry::NotifyNotReadyCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.NotifyNotReadyCount);
            break;
        case USBHostSTM32DebugEntry::NotifyStallCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.NotifyStallCount);
            break;
        case USBHostSTM32DebugEntry::NotifyErrorCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.NotifyErrorCount);
            break;
        case USBHostSTM32DebugEntry::Count:
            return false;
    }
    return true;
}
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::RequestRecovery()
{
    USBIRQDisabler irqDisabler(*m_Driver);
    m_Driver->EnableIRQ(false);
    m_Port->GINTMSK = 0;
    m_Driver->HoldCoreInReset();
    m_RecoveryPending = true;

    for (size_t channelIndex = 0; channelIndex < CHANNEL_COUNT; ++channelIndex)
    {
        FinishDMATransfer(static_cast<USB_PipeIndex>(channelIndex), false, false, nullptr);
        m_ChannelStates[channelIndex] = USBHostChannelData();
        // Existing submission/setup guards keep the stopped controller quiescent until StartHost.
        m_ChannelStates[channelIndex].CancelHaltPending = true;
    }
    m_ChannelHaltCondition.WakeupAll();
    kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "USB host controller stopped for recovery.");
    m_Driver->IRQDeviceDisconnected();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::SetChannelURBState(USB_PipeIndex pipeIndex, USB_URBState state)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];

    channel.URBState = state;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    if (state == USB_URBState::Error)
    {
        const USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];
        m_ChannelErrorSnapshot.PipeIndex = pipeIndex;
        m_ChannelErrorSnapshot.Interrupts = channel.LastInterrupts;
        m_ChannelErrorSnapshot.HCCHAR = channelRegs.HCCHAR;
        m_ChannelErrorSnapshot.HCINT = channelRegs.HCINT;
        m_ChannelErrorSnapshot.HCTSIZ = channelRegs.HCTSIZ;
        m_ChannelErrorSnapshot.HCDMA = channelRegs.HCDMA;
        m_ChannelErrorSnapshot.BytesTransferred = channel.BytesTransferred;
        m_ChannelErrorSnapshot.RequestedTransferLength = channel.RequestedTransferLength;
        m_ChannelErrorSnapshot.TransferDataLength = channel.TransferDataLength;
        m_ChannelErrorSnapshot.XferSize = channel.XferSize;
        m_ChannelErrorSnapshot.ChannelState = channel.ChannelState;
        m_ChannelErrorSnapshot.CancelHaltPending = channel.CancelHaltPending;
        m_ChannelErrorSnapshot.DMATransferActive = channel.DMATransferActive;
        m_ChannelErrorSnapshot.Pending = true;
    }
    IncrementUSBHostSTM32NotifyCounter(channel.Diagnostics, state);
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    m_Driver->IRQPipeURBStateChanged(pipeIndex, state, channel.BytesTransferred);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::DriveVBus(bool state)
{
    const uint32_t hprt0 = m_HPRT[0] & ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);

    if (state) {
        if ((hprt0 & USB_OTG_HPRT_PPWR) == 0) m_HPRT[0] = hprt0 | USB_OTG_HPRT_PPWR;
    } else {
        if ((hprt0 & USB_OTG_HPRT_PPWR)) m_HPRT[0] = hprt0 & ~USB_OTG_HPRT_PPWR;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::SelectPhyClock(uint32_t clock)
{
    set_bit_group(m_Host->HCFG, USB_OTG_HCFG_FSLSPCS_Msk, clock << USB_OTG_HCFG_FSLSPCS_Pos);

    if (clock == USB_OTG_HCFG_48_MHZ) {
        m_Host->HFIR = 48000;
    } else if (clock == USB_OTG_HCFG_6_MHZ) {
        m_Host->HFIR = 6000;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::ActivateChannel(USB_PipeIndex pipeIndex)
{
    const USBHostChannelData&   channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

    uint32_t channelCharacteristics = channelRegs.HCCHAR;
    const bool nextFrameIsOdd = (m_Host->HFNUM & 0x01) == 0;
    if (nextFrameIsOdd) {
        channelCharacteristics |= USB_OTG_HCCHAR_ODDFRM;
    } else {
        channelCharacteristics &= ~USB_OTG_HCCHAR_ODDFRM;
    }

    channelCharacteristics &= ~USB_OTG_HCCHAR_CHDIS;

    if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
        channelCharacteristics |= USB_OTG_HCCHAR_EPDIR;
    } else {
        channelCharacteristics &= ~USB_OTG_HCCHAR_EPDIR;
    }

    channelCharacteristics |= USB_OTG_HCCHAR_CHENA;
    channelRegs.HCCHAR = channelCharacteristics;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t USBHost_STM32::GetCurrentFrame()
{
    return m_Host->HFNUM & USB_OTG_HFNUM_FRNUM;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize)
{
    if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT || maxPacketSize == 0 || maxPacketSize > DMA_BOUNCE_BUFFER_SIZE) {
        return false;
    }
    constexpr size_t hardwareMaxPacketCount = USB_OTG_HCTSIZ_PKTCNT_Msk >> USB_OTG_HCTSIZ_PKTCNT_Pos;
    constexpr size_t hardwareMaxTransferSize = USB_OTG_HCTSIZ_XFRSIZ_Msk >> USB_OTG_HCTSIZ_XFRSIZ_Pos;

    USBHostChannelData&         channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];
    if (channel.CancelHaltPending) {
        return false;
    }

#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.Diagnostics = USBHostChannelDiagnostics();
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.MaxPacketSize        = static_cast<uint16_t>(maxPacketSize);
    channel.MaxDMAPacketCount    = static_cast<uint16_t>(
        std::min(hardwareMaxPacketCount, hardwareMaxTransferSize / maxPacketSize));
    channel.BounceDMAPacketCount = static_cast<uint16_t>(
        std::min<size_t>(channel.MaxDMAPacketCount, DMA_BOUNCE_BUFFER_SIZE / maxPacketSize));
    channel.EndpointType         = endpointType;
    channel.Direction            = (endpointAddr & USB_ADDRESS_DIR_IN) ? USB_RequestDirection::DEVICE_TO_HOST : USB_RequestDirection::HOST_TO_DEVICE;
    channel.Speed                = speed;

    // Clear all channel interrupts.
    channelRegs.HCINT = ~0u;

    // Enable channel interrupts required for this transfer.
    switch (endpointType)
    {
        case USB_TransferType::CONTROL:
        case USB_TransferType::BULK:
            // Internal DMA handles NAK/NYET/ACK for these channels at every bus speed.
            channelRegs.HCINTMSK
                = USB_OTG_HCINTMSK_XFRCM
                | USB_OTG_HCINTMSK_STALLM
                | USB_OTG_HCINTMSK_TXERRM
                | USB_OTG_HCINTMSK_DTERRM
                | USB_OTG_HCINTMSK_AHBERR;

            if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
                channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
            }
            break;

        case USB_TransferType::INTERRUPT:
            channelRegs.HCINTMSK
                = USB_OTG_HCINTMSK_XFRCM
                | USB_OTG_HCINTMSK_STALLM
                | USB_OTG_HCINTMSK_TXERRM
                | USB_OTG_HCINTMSK_DTERRM
                | USB_OTG_HCINTMSK_NAKM
                | USB_OTG_HCINTMSK_AHBERR
                | USB_OTG_HCINTMSK_FRMORM;

            if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
                channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
            }
            break;

        case USB_TransferType::ISOCHRONOUS:
            channelRegs.HCINTMSK
                = USB_OTG_HCINTMSK_XFRCM
                | USB_OTG_HCINTMSK_ACKM
                | USB_OTG_HCINTMSK_AHBERR
                | USB_OTG_HCINTMSK_FRMORM;

            if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
                channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_TXERRM | USB_OTG_HCINTMSK_BBERRM;
            }
            break;

        default:
            return false;
            break;
    }

    // Channel halt is part of normal transfer completion and cancellation.
    channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_CHHM;

    // Enable top-level host channel interrupt.
    m_Host->HAINTMSK |= 1 << pipeIndex;

    // Enable channel interrupts.
    m_Port->GINTMSK |= USB_OTG_GINTMSK_HCIM;

    channelRegs.HCSPLT = 0;

    const uint32_t hostCoreSpeed = (m_HPRT[0] & USB_OTG_HPRT_PSPD) >> USB_OTG_HPRT_PSPD_Pos;

    const bool lowSpeedDevice = speed == USB_Speed::LOW && hostCoreSpeed != USB_OTG_CON_DEVICE_SPEED_LOW;

    uint32_t hostChannelCharacteristics
        = (static_cast<uint32_t>(deviceAddr) << USB_OTG_HCCHAR_DAD_Pos)
        | (static_cast<uint32_t>(USB_ADDRESS_EPNUM(endpointAddr)) << USB_OTG_HCCHAR_EPNUM_Pos)
        | (static_cast<uint32_t>(endpointType) << USB_OTG_HCCHAR_EPTYP_Pos)
        | (static_cast<uint32_t>(maxPacketSize) << USB_OTG_HCCHAR_MPSIZ_Pos)
        | ((channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) ? USB_OTG_HCCHAR_EPDIR : 0)
        | (lowSpeedDevice ? USB_OTG_HCCHAR_LSDEV : 0)
        | USB_OTG_HCCHAR_MC_0;

    if (endpointType == USB_TransferType::INTERRUPT || endpointType == USB_TransferType::ISOCHRONOUS) {
        hostChannelCharacteristics |= USB_OTG_HCCHAR_ODDFRM;
    }
    channelRegs.HCCHAR = hostChannelCharacteristics;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t USBHost_STM32::PrepareDMATransfer(USB_PipeIndex pipeIndex)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];

    const size_t remainingLength = channel.RequestedTransferLength - channel.BytesTransferred;

    uint8_t* transferBuffer = m_DMABounceBuffers[pipeIndex];
    size_t segmentRemainingLength = 0;
    size_t remainingCapacity = 0;
    if (remainingLength != 0)
    {
        if (channel.TransferSegments != nullptr)
        {
            const USB_TransferSegment& segment = channel.TransferSegments[channel.TransferSegmentIndex];
            transferBuffer = channel.TransferBuffer;
            const size_t segmentOffset
                = reinterpret_cast<uintptr_t>(transferBuffer) - reinterpret_cast<uintptr_t>(segment.Buffer);
            segmentRemainingLength = segment.Length - segmentOffset;
            remainingCapacity = (segment.ReceiveCapacity > segmentOffset) ? segment.ReceiveCapacity - segmentOffset : 0;
        }
        else
        {
            transferBuffer = channel.TransferBuffer + channel.BytesTransferred;
            segmentRemainingLength = remainingLength;
            remainingCapacity = (channel.ReceiveCapacity > channel.BytesTransferred)
                ? channel.ReceiveCapacity - channel.BytesTransferred : 0;
        }
    }
    const size_t availableLength = std::min(remainingLength, segmentRemainingLength);

    size_t requestedPacketCount = 1;
    if (availableLength != 0) {
        requestedPacketCount = 1 + (availableLength - 1) / channel.MaxPacketSize;
    }
    const bool receive = channel.Direction == USB_RequestDirection::DEVICE_TO_HOST;
    size_t directDataLength = std::min(availableLength, static_cast<size_t>(channel.MaxDMAPacketCount) * channel.MaxPacketSize);
    if (receive) {
        directDataLength = USB_STM32::GetDirectDMAReceiveLength(
            transferBuffer, directDataLength, channel.MaxPacketSize, remainingCapacity);
    }
    const bool useDirectDMA = directDataLength != 0
        && (receive || USB_STM32::IsDirectDMATransmitBuffer(transferBuffer, directDataLength));

    const uint32_t packetCount = static_cast<uint32_t>(useDirectDMA
        ? 1 + (directDataLength - 1) / channel.MaxPacketSize
        : std::min<size_t>(requestedPacketCount, channel.BounceDMAPacketCount));

    channel.TransferPacketCount = packetCount;
    channel.TransferDataLength = std::min(availableLength, static_cast<size_t>(packetCount) * channel.MaxPacketSize);
    if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
        channel.XferSize = packetCount * channel.MaxPacketSize;
    } else {
        channel.XferSize = channel.TransferDataLength;
    }

    channel.DMAUsesBounceBuffer = !useDirectDMA;
    channel.DMATransferBuffer = useDirectDMA ? transferBuffer : m_DMABounceBuffers[pipeIndex];

    if (channel.DMAUsesBounceBuffer && channel.Direction == USB_RequestDirection::HOST_TO_DEVICE && channel.TransferDataLength > 0) {
        std::memcpy(m_DMABounceBuffers[pipeIndex], transferBuffer, channel.TransferDataLength);
    }

    const size_t cacheLength = align_up(channel.XferSize, __SCB_DCACHE_LINE_SIZE);
    if (cacheLength > 0)
    {
        if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
            SCB_CleanInvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(channel.DMATransferBuffer), static_cast<int32_t>(cacheLength));
        } else {
            USB_STM32::CleanDMATransmitBuffer(channel.DMATransferBuffer, channel.XferSize);
        }
    }
    return packetCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::StartTransfer(USB_PipeIndex pipeIndex)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    ++channel.Diagnostics.StartTransferCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];
    const uint32_t packetCount = PrepareDMATransfer(pipeIndex);

    USBIRQDisabler irqDisabler(*m_Driver);

    channel.RetryOnNextSOF = false;
    channelRegs.HCTSIZ
        = (channel.XferSize & USB_OTG_HCTSIZ_XFRSIZ)
        | ((packetCount << USB_OTG_HCTSIZ_PKTCNT_Pos) & USB_OTG_HCTSIZ_PKTCNT_Msk)
        | ((channel.InitialDataPID << USB_OTG_HCTSIZ_DPID_Pos) & USB_OTG_HCTSIZ_DPID);

    channelRegs.HCDMA = reinterpret_cast<uint32_t>(channel.DMATransferBuffer);
    channel.DMATransferActive = true;

    ActivateChannel(pipeIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::FinishDMATransfer(USB_PipeIndex pipeIndex, bool commitTransfer, bool transferComplete, bool* madeProgress)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
    if (madeProgress != nullptr) {
        *madeProgress = false;
    }
    if (!channel.DMATransferActive) {
        return true;
    }

    uint8_t* dmaTransferBuffer = channel.DMATransferBuffer;
    const bool dmaUsesBounceBuffer = channel.DMAUsesBounceBuffer;

    if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST && channel.XferSize > 0)
    {
        const size_t cacheLength = align_up(channel.XferSize, __SCB_DCACHE_LINE_SIZE);
        SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(dmaTransferBuffer), static_cast<int32_t>(cacheLength));
    }
    channel.DMATransferActive = false;
    channel.DMATransferBuffer = nullptr;
    channel.DMAUsesBounceBuffer = false;

    size_t transferredLength;
    uint32_t transferredPacketCount;
    if (transferComplete && channel.Direction == USB_RequestDirection::HOST_TO_DEVICE)
    {
        // On OUT transfer completion the core does not guarantee useful residual
        // HCTSIZ values. XFRC confirms that the entire programmed transfer was sent.
        transferredLength = channel.TransferDataLength;
        transferredPacketCount = channel.TransferPacketCount;
    }
    else
    {
        const uint32_t transferState = m_HostChannels[pipeIndex].HCTSIZ;
        const size_t remainingLength = transferState & USB_OTG_HCTSIZ_XFRSIZ_Msk;
        const uint32_t remainingPacketCount = (transferState & USB_OTG_HCTSIZ_PKTCNT_Msk) >> USB_OTG_HCTSIZ_PKTCNT_Pos;
        if (remainingLength > channel.XferSize || remainingPacketCount > channel.TransferPacketCount) {
            return false;
        }
        transferredLength = channel.XferSize - remainingLength;
        transferredPacketCount = channel.TransferPacketCount - remainingPacketCount;
    }
    if (transferredLength > channel.TransferDataLength ||
        (transferredLength > 0 && transferredPacketCount == 0) ||
        (transferComplete && transferredPacketCount == 0) ||
        (transferComplete && channel.Direction == USB_RequestDirection::HOST_TO_DEVICE && transferredLength != channel.TransferDataLength)) {
        return false;
    }

    if (!commitTransfer)
    {
        UpdateDataToggle(channel, transferredPacketCount);
        return true;
    }

    if (transferredLength != 0)
    {
        uint8_t* transferBuffer;
        if (channel.TransferSegments != nullptr) {
            transferBuffer = channel.TransferBuffer;
        } else {
            transferBuffer = channel.TransferBuffer + channel.BytesTransferred;
        }
        if (dmaUsesBounceBuffer && channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
            std::memcpy(transferBuffer, dmaTransferBuffer, transferredLength);
        }

        if (channel.TransferSegments != nullptr)
        {
            const USB_TransferSegment& segment = channel.TransferSegments[channel.TransferSegmentIndex];
            channel.TransferBuffer += transferredLength;
            if (reinterpret_cast<uintptr_t>(channel.TransferBuffer)
                == reinterpret_cast<uintptr_t>(segment.Buffer) + segment.Length)
            {
                ++channel.TransferSegmentIndex;
                if (channel.BytesTransferred + transferredLength < channel.RequestedTransferLength) {
                    channel.TransferBuffer = static_cast<uint8_t*>(channel.TransferSegments[channel.TransferSegmentIndex].Buffer);
                }
            }
        }
    }
    channel.BytesTransferred += transferredLength;
    channel.ShortPacketReceived
        = transferComplete
        && channel.Direction == USB_RequestDirection::DEVICE_TO_HOST
        && transferredLength < channel.XferSize;
    UpdateDataToggle(channel, transferredPacketCount);

    if (madeProgress != nullptr) {
        *madeProgress = transferredPacketCount != 0;
    }
    return channel.BytesTransferred <= channel.RequestedTransferLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::CompleteChannelCancellation(USB_PipeIndex pipeIndex)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];

    FinishDMATransfer(pipeIndex, false, false, nullptr);
    channel.CancelHaltPending = false;
    m_ChannelHaltCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::RecoverDMATransferError(USB_PipeIndex pipeIndex)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
    channel.RetryOnNextSOF = false;

    bool madeProgress = false;
    if (!FinishDMATransfer(pipeIndex, true, false, &madeProgress))
    {
        channel.ErrorCount = 0;
        channel.TransferActive = false;
        channel.StartOnNextSOF = false;
        SetChannelURBState(pipeIndex, USB_URBState::Error);
        return;
    }

    if (madeProgress)
    {
        channel.ErrorCount = 0;
        if (channel.BytesTransferred >= channel.RequestedTransferLength)
        {
            channel.TransferActive = false;
            channel.StartOnNextSOF = false;
            SetChannelURBState(pipeIndex, USB_URBState::Done);
            return;
        }
    }
    else if (++channel.ErrorCount > 2)
    {
        channel.ErrorCount = 0;
        channel.TransferActive = false;
        channel.StartOnNextSOF = false;
        SetChannelURBState(pipeIndex, USB_URBState::Error);
        return;
    }

    // Rebuild the hardware transaction at the next frame boundary. Immediate
    // retries can exhaust the error limit before the device has another frame
    // in which to recover from the failed transaction.
    channel.RetryOnNextSOF = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::UpdateDataToggle(USBHostChannelData& channel, uint32_t packetCount)
{
    const bool updateDataToggle
        = (channel.EndpointType == USB_TransferType::CONTROL && channel.InitialDataPID != USB_OTG_DATA_PID_SETUP)
        || channel.EndpointType == USB_TransferType::BULK
        || channel.EndpointType == USB_TransferType::INTERRUPT;
    if (updateDataToggle && (packetCount & 1) != 0)
    {
        if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
            channel.ToggleIn ^= 1;
        } else {
            channel.ToggleOut ^= 1;
        }
        const bool toggle = (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) ? channel.ToggleIn : channel.ToggleOut;
        channel.InitialDataPID = (toggle) ? USB_OTG_DATA_PID_DATA1 : USB_OTG_DATA_PID_DATA0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::HaltChannel(USB_PipeIndex pipeIndex)
{
    if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT) {
        return false;
    }

    if (m_RecoveryPending) {
        return true;
    }

    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    const uint32_t channelCharacteristics = channelRegs.HCCHAR;
    const uint32_t channelInterrupts = channelRegs.HCINT;
    const uint32_t channelInterruptMask = channelRegs.HCINTMSK;
    const uint32_t channelTransferSize = channelRegs.HCTSIZ;
    const uint32_t channelDMAAddress = channelRegs.HCDMA;
    const uint32_t globalInterrupts = m_Port->GINTSTS;
    const uint32_t hostInterrupts = m_Host->HAINT;
    if (channel.TransferActive || channel.DMATransferActive
        || (channelCharacteristics & USB_OTG_HCCHAR_CHENA) != 0)
    {
        kernel_log<PLogSeverity::WARNING>(
            LogCategoryUSBHost,
            "STM32 host cancel state: pipe={}, direction={}, type={}, mps={}, direct-packets={}, bounce-packets={}, "
            "pid={}, channel-state={}, errors={}, transferred={}/{}, chunk={}/{}, packets={}, active={}, "
            "dma-active={}, bounce={}, short={}, start-sof={}, retry-sof={}, last-irq=0x{:08x}.",
            pipeIndex,
            std::to_underlying(channel.Direction),
            std::to_underlying(channel.EndpointType),
            channel.MaxPacketSize,
            channel.MaxDMAPacketCount,
            channel.BounceDMAPacketCount,
            channel.InitialDataPID,
            std::to_underlying(channel.ChannelState),
            channel.ErrorCount,
            channel.BytesTransferred,
            channel.RequestedTransferLength,
            channel.TransferDataLength,
            channel.XferSize,
            channel.TransferPacketCount,
            channel.TransferActive,
            channel.DMATransferActive,
            channel.DMAUsesBounceBuffer,
            channel.ShortPacketReceived,
            channel.StartOnNextSOF,
            channel.RetryOnNextSOF,
            channel.LastInterrupts
        );
        kernel_log<PLogSeverity::WARNING>(
            LogCategoryUSBHost,
            "STM32 host cancel registers: HCCHAR=0x{:08x}, HCINT=0x{:08x}, HCINTMSK=0x{:08x}, "
            "HCTSIZ=0x{:08x}, HCDMA=0x{:08x}, GINTSTS=0x{:08x}, HAINT=0x{:08x}.",
            channelCharacteristics, channelInterrupts, channelInterruptMask, channelTransferSize,
            channelDMAAddress, globalInterrupts, hostInterrupts
        );
    }
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    const TimeValNanos haltDeadline = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100);
    bool result = true;

    // IRQWaitDeadline() requires kernel-wide IRQ exclusion while the waiter is linked to the scheduler.
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        const bool hardwareHaltPending
            = (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) != 0
            || (channelRegs.HCINT & USB_OTG_HCINT_CHH) != 0;
        const bool transferDeferred = channel.StartOnNextSOF || channel.RetryOnNextSOF;
        if (channel.TransferActive || channel.CancelHaltPending || hardwareHaltPending)
        {
            channel.TransferActive = false;
            channel.StartOnNextSOF = false;
            channel.RetryOnNextSOF = false;
            channel.PendingHaltURBState = USB_URBState::Idle;
            if (transferDeferred && !hardwareHaltPending)
            {
                FinishDMATransfer(pipeIndex, false, false, nullptr);
                channel.CancelHaltPending = false;
            }
            else
            {
                channel.CancelHaltPending = true;
                HaltChannelInternal(pipeIndex);

                if ((channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) == 0) {
                    CompleteChannelCancellation(pipeIndex);
                }

                while (result && channel.CancelHaltPending)
                {
                    const PErrorCode waitResult = m_ChannelHaltCondition.IRQWaitDeadline(haltDeadline);
                    if (waitResult != PErrorCode::Success)
                    {
                        if ((channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) == 0) {
                            CompleteChannelCancellation(pipeIndex);
                        } else {
                            result = false;
                        }
                    }
                }
            }
        }
        if (result)
        {
            const bool transferReleased = FinishDMATransfer(pipeIndex, false, false, nullptr);
            channel.TransferActive = false;
            channel.DMATransferActive = false;
            channel.CancelHaltPending = false;
            channel.PendingHaltURBState = USB_URBState::Idle;
            channel.StartOnNextSOF = false;
            channel.RetryOnNextSOF = false;
            channel.URBState = USB_URBState::Idle;
            channel.ChannelState = USB_HostChannelState::IDLE;
            channelRegs.HCINT = ~0u;
            result = transferReleased;
        }
    } CRITICAL_END;
    if (!result) {
        RequestRecovery();
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::HaltChannelInternal(USB_PipeIndex pipeIndex)
{
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];
    const bool channelEnabled = (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) != 0;
    const bool splitEnabled = (channelRegs.HCSPLT & USB_OTG_HCSPLT_SPLITEN) != 0;

    // Buffer-DMA periodic channels halt automatically at the frame boundary.
    // Programming CHDIS for them can leave the channel in an undefined state.
    if (splitEnabled || (channelEnabled && !IsUSBHostSTM32NonSplitPeriodicDMAChannel(channelRegs)))
    {
        channelRegs.HCCHAR |= USB_OTG_HCCHAR_CHDIS;
        channelRegs.HCCHAR |= USB_OTG_HCCHAR_CHENA;
        // Enable channel halt interrupt.
        channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_CHHM;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USBHost_STM32::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<USBHost_STM32*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USBHost_STM32::HandleIRQ()
{
    if (m_Driver->GetUSBMode() == USB_Mode::Host)
    {
        const uint32_t interrupts = m_Port->GINTSTS & m_Port->GINTMSK;

        if (interrupts == 0) {
            return IRQResult::HANDLED;
        }
        if (interrupts & USB_OTG_GINTSTS_PXFR_INCOMPISOOUT) {
            m_Port->GINTSTS = USB_OTG_GINTSTS_PXFR_INCOMPISOOUT;
        }
        if (interrupts & USB_OTG_GINTSTS_IISOIXFR) {
            m_Port->GINTSTS = USB_OTG_GINTSTS_IISOIXFR;
        }
        if (interrupts & USB_OTG_GINTSTS_MMIS) {
            m_Port->GINTSTS = USB_OTG_GINTSTS_MMIS;
        }

        // Handle disconnect interrupt.
        if (interrupts & USB_OTG_GINTSTS_DISCINT)
        {
            m_Port->GINTSTS = USB_OTG_GINTSTS_DISCINT;

            if ((m_HPRT[0] & USB_OTG_HPRT_PCSTS) == 0)
            {
                m_Driver->FlushTxFifo(16);
                m_Driver->FlushRxFifo();

                SelectPhyClock(USB_OTG_HCFG_48_MHZ);

                m_Driver->IRQDeviceDisconnected();
            }
        }
        if (interrupts & USB_OTG_GINTSTS_HPRTINT) {
            HandlePortIRQ();
        }
        if (interrupts & USB_OTG_GINTSTS_SOF)
        {
            for (uint32_t i = 0; i < CHANNEL_COUNT; ++i)
            {
                USBHostChannelData& channel = m_ChannelStates[i];
                const USB_PipeIndex pipeIndex = static_cast<USB_PipeIndex>(i);
                if (channel.CancelHaltPending && (m_HostChannels[i].HCCHAR & USB_OTG_HCCHAR_CHENA) == 0) {
                    CompleteChannelCancellation(pipeIndex);
                }

                if (channel.RetryOnNextSOF)
                {
                    channel.RetryOnNextSOF = false;
                    if (channel.TransferActive) {
                        StartTransfer(pipeIndex);
                    }
                }
            }
            m_Driver->IRQStartOfFrame();
            m_Port->GINTSTS = USB_OTG_GINTSTS_SOF;
        }

        // Handle channel interrupts.
        if (interrupts & USB_OTG_GINTSTS_HCINT)
        {
            const uint32_t channelInterrupts = m_Host->HAINT;
            for (int32_t i = 0; i < CHANNEL_COUNT; ++i)
            {
                if (channelInterrupts & (1 << i))
                {
                    if (m_HostChannels[i].HCCHAR & USB_OTG_HCCHAR_EPDIR) {
                        HandleChannelInIRQ(i);
                    } else {
                        HandleChannelOutIRQ(i);
                    }
                }
            }
            m_Port->GINTSTS = USB_OTG_GINTSTS_HCINT;
        }
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::HandleChannelInIRQ(USB_PipeIndex pipeIndex)
{
    USBHostChannelData&         channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

    const uint32_t interrupts = channelRegs.HCINT & channelRegs.HCINTMSK;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.LastInterrupts = interrupts;
    CountUSBHostSTM32ChannelInterrupts(channel, interrupts);
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    if (channel.CancelHaltPending)
    {
        channelRegs.HCINT = interrupts;
        if ((interrupts & USB_OTG_HCINT_CHH) != 0) {
            CompleteChannelCancellation(pipeIndex);
        }
        return;
    }
    if (channel.PendingHaltURBState != USB_URBState::Idle)
    {
        channelRegs.HCINT = interrupts;
        if ((interrupts & USB_OTG_HCINT_CHH) != 0)
        {
            if ((channelRegs.HCCHAR & USB_OTG_HCCHAR_CHDIS) != 0) {
                return;
            }
            const USB_URBState urbState = channel.PendingHaltURBState;
            channel.PendingHaltURBState = USB_URBState::Idle;
            channel.TransferActive = false;
            channel.StartOnNextSOF = false;
            channel.RetryOnNextSOF = false;
            const bool transferValid = FinishDMATransfer(pipeIndex, true, false, nullptr);
            SetChannelURBState(pipeIndex, (transferValid) ? urbState : USB_URBState::Error);
        }
        return;
    }
    if (!channel.TransferActive)
    {
        channelRegs.HCINT = interrupts;
        return;
    }

    if (interrupts & USB_OTG_HCINT_AHBERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_AHBERR;
        channel.ChannelState = USB_HostChannelState::XACTERR;
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_BBERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_BBERR;
        channel.ChannelState = USB_HostChannelState::BBLERR;
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_ACK)
    {
        channelRegs.HCINT = USB_OTG_HCINT_ACK;
    }
    else if (interrupts & USB_OTG_HCINT_STALL)
    {
        channelRegs.HCINT = USB_OTG_HCINT_STALL;
        channel.ChannelState = USB_HostChannelState::STALL;
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_DTERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_DTERR;
        channel.ChannelState = USB_HostChannelState::DATATGLERR;
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_TXERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_TXERR;
        channel.ChannelState = USB_HostChannelState::XACTERR;
        HaltChannelInternal(pipeIndex);
    }

    if (interrupts & USB_OTG_HCINT_FRMOR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_FRMOR;
        if (channel.EndpointType == USB_TransferType::INTERRUPT)
        {
            channel.ChannelState = USB_HostChannelState::NAK;
            HaltChannelInternal(pipeIndex);
        }
        else
        {
            channel.RetryOnNextSOF = false;
            channel.PendingHaltURBState = USB_URBState::Error;
            HaltChannelInternal(pipeIndex);
        }
    }
    else if (interrupts & USB_OTG_HCINT_XFRC)
    {
        if (!FinishDMATransfer(pipeIndex, true, true, nullptr))
        {
            channelRegs.HCINT = USB_OTG_HCINT_XFRC;
            channel.PendingHaltURBState = USB_URBState::Error;
            HaltChannelInternal(pipeIndex);
            return;
        }
        channel.ChannelState = USB_HostChannelState::XFRC;
        channel.ErrorCount   = 0;

        channelRegs.HCINT = USB_OTG_HCINT_XFRC;

        if (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)
        {
            HaltChannelInternal(pipeIndex);
            channelRegs.HCINT = USB_OTG_HCINT_NAK;
        }
        else if (channel.EndpointType == USB_TransferType::INTERRUPT || channel.EndpointType == USB_TransferType::ISOCHRONOUS)
        {
            channelRegs.HCCHAR |= USB_OTG_HCCHAR_ODDFRM;
            if (channel.BytesTransferred < channel.RequestedTransferLength && !channel.ShortPacketReceived)
            {
                channel.StartOnNextSOF = true;
                HaltChannelInternal(pipeIndex);
            }
            else
            {
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Done);
            }
        }
    }
    else if (interrupts & USB_OTG_HCINT_NAK)
    {
        if (channel.EndpointType == USB_TransferType::INTERRUPT)
        {
            channel.ErrorCount = 0;
            channel.ChannelState = USB_HostChannelState::NAK;
            HaltChannelInternal(pipeIndex);
        }
        channelRegs.HCINT = USB_OTG_HCINT_NAK;
    }
    else if (interrupts & USB_OTG_HCINT_CHH)
    {
        // CHH is write-one-to-clear. Acknowledge it before restarting the channel
        // so the acknowledgement cannot consume the next transfer's halt event.
        channelRegs.HCINT = USB_OTG_HCINT_CHH;
        if ((channelRegs.HCCHAR & USB_OTG_HCCHAR_CHDIS) != 0) {
            return;
        }

        if (channel.ChannelState == USB_HostChannelState::XFRC)
        {
            if (channel.StartOnNextSOF)
            {
                channel.StartOnNextSOF = false;
                channel.RetryOnNextSOF = true;
                return;
            }
            if (channel.BytesTransferred < channel.RequestedTransferLength && !channel.ShortPacketReceived)
            {
                StartTransfer(pipeIndex);
                return;
            }
            channel.TransferActive = false;
            channel.RetryOnNextSOF = false;
            SetChannelURBState(pipeIndex, USB_URBState::Done);
        }
        else if (channel.ChannelState == USB_HostChannelState::STALL)
        {
            const bool transferValid = FinishDMATransfer(pipeIndex, true, false, nullptr);
            channel.TransferActive = false;
            channel.RetryOnNextSOF = false;
            SetChannelURBState(pipeIndex, (transferValid) ? USB_URBState::Stall : USB_URBState::Error);
        }
        else if (channel.ChannelState == USB_HostChannelState::XACTERR || channel.ChannelState == USB_HostChannelState::DATATGLERR)
        {
            if (channel.EndpointType == USB_TransferType::CONTROL && channel.Speed == USB_Speed::LOW && channel.RequestedTransferLength > 0)
            {
                FinishDMATransfer(pipeIndex, true, false, nullptr);
                channel.ErrorCount = 0;
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else
            {
                RecoverDMATransferError(pipeIndex);
            }
        }
        else if (channel.ChannelState == USB_HostChannelState::IDLE
            && (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) == 0)
        {
            // A channel can report a delayed halt after a new DMA request has
            // been installed. Retry rather than leaving the request orphaned.
            RecoverDMATransferError(pipeIndex);
        }
        else if (channel.ChannelState == USB_HostChannelState::NAK)
        {
            bool madeProgress = false;
            const bool transferValid = FinishDMATransfer(pipeIndex, true, false, &madeProgress);
            if (!transferValid)
            {
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else if (madeProgress && channel.BytesTransferred >= channel.RequestedTransferLength)
            {
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Done);
            }
            else if (madeProgress || channel.EndpointType == USB_TransferType::INTERRUPT)
            {
                StartTransfer(pipeIndex);
            }
            else if (channel.BytesTransferred != 0)
            {
                // A segmented request is one URB. Keep a NAK on a later
                // segment inside the HCD instead of reporting a partial URB.
                channel.RetryOnNextSOF = true;
            }
            else
            {
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::NotReady);
            }
        }
        else if (channel.ChannelState == USB_HostChannelState::BBLERR)
        {
            FinishDMATransfer(pipeIndex, true, false, nullptr);
            channel.ErrorCount++;
            channel.TransferActive = false;
            channel.RetryOnNextSOF = false;
            SetChannelURBState(pipeIndex, USB_URBState::Error);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::HandleChannelOutIRQ(USB_PipeIndex pipeIndex)
{
    USBHostChannelData&         channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

    const uint32_t interrupts = channelRegs.HCINT & channelRegs.HCINTMSK;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.LastInterrupts = interrupts;
    CountUSBHostSTM32ChannelInterrupts(channel, interrupts);
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    if (channel.CancelHaltPending)
    {
        channelRegs.HCINT = interrupts;
        if ((interrupts & USB_OTG_HCINT_CHH) != 0) {
            CompleteChannelCancellation(pipeIndex);
        }
        return;
    }
    if (channel.PendingHaltURBState != USB_URBState::Idle)
    {
        channelRegs.HCINT = interrupts;
        if ((interrupts & USB_OTG_HCINT_CHH) != 0)
        {
            const USB_URBState urbState = channel.PendingHaltURBState;
            channel.PendingHaltURBState = USB_URBState::Idle;
            channel.TransferActive = false;
            channel.StartOnNextSOF = false;
            channel.RetryOnNextSOF = false;
            const bool transferValid = FinishDMATransfer(pipeIndex, true, false, nullptr);
            SetChannelURBState(pipeIndex, (transferValid) ? urbState : USB_URBState::Error);
        }
        return;
    }
    if (!channel.TransferActive)
    {
        channelRegs.HCINT = interrupts;
        return;
    }

    if (interrupts & USB_OTG_HCINT_AHBERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_AHBERR;
        channel.ChannelState = USB_HostChannelState::XACTERR;
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_ACK)
    {
        channelRegs.HCINT = USB_OTG_HCINT_ACK;
    }
    else if (interrupts & USB_OTG_HCINT_FRMOR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_FRMOR;
        if (channel.EndpointType == USB_TransferType::INTERRUPT)
        {
            channel.ChannelState = USB_HostChannelState::NAK;
            HaltChannelInternal(pipeIndex);
        }
        else
        {
            channel.RetryOnNextSOF = false;
            channel.PendingHaltURBState = USB_URBState::Error;
            HaltChannelInternal(pipeIndex);
        }
    }
    else if (interrupts & USB_OTG_HCINT_XFRC)
    {
        if (!FinishDMATransfer(pipeIndex, true, true, nullptr))
        {
            channelRegs.HCINT = USB_OTG_HCINT_XFRC;
            channel.PendingHaltURBState = USB_URBState::Error;
            HaltChannelInternal(pipeIndex);
            return;
        }
        channel.ErrorCount = 0;

        channelRegs.HCINT = USB_OTG_HCINT_XFRC;
        channel.ChannelState = USB_HostChannelState::XFRC;
        if ((channel.EndpointType == USB_TransferType::INTERRUPT || channel.EndpointType == USB_TransferType::ISOCHRONOUS) &&
            channel.BytesTransferred < channel.RequestedTransferLength) {
            channel.StartOnNextSOF = true;
        }
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_STALL)
    {
        channelRegs.HCINT = USB_OTG_HCINT_STALL;
        channel.ChannelState = USB_HostChannelState::STALL;
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_NAK)
    {
        channel.ErrorCount = 0;
        channel.ChannelState = USB_HostChannelState::NAK;

        HaltChannelInternal(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_NAK;
    }
    else if (interrupts & USB_OTG_HCINT_TXERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_TXERR;
        channel.ChannelState = USB_HostChannelState::XACTERR;
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_DTERR)
    {
        channel.ChannelState = USB_HostChannelState::DATATGLERR;
        HaltChannelInternal(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_DTERR;
    }
    else if (interrupts & USB_OTG_HCINT_CHH)
    {
        // Clear this halt before any restart can generate the next one.
        channelRegs.HCINT = USB_OTG_HCINT_CHH;
        if (channel.ChannelState == USB_HostChannelState::XFRC)
        {
            if (channel.StartOnNextSOF)
            {
                channel.StartOnNextSOF = false;
                channel.RetryOnNextSOF = true;
                return;
            }
            else if (channel.BytesTransferred < channel.RequestedTransferLength)
            {
                StartTransfer(pipeIndex);
                return;
            }

            channel.TransferActive = false;
            channel.RetryOnNextSOF = false;
            SetChannelURBState(pipeIndex, USB_URBState::Done);
        }
        else if (channel.ChannelState == USB_HostChannelState::NAK)
        {
            bool madeProgress = false;
            const bool transferValid = FinishDMATransfer(pipeIndex, true, false, &madeProgress);
            if (!transferValid)
            {
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else if (madeProgress && channel.BytesTransferred < channel.RequestedTransferLength)
            {
                StartTransfer(pipeIndex);
                return;
            }
            else if (madeProgress)
            {
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Done);
            }
            else if (channel.BytesTransferred != 0)
            {
                // A segmented request is one URB. Keep a NAK on a later
                // segment inside the HCD instead of reporting a partial URB.
                channel.RetryOnNextSOF = true;
            }
            else
            {
                channel.TransferActive = false;
                SetChannelURBState(pipeIndex, USB_URBState::NotReady);
            }
        }
        else if (channel.ChannelState == USB_HostChannelState::STALL)
        {
            const bool transferValid = FinishDMATransfer(pipeIndex, true, false, nullptr);
            channel.TransferActive = false;
            channel.RetryOnNextSOF = false;
            SetChannelURBState(pipeIndex, (transferValid) ? USB_URBState::Stall : USB_URBState::Error);
        }
        else if (channel.ChannelState == USB_HostChannelState::XACTERR || channel.ChannelState == USB_HostChannelState::DATATGLERR)
        {
            RecoverDMATransferError(pipeIndex);
        }
        else if (channel.ChannelState == USB_HostChannelState::IDLE
            && (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) == 0)
        {
            // A channel can report a delayed halt after a new DMA request has
            // been installed. Retry rather than leaving the request orphaned.
            RecoverDMATransferError(pipeIndex);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::HandlePortIRQ()
{
    const uint32_t  hprt0Src = m_HPRT[0];
    uint32_t        hprt0Dst = hprt0Src;
    bool            disconnectReported = false;

    hprt0Dst &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);

    // Check whether port connect detected.
    if (hprt0Src & USB_OTG_HPRT_PCDET)
    {
        if (hprt0Src & USB_OTG_HPRT_PCSTS)
        {
            m_Driver->IRQDeviceConnected();
        }
        else
        {
            m_Driver->FlushTxFifo(16);
            m_Driver->FlushRxFifo();
            SelectPhyClock(USB_OTG_HCFG_48_MHZ);
            m_Driver->IRQDeviceDisconnected();
            disconnectReported = true;
        }
        hprt0Dst |= USB_OTG_HPRT_PCDET;
    }

    // Check whether port enable changed.
    if (hprt0Src & USB_OTG_HPRT_PENCHNG)
    {
        hprt0Dst |= USB_OTG_HPRT_PENCHNG;

        if (hprt0Src & USB_OTG_HPRT_PENA)
        {
            if (m_Driver->GetPhyInterface() == USB_OTG_Phy::Embedded)
            {
                const uint32_t speed = (hprt0Src & USB_OTG_HPRT_PSPD_Msk) >> USB_OTG_HPRT_PSPD_Pos;
                SelectPhyClock((speed == USB_OTG_CON_DEVICE_SPEED_LOW) ? USB_OTG_HCFG_6_MHZ : USB_OTG_HCFG_48_MHZ);
            }
            else
            {
                if (m_Driver->GetConfigSpeed() == USB_Speed::FULL) {
                    m_Host->HFIR = 60000;
                }
            }
            m_Driver->IRQPortEnableChange(true);
        }
        else
        {
            if (!disconnectReported) {
                m_Driver->IRQPortEnableChange(false);
            }
        }
    }

    // Acknowledge over current event.
    if (hprt0Src & USB_OTG_HPRT_POCCHNG) {
        hprt0Dst |= USB_OTG_HPRT_POCCHNG;
    }
    // Clear port interrupts.
    m_HPRT[0] = hprt0Dst;
}

} // namespace kernel
