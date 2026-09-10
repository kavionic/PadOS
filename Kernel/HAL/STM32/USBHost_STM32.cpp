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

#include <algorithm>
#include <cstring>
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

alignas(__SCB_DCACHE_LINE_SIZE)
uint8_t g_USBHostSTM32DMABounceBuffers[2][USBHost_STM32::CHANNEL_COUNT][USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE]
    __attribute__((section(".sram.data")));
static_assert((USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE % __SCB_DCACHE_LINE_SIZE) == 0);

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
    DoPing,
    ToggleIn,
    ToggleOut,
    HCCHAR,
    HCSPLT,
    HCINT,
    HCINTMSK,
    HCTSIZ,
    HCDMA,
    SubmitRequestCount,
    SubmitRequestFailureCount,
    StartTransferCount,
    StartTransferFailureCount,
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
    "stm32.doPing",
    "stm32.toggleIn",
    "stm32.toggleOut",
    "stm32.HCCHAR",
    "stm32.HCSPLT",
    "stm32.HCINT",
    "stm32.HCINTMSK",
    "stm32.HCTSIZ",
    "stm32.HCDMA",
    "stm32.submitRequestCount",
    "stm32.submitRequestFailureCount",
    "stm32.startTransferCount",
    "stm32.startTransferFailureCount",
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
        case USB_HostChannelState::NYET:       return "nyet";
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
    m_Driver = driver;

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
    for (size_t i = 0; i < CHANNEL_COUNT; ++i) {
        m_ChannelStates[i].DMABounceBuffer = g_USBHostSTM32DMABounceBuffers[dmaBufferSet][i];
    }

    m_Port          = get_usb_from_id(portID);
    m_Host          = reinterpret_cast<USB_OTG_HostTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_HOST_BASE);
    m_HPRT          = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(m_Port) + USB_OTG_HOST_PORT_BASE);
    m_HostChannels  = reinterpret_cast<USB_OTG_HostChannelTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_HOST_CHANNEL_BASE);
    m_PCGCCTL       = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(m_Port) + USB_OTG_PCGCCTL_BASE);

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

    // Enable the common interrupts.
    if (!m_Driver->UseDMA()) {
        m_Port->GINTMSK |= USB_OTG_GINTMSK_RXFLVLM;
    }
    // Enable host mode only interrupts.
    m_Port->GINTMSK |= USB_OTG_GINTMSK_PRTIM | USB_OTG_GINTMSK_HCIM | USB_OTG_GINTMSK_SOFM | USB_OTG_GINTSTS_DISCINT | USB_OTG_GINTMSK_PXFRM_IISOOXFRM | USB_OTG_GINTMSK_WUIM;

    IRQn_Type irq = get_usb_irq(portID);
    register_irq_handler(irq, &USBHost_STM32::IRQCallback, this);

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
    DriveVBus(true);
    m_Driver->EnableIRQ(true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::StopHost()
{
    bool ret = true;

    m_Driver->EnableIRQ(false);
    m_Port->GINTMSK &= ~USB_OTG_GINTMSK_HCIM;
    m_Host->HAINTMSK = 0;

    if (!m_Driver->FlushTxFifo(16))
    {
        ret = false;
    }
    if (!m_Driver->FlushRxFifo())
    {
        ret = false;
    }

    // Flush out any leftover queued requests.
    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
        set_bit_group(m_HostChannels[i].HCCHAR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS | USB_OTG_HCCHAR_EPDIR, USB_OTG_HCCHAR_CHDIS);
    }

    // Halt all channels to put them into a known state.
    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
        set_bit_group(m_HostChannels[i].HCCHAR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS | USB_OTG_HCCHAR_EPDIR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS);
        for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100); kget_monotonic_time() < endTime && (m_HostChannels[i].HCCHAR & USB_OTG_HCCHAR_CHENA); ) {}

        if ((m_HostChannels[i].HCCHAR & USB_OTG_HCCHAR_CHENA) != 0) {
            ret = false;
        }
        FinishDMATransfer(static_cast<USB_PipeIndex>(i), false, false, nullptr);

        m_HostChannels[i].HCINTMSK = 0;
        m_HostChannels[i].HCINT = ~0u;
        m_HostChannels[i].HCTSIZ = 0;
        m_HostChannels[i].HCDMA = 0;
        uint8_t* dmaBounceBuffer = m_ChannelStates[i].DMABounceBuffer;
        m_ChannelStates[i] = USBHostChannelData();
        m_ChannelStates[i].DMABounceBuffer = dmaBounceBuffer;
    }
    m_ChannelHaltCondition.WakeupAll();

    // Clear any pending host interrupts.
    m_Host->HAINT   = ~0u;
    m_Port->GINTSTS = ~0u;

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::ResetPort()
{
    uint32_t hprt0 = m_HPRT[0];

    hprt0 &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);

    m_HPRT[0] = USB_OTG_HPRT_PRST | hprt0;
    snooze_ms(100); // Must wait at least 10mS (waiting 100mS for safety).
    m_HPRT[0] = ~USB_OTG_HPRT_PRST & hprt0;
    snooze_ms(10);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::SubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, void* buffer, size_t length, bool doPing)
{
    if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT) {
        return false;
    }
    USBHostChannelErrorSnapshot errorSnapshot;
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
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
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
    if (channel.CancelHaltPending || channel.DMATransferActive || (length > 0 && buffer == nullptr)) {
        return false;
    }
    const bool dmaEnabled = m_Driver->UseDMA();
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    ++channel.Diagnostics.SubmitRequestCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    channel.Direction    = direction;
    channel.EndpointType = endpointType;
    channel.DoPing
        = !dmaEnabled
        && direction == USB_RequestDirection::HOST_TO_DEVICE
        && channel.Speed == USB_Speed::HIGH
        && (endpointType == USB_TransferType::CONTROL || endpointType == USB_TransferType::BULK)
        && doPing;

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

    channel.TransferBuffer          = reinterpret_cast<uint8_t*>(buffer);
    channel.TransferDataLength      = 0;
    channel.RequestedTransferLength = length;
    channel.URBState                = USB_URBState::Idle;
    channel.PendingHaltURBState     = USB_URBState::Idle;
    channel.BytesTransferred        = 0;
    channel.TransferPacketCount     = 0;
    channel.LastInterrupts          = 0;
    channel.ErrorCount              = 0;
    channel.TransferActive          = true;
    channel.DMATransferActive       = false;
    channel.ShortPacketReceived     = false;
    channel.StartOnNextSOF          = false;
    channel.RetryOnNextSOF          = false;
    channel.ChannelState            = USB_HostChannelState::IDLE;

    const bool result = StartTransfer(pipeIndex, dmaEnabled);
    if (!result) {
        channel.TransferActive = false;
    }
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    if (!result) {
        ++channel.Diagnostics.SubmitRequestFailureCount;
    }
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    return result;
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
        case USBHostSTM32DebugEntry::DoPing:
            *outValue = GetUSBHostSTM32BoolName(channel.DoPing != 0);
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
        case USBHostSTM32DebugEntry::SubmitRequestFailureCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.SubmitRequestFailureCount);
            break;
        case USBHostSTM32DebugEntry::StartTransferCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.StartTransferCount);
            break;
        case USBHostSTM32DebugEntry::StartTransferFailureCount:
            *outValue = PString::format_string("{}", channel.Diagnostics.StartTransferFailureCount);
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

void USBHost_STM32::SetChannelURBState(USB_PipeIndex pipeIndex, USB_URBState state)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];

    channel.URBState = state;
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
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
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
    USBHostChannelData&         channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];
    if (channel.CancelHaltPending) {
        return false;
    }

#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.Diagnostics = USBHostChannelDiagnostics();
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    channel.DoPing          = false;
    channel.MaxPacketSize   = maxPacketSize;
    channel.EndpointType    = endpointType;
    channel.Direction       = (endpointAddr & USB_ADDRESS_DIR_IN) ? USB_RequestDirection::DEVICE_TO_HOST : USB_RequestDirection::HOST_TO_DEVICE;
    channel.Speed           = speed;

    // Clear all channel interrupts.
    channelRegs.HCINT = ~0u;

    // Enable channel interrupts required for this transfer.
    switch (endpointType)
    {
        case USB_TransferType::CONTROL:
        case USB_TransferType::BULK:
            channelRegs.HCINTMSK
                = USB_OTG_HCINTMSK_XFRCM
                | USB_OTG_HCINTMSK_STALLM
                | USB_OTG_HCINTMSK_TXERRM
                | USB_OTG_HCINTMSK_DTERRM
                | USB_OTG_HCINTMSK_AHBERR
                | USB_OTG_HCINTMSK_NAKM;

            if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
                channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_BBERRM;
            } else {
                channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM;
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

bool USBHost_STM32::PrepareDMATransfer(USB_PipeIndex pipeIndex, uint32_t* packetCount)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
    constexpr uint32_t hardwareMaxPacketCount = USB_OTG_HCTSIZ_PKTCNT_Msk >> USB_OTG_HCTSIZ_PKTCNT_Pos;
    constexpr size_t hardwareMaxTransferSize = USB_OTG_HCTSIZ_XFRSIZ_Msk >> USB_OTG_HCTSIZ_XFRSIZ_Pos;

    if (packetCount == nullptr || channel.DMABounceBuffer == nullptr || channel.DMATransferActive || channel.MaxPacketSize == 0 ||
        channel.BytesTransferred > channel.RequestedTransferLength)
    {
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
        ++channel.Diagnostics.StartTransferFailureCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
        return false;
    }

    const size_t remainingLength = channel.RequestedTransferLength - channel.BytesTransferred;
    const size_t bouncePacketCapacity = DMA_BOUNCE_BUFFER_SIZE / channel.MaxPacketSize;
    const size_t transferSizePacketCapacity = hardwareMaxTransferSize / channel.MaxPacketSize;
    const size_t packetCapacity = std::min<size_t>(hardwareMaxPacketCount, std::min(bouncePacketCapacity, transferSizePacketCapacity));
    if (packetCapacity == 0)
    {
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
        ++channel.Diagnostics.StartTransferFailureCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
        return false;
    }

    if (remainingLength > 0) {
        *packetCount = static_cast<uint32_t>(std::min(1 + (remainingLength - 1) / channel.MaxPacketSize, packetCapacity));
    } else {
        *packetCount = 1;
    }

    channel.TransferPacketCount = *packetCount;
    channel.TransferDataLength = std::min(remainingLength, static_cast<size_t>(*packetCount) * channel.MaxPacketSize);
    if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
        channel.XferSize = *packetCount * channel.MaxPacketSize;
    } else {
        channel.XferSize = channel.TransferDataLength;
    }

    if (channel.Direction == USB_RequestDirection::HOST_TO_DEVICE && channel.TransferDataLength > 0) {
        std::memcpy(channel.DMABounceBuffer, channel.TransferBuffer + channel.BytesTransferred, channel.TransferDataLength);
    }

    const size_t cacheLength = align_up(channel.XferSize, __SCB_DCACHE_LINE_SIZE);
    if (cacheLength > 0)
    {
        if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
            SCB_CleanInvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(channel.DMABounceBuffer), static_cast<int32_t>(cacheLength));
        } else {
            SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(channel.DMABounceBuffer), static_cast<int32_t>(cacheLength));
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::StartTransfer(USB_PipeIndex pipeIndex, bool dma)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    ++channel.Diagnostics.StartTransferCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

    // In DMA mode the host core handles NAK/NYET/ACK for control and bulk
    // channels at every bus speed.
    if (dma && (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)) {
        channelRegs.HCINTMSK &= ~(USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM | USB_OTG_HCINTMSK_NAKM);
    }

    if (channel.Speed == USB_Speed::HIGH && !dma && channel.DoPing)
    {
        if (!DoPing(pipeIndex))
        {
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
            ++channel.Diagnostics.StartTransferFailureCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
            return false;
        }
        return true;
    }

    uint32_t packetCount;
    if (dma)
    {
        if (!PrepareDMATransfer(pipeIndex, &packetCount)) {
            return false;
        }
    }
    else if (!dma && channel.Direction == USB_RequestDirection::HOST_TO_DEVICE)
    {
        if (channel.BytesTransferred > channel.RequestedTransferLength ||
            (channel.MaxPacketSize == 0 && channel.BytesTransferred < channel.RequestedTransferLength))
        {
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
            ++channel.Diagnostics.StartTransferFailureCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
            return false;
        }

        const size_t remainingLength = channel.RequestedTransferLength - channel.BytesTransferred;
        channel.XferSize = (remainingLength > channel.MaxPacketSize) ? channel.MaxPacketSize : remainingLength;
        packetCount = 1;
    }
    else
    {
        if (channel.RequestedTransferLength > 0)
        {
            constexpr uint32_t maxPacketCount = USB_OTG_HCTSIZ_PKTCNT_Msk >> USB_OTG_HCTSIZ_PKTCNT_Pos;
            packetCount = (channel.RequestedTransferLength + channel.MaxPacketSize - 1) / channel.MaxPacketSize;

            if (packetCount > maxPacketCount)
            {
                packetCount = maxPacketCount;
                channel.RequestedTransferLength = packetCount * channel.MaxPacketSize;
            }
        }
        else
        {
            packetCount = 1;
        }

        // For IN channel HCTSIZ.XferSize should be an integer multiple of max packet size.
        if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
            channel.XferSize = packetCount * channel.MaxPacketSize;
        } else {
            channel.XferSize = channel.RequestedTransferLength;
        }
    }

    CRITICAL_SCOPE(CRITICAL_IRQ);

    if (!dma && channel.Direction == USB_RequestDirection::HOST_TO_DEVICE && channel.XferSize > 0)
    {
        const size_t lengthWords = (channel.XferSize + 3) / 4;
        size_t fifoDepth;
        size_t fifoSpace;

        if (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)
        {
            fifoDepth = (m_Port->DIEPTXF0_HNPTXFSIZ & USB_OTG_NPTXFD_Msk) >> USB_OTG_NPTXFD_Pos;
            fifoSpace = (m_Port->HNPTXSTS & USB_OTG_GNPTXSTS_NPTXFSAV_Msk) >> USB_OTG_GNPTXSTS_NPTXFSAV_Pos;
        }
        else
        {
            fifoDepth = (m_Port->HPTXFSIZ & USB_OTG_HPTXFSIZ_PTXFD_Msk) >> USB_OTG_HPTXFSIZ_PTXFD_Pos;
            fifoSpace = (m_Host->HPTXSTS & USB_OTG_HPTXSTS_PTXFSAVL_Msk) >> USB_OTG_HPTXSTS_PTXFSAVL_Pos;
        }

        if (lengthWords > fifoDepth)
        {
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
            ++channel.Diagnostics.StartTransferFailureCount;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
            return false;
        }
        if (lengthWords > fifoSpace)
        {
            channel.RetryOnNextSOF = true;
            return true;
        }
    }

    channel.RetryOnNextSOF = false;
    channelRegs.HCTSIZ
        = (channel.XferSize & USB_OTG_HCTSIZ_XFRSIZ)
        | ((packetCount << USB_OTG_HCTSIZ_PKTCNT_Pos) & USB_OTG_HCTSIZ_PKTCNT_Msk)
        | ((channel.InitialDataPID << USB_OTG_HCTSIZ_DPID_Pos) & USB_OTG_HCTSIZ_DPID);

    if (dma)
    {
        channelRegs.HCDMA = reinterpret_cast<uint32_t>(channel.DMABounceBuffer);
        channel.DMATransferActive = true;
    }

    ActivateChannel(pipeIndex);

    if (dma) {
        return true;
    }

    if (channel.Direction == USB_RequestDirection::HOST_TO_DEVICE && channel.XferSize > 0) {
        m_Driver->WriteToFIFO(pipeIndex, channel.TransferBuffer + channel.BytesTransferred, channel.XferSize);
    }

    return true;
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

    if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST && channel.XferSize > 0)
    {
        const size_t cacheLength = align_up(channel.XferSize, __SCB_DCACHE_LINE_SIZE);
        SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(channel.DMABounceBuffer), static_cast<int32_t>(cacheLength));
    }
    channel.DMATransferActive = false;

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

    if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST && transferredLength > 0) {
        std::memcpy(channel.TransferBuffer + channel.BytesTransferred, channel.DMABounceBuffer, transferredLength);
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

    USBHostChannelData& channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];
    const TimeValNanos haltDeadline = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100);
    bool result = true;

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
                result = HaltChannelInternal(pipeIndex);

                while (result && channel.CancelHaltPending)
                {
                    const PErrorCode waitResult = m_ChannelHaltCondition.IRQWaitDeadline(haltDeadline);
                    if (waitResult != PErrorCode::Success) {
                        result = false;
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
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::HaltChannelInternal(USB_PipeIndex pipeIndex)
{
    USB_OTG_HostChannelTypeDef& channelRegs     = m_HostChannels[pipeIndex];
    USB_TransferType            endpointType    = USB_TransferType((channelRegs.HCCHAR & USB_OTG_HCCHAR_EPTYP) >> USB_OTG_HCCHAR_EPTYP_Pos);
    const bool                  channelEnabled  = (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) != 0;
    const bool                  dmaEnabled      = (m_Port->GAHBCFG & USB_OTG_GAHBCFG_DMAEN) != 0;
    const bool                  splitEnabled    = (channelRegs.HCSPLT & USB_OTG_HCSPLT_SPLITEN) != 0;
    const bool                  periodicChannel = endpointType == USB_TransferType::INTERRUPT || endpointType == USB_TransferType::ISOCHRONOUS;

    if (dmaEnabled && !splitEnabled && (!channelEnabled || periodicChannel))
    {
        // Buffer-DMA periodic channels halt automatically at the frame boundary.
        // Programming CHDIS for them can leave the channel in an undefined state.
        return true;
    }
    channelRegs.HCCHAR |= USB_OTG_HCCHAR_CHDIS;

    if (!dmaEnabled)
    {
        const bool hasQueueSpace = (endpointType == USB_TransferType::CONTROL || endpointType == USB_TransferType::BULK)
            ? ((m_Port->HNPTXSTS & USB_OTG_GNPTXSTS_NPTQXSAV_Msk) != 0)
            : ((m_Host->HPTXSTS & USB_OTG_HPTXSTS_PTXQSAV_Msk) != 0);
    
        if (!hasQueueSpace)
        {
            // Flush queue to make space for the disable request.
            channelRegs.HCCHAR &= ~USB_OTG_HCCHAR_CHENA;
            channelRegs.HCCHAR |= USB_OTG_HCCHAR_CHENA;
            for (int i = 0; i < 1000 && (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA); ++i) {}
        }
        else
        {
            channelRegs.HCCHAR |= USB_OTG_HCCHAR_CHENA;
        }
    }
    else
    {
        channelRegs.HCCHAR |= USB_OTG_HCCHAR_CHENA;
    }
    // Enable channel halt interrupt.
    channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_CHHM;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::DoPing(USB_PipeIndex pipeIndex)
{
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];
    const uint32_t packetCount = 1;

    channelRegs.HCTSIZ = (packetCount << USB_OTG_HCTSIZ_PKTCNT_Pos) | USB_OTG_HCTSIZ_DOPING;
    // Enable channel.
    set_bit_group(channelRegs.HCCHAR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS, USB_OTG_HCCHAR_CHENA);
    return true;
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
        if (interrupts & USB_OTG_GINTSTS_NPTXFE) {
            m_Port->GINTMSK &= ~USB_OTG_GINTMSK_NPTXFEM;
        }
        if (interrupts & USB_OTG_GINTSTS_PTXFE) {
            m_Port->GINTMSK &= ~USB_OTG_GINTMSK_PTXFEM;
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
                // Workaround the interrupts flood issue: re-enable NAK interrupt
                if (!m_Driver->UseDMA()) {
                    m_HostChannels[i].HCINTMSK |= USB_OTG_HCINT_NAK;
                }

                USBHostChannelData& channel = m_ChannelStates[i];
                if (channel.RetryOnNextSOF)
                {
                    channel.RetryOnNextSOF = false;
                    if (channel.TransferActive)
                    {
                        const USB_PipeIndex pipeIndex = static_cast<USB_PipeIndex>(i);
                        if (!StartTransfer(pipeIndex, m_Driver->UseDMA()))
                        {
                            channel.TransferActive = false;
                            SetChannelURBState(pipeIndex, USB_URBState::Error);
                        }
                    }
                }
            }
            m_Driver->IRQStartOfFrame();
            m_Port->GINTSTS = USB_OTG_GINTSTS_SOF;
        }

        // Handle RX FIFO level interrupt.
        if (interrupts & USB_OTG_GINTSTS_RXFLVL)
        {
            m_Port->GINTMSK &= ~USB_OTG_GINTSTS_RXFLVL;
            HandleRxFIFONotEmptyIRQ();
            const bool receiveFIFOStillNotEmpty = (m_Port->GINTSTS & USB_OTG_GINTSTS_RXFLVL) != 0;
            m_Port->GINTMSK |= USB_OTG_GINTSTS_RXFLVL;
            if (receiveFIFOStillNotEmpty) {
                return IRQResult::HANDLED;
            }
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
    channel.LastInterrupts = interrupts;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    CountUSBHostSTM32ChannelInterrupts(channel, interrupts);
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    if (channel.CancelHaltPending)
    {
        channelRegs.HCINT = interrupts;
        if ((interrupts & USB_OTG_HCINT_CHH) != 0)
        {
            if ((channelRegs.HCCHAR & USB_OTG_HCCHAR_CHDIS) != 0) {
                return;
            }
            FinishDMATransfer(pipeIndex, false, false, nullptr);
            channel.CancelHaltPending = false;
            m_ChannelHaltCondition.WakeupAll();
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
        const bool dmaEnabled = m_Driver->UseDMA();
        if (dmaEnabled && !FinishDMATransfer(pipeIndex, true, true, nullptr))
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
            if (dmaEnabled && channel.BytesTransferred < channel.RequestedTransferLength && !channel.ShortPacketReceived)
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

        if (!dmaEnabled) {
            channel.ToggleIn ^= 1;
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
        else if (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)
        {
            channel.ErrorCount = 0;

            if (!m_Driver->UseDMA())
            {
                // Workaround NAK interrupt flood issue.
                channelRegs.HCINTMSK &= ~USB_OTG_HCINT_NAK;
            }
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
            if (m_Driver->UseDMA() && channel.StartOnNextSOF)
            {
                channel.StartOnNextSOF = false;
                channel.RetryOnNextSOF = true;
                return;
            }
            if (m_Driver->UseDMA() && channel.BytesTransferred < channel.RequestedTransferLength && !channel.ShortPacketReceived)
            {
                if (!StartTransfer(pipeIndex, true))
                {
                    channel.TransferActive = false;
                    channel.RetryOnNextSOF = false;
                    SetChannelURBState(pipeIndex, USB_URBState::Error);
                }
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
            else if (++channel.ErrorCount > 2)
            {
                FinishDMATransfer(pipeIndex, true, false, nullptr);
                channel.ErrorCount = 0;
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else
            {
                // Resume the current hardware transaction. In DMA mode HCTSIZ and
                // HCDMA identify the untransferred portion and must not be rebuilt.
                ActivateChannel(pipeIndex);
            }
        }
        else if (channel.ChannelState == USB_HostChannelState::NAK)
        {
            const bool dmaEnabled = m_Driver->UseDMA();
            if (dmaEnabled)
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
                    if (!StartTransfer(pipeIndex, true))
                    {
                        channel.TransferActive = false;
                        channel.RetryOnNextSOF = false;
                        SetChannelURBState(pipeIndex, USB_URBState::Error);
                    }
                }
                else
                {
                    channel.TransferActive = false;
                    channel.RetryOnNextSOF = false;
                    SetChannelURBState(pipeIndex, USB_URBState::NotReady);
                }
                return;
            }

            if (channel.EndpointType != USB_TransferType::INTERRUPT)
            {
                SetChannelURBState(pipeIndex, USB_URBState::NotReady);

                // Re-activate the channel.
                ActivateChannel(pipeIndex);
            }
            else
            {
                if (!StartTransfer(pipeIndex, false))
                {
                    channel.TransferActive = false;
                    channel.RetryOnNextSOF = false;
                    SetChannelURBState(pipeIndex, USB_URBState::Error);
                }
                return;
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
    channel.LastInterrupts = interrupts;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    CountUSBHostSTM32ChannelInterrupts(channel, interrupts);
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

    if (channel.CancelHaltPending)
    {
        channelRegs.HCINT = interrupts;
        if ((interrupts & USB_OTG_HCINT_CHH) != 0)
        {
            FinishDMATransfer(pipeIndex, false, false, nullptr);
            channel.CancelHaltPending = false;
            m_ChannelHaltCondition.WakeupAll();
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

        if (channel.DoPing)
        {
            channel.DoPing = false;
            channel.ChannelState = USB_HostChannelState::NYET;
            HaltChannelInternal(pipeIndex);
        }
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
        if (m_Driver->UseDMA() && !FinishDMATransfer(pipeIndex, true, true, nullptr))
        {
            channelRegs.HCINT = USB_OTG_HCINT_XFRC;
            channel.PendingHaltURBState = USB_URBState::Error;
            HaltChannelInternal(pipeIndex);
            return;
        }
        channel.ErrorCount = 0;

        // Transaction completed with NYET state, update do ping state.
        if (interrupts & USB_OTG_HCINT_NYET)
        {
            channel.DoPing = true;
            channelRegs.HCINT = USB_OTG_HCINT_NYET;
        }
        channelRegs.HCINT = USB_OTG_HCINT_XFRC;
        channel.ChannelState = USB_HostChannelState::XFRC;
        if (m_Driver->UseDMA() &&
            (channel.EndpointType == USB_TransferType::INTERRUPT || channel.EndpointType == USB_TransferType::ISOCHRONOUS) &&
            channel.BytesTransferred < channel.RequestedTransferLength) {
            channel.StartOnNextSOF = true;
        }
        HaltChannelInternal(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_NYET)
    {
        channel.ChannelState = USB_HostChannelState::NYET;
        channel.DoPing = true;
        channel.ErrorCount = 0;
        HaltChannelInternal(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_NYET;
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

        if (!channel.DoPing && channel.Speed == USB_Speed::HIGH &&
            (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)) {
            channel.DoPing = true;
        }

        HaltChannelInternal(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_NAK;
    }
    else if (interrupts & USB_OTG_HCINT_TXERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_TXERR;
        if (!m_Driver->UseDMA())
        {
            channel.ChannelState = USB_HostChannelState::XACTERR;
            HaltChannelInternal(pipeIndex);
        }
        else if (++channel.ErrorCount > 2)
        {
            FinishDMATransfer(pipeIndex, true, false, nullptr);
            channel.ErrorCount = 0;
            channel.TransferActive = false;
            channel.RetryOnNextSOF = false;
            SetChannelURBState(pipeIndex, USB_URBState::Error);
        }
        else
        {
            // The DMA engine has already advanced HCDMA and HCTSIZ past packets
            // accepted by the device. Resume those registers so only the failed
            // transaction is retried.
            ActivateChannel(pipeIndex);
        }
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
        const bool dmaEnabled = m_Driver->UseDMA();
        if (channel.ChannelState == USB_HostChannelState::XFRC)
        {
            if (!dmaEnabled)
            {
                channel.BytesTransferred += channel.XferSize;

                const bool updateDataToggle
                    = (channel.EndpointType == USB_TransferType::CONTROL && channel.InitialDataPID != USB_OTG_DATA_PID_SETUP)
                    || channel.EndpointType == USB_TransferType::BULK
                    || channel.EndpointType == USB_TransferType::INTERRUPT;
                if (updateDataToggle)
                {
                    channel.ToggleOut ^= 1;
                    channel.InitialDataPID = (channel.ToggleOut) ? USB_OTG_DATA_PID_DATA1 : USB_OTG_DATA_PID_DATA0;
                }

                if (channel.BytesTransferred < channel.RequestedTransferLength)
                {
                    channelRegs.HCINT = USB_OTG_HCINT_CHH;
                    if (channel.DoPing)
                    {
                        channel.RetryOnNextSOF = true;
                    }
                    else if (!StartTransfer(pipeIndex, false))
                    {
                        channel.TransferActive = false;
                        channel.RetryOnNextSOF = false;
                        SetChannelURBState(pipeIndex, USB_URBState::Error);
                    }
                    return;
                }
            }
            else if (channel.StartOnNextSOF)
            {
                channel.StartOnNextSOF = false;
                channel.RetryOnNextSOF = true;
                return;
            }
            else if (channel.BytesTransferred < channel.RequestedTransferLength)
            {
                if (!StartTransfer(pipeIndex, true))
                {
                    channel.TransferActive = false;
                    channel.RetryOnNextSOF = false;
                    SetChannelURBState(pipeIndex, USB_URBState::Error);
                }
                return;
            }

            channel.TransferActive = false;
            channel.RetryOnNextSOF = false;
            SetChannelURBState(pipeIndex, USB_URBState::Done);
        }
        else if (channel.ChannelState == USB_HostChannelState::NAK || channel.ChannelState == USB_HostChannelState::NYET)
        {
            if (!dmaEnabled)
            {
                channel.RetryOnNextSOF = true;
            }
            else
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
                    if (!StartTransfer(pipeIndex, true))
                    {
                        channel.TransferActive = false;
                        channel.RetryOnNextSOF = false;
                        SetChannelURBState(pipeIndex, USB_URBState::Error);
                    }
                    return;
                }
                else if (madeProgress)
                {
                    channel.TransferActive = false;
                    channel.RetryOnNextSOF = false;
                    SetChannelURBState(pipeIndex, USB_URBState::Done);
                }
                else
                {
                    channel.TransferActive = false;
                    SetChannelURBState(pipeIndex, USB_URBState::NotReady);
                }
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
            if (++channel.ErrorCount > 2)
            {
                FinishDMATransfer(pipeIndex, true, false, nullptr);
                channel.ErrorCount = 0;
                channel.TransferActive = false;
                channel.RetryOnNextSOF = false;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else
            {
                // Resume the current hardware transaction. In DMA mode HCTSIZ and
                // HCDMA identify the untransferred portion and must not be rebuilt.
                ActivateChannel(pipeIndex);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::DiscardFromFIFO(size_t length)
{
    uint32_t discardBuffer;

    while (length != 0)
    {
        const size_t chunkLength = std::min(length, sizeof(discardBuffer));
        m_Driver->ReadFromFIFO(&discardBuffer, chunkLength);
        length -= chunkLength;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost_STM32::HandleRxFIFONotEmptyIRQ()
{
    const uint32_t GRXSTSP = m_Port->GRXSTSP;
    const uint32_t packetStatus = (GRXSTSP & USB_OTG_GRXSTSP_PKTSTS_Msk) >> USB_OTG_GRXSTSP_PKTSTS_Pos;

    if (packetStatus == USB_PKTSTS_HOST_IN_DATA_RCV)
    {
        const USB_PipeIndex pipeIndex     = (GRXSTSP & USB_OTG_GRXSTSP_EPNUM_Msk) >> USB_OTG_GRXSTSP_EPNUM_Pos;
        const uint32_t      bytesReceived = (GRXSTSP & USB_OTG_GRXSTSP_BCNT_Msk)  >> USB_OTG_GRXSTSP_BCNT_Pos;

        if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT)
        {
            DiscardFromFIFO(bytesReceived);
            return;
        }

        USBHostChannelData&         channel = m_ChannelStates[pipeIndex];
        USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

        if (channel.CancelHaltPending || channel.PendingHaltURBState != USB_URBState::Idle)
        {
            DiscardFromFIFO(bytesReceived);
            return;
        }

        if (bytesReceived > 0)
        {
            const bool transferFits
                = channel.BytesTransferred <= channel.RequestedTransferLength
                && bytesReceived <= channel.RequestedTransferLength - channel.BytesTransferred;

            if (channel.TransferActive && channel.TransferBuffer != nullptr && transferFits)
            {
                m_Driver->ReadFromFIFO(channel.TransferBuffer, bytesReceived);

                channel.TransferBuffer += bytesReceived;
                channel.BytesTransferred += bytesReceived;

                const uint32_t transferPacketCount = (channelRegs.HCTSIZ & USB_OTG_HCTSIZ_PKTCNT) >> USB_OTG_HCTSIZ_PKTCNT_Pos;

                if (channel.MaxPacketSize == bytesReceived && transferPacketCount > 0)
                {
                    // Re-activate the channel when more packets are expected.
                    ActivateChannel(pipeIndex);
                    channel.ToggleIn ^= 1;
                }
            }
            else
            {
                DiscardFromFIFO(bytesReceived);
                if (channel.TransferActive)
                {
                    channel.RetryOnNextSOF = false;
                    channel.PendingHaltURBState = USB_URBState::Error;
                    HaltChannelInternal(pipeIndex);
                }
            }
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
            if (hprt0Src & USB_OTG_HPRT_PCSTS) {
                m_Driver->IRQDeviceConnected();
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
