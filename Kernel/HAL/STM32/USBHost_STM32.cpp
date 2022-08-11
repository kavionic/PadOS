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

#include <Kernel/HAL/STM32/USBHost_STM32.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/IRQDispatcher.h>


namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense)
{
    m_Driver = driver;

    m_Port          = get_usb_from_id(portID);
    m_Host          = reinterpret_cast<USB_OTG_HostTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_HOST_BASE);
    m_HPRT          = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(m_Port) + USB_OTG_HOST_PORT_BASE);
    m_HostChannels  = reinterpret_cast<USB_OTG_HostChannelTypeDef*>(reinterpret_cast<uint8_t*>(m_Port) + USB_OTG_HOST_CHANNEL_BASE);
    m_PCGCCTL       = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(m_Port) + USB_OTG_PCGCCTL_BASE);

    bool result = true;

    m_Driver->EnableIRQ(false);

    // Restart the Phy clock.
    *m_PCGCCTL = 0;

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

    // Set Rx FIFO size.
    m_Port->GRXFSIZ = 2048 / 4;
    m_Port->DIEPTXF0_HNPTXFSIZ
        = ((1024 / 4) << USB_OTG_NPTXFSA_Pos)
        | ((2048 / 4) << USB_OTG_NPTXFD_Pos);
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
        for (TimeValMicros endTime = get_system_time() + TimeValMicros::FromMilliseconds(100); get_system_time() < endTime && (m_HostChannels[i].HCCHAR & USB_OTG_HCCHAR_CHENA); ) {}
    }

    // Clear any pending host interrupts.
    m_Host->HAINT   = ~0u;
    m_Port->GINTSTS = ~0u;

    m_Driver->EnableIRQ(true);

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
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];

    channel.Direction    = direction;
    channel.EndpointType = endpointType;

    if (initialPID == USBH_InitialTransactionPID::Setup)
    {
        channel.InitialDataPID = USB_OTG_DATA_PID_SETUP;
        channel.DoPing = (channel.Speed == USB_Speed::HIGH) ? doPing : false;
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
    channel.RequestedTransferLength = length;
    channel.URBState                = USB_URBState::Idle;
    channel.BytesTransferred        = 0;
    channel.ChannelState            = USB_HostChannelState::IDLE;

    return StartTransfer(pipeIndex, m_Driver->UseDMA());
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

void USBHost_STM32::SetChannelURBState(USB_PipeIndex pipeIndex, USB_URBState state)
{
    m_ChannelStates[pipeIndex].URBState = state;
    m_Driver->IRQPipeURBStateChanged(pipeIndex, state, m_ChannelStates[pipeIndex].BytesTransferred);
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

uint32_t USBHost_STM32::GetCurrentFrame()
{
    return m_Host->HFNUM & USB_OTG_HFNUM_FRNUM;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize)
{
    USBHostChannelData&         channel = m_ChannelStates[pipeIndex];
    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

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

    // Enable top-level host channel interrupt.
    m_Host->HAINTMSK |= 1 << pipeIndex;

    // Enable channel interrupts.
    m_Port->GINTMSK |= USB_OTG_GINTMSK_HCIM;

    const uint32_t hostCoreSpeed = (m_HPRT[0] & USB_OTG_HPRT_PSPD) >> USB_OTG_HPRT_PSPD_Pos;

    const bool lowSpeedDevice = speed == USB_Speed::LOW && hostCoreSpeed != USB_OTG_CON_DEVICE_SPEED_LOW;

    channelRegs.HCCHAR
        = (static_cast<uint32_t>(deviceAddr) << USB_OTG_HCCHAR_DAD_Pos)
        | (static_cast<uint32_t>(USB_ADDRESS_EPNUM(endpointAddr)) << USB_OTG_HCCHAR_EPNUM_Pos)
        | (static_cast<uint32_t>(endpointType) << USB_OTG_HCCHAR_EPTYP_Pos)
        | (static_cast<uint32_t>(maxPacketSize) << USB_OTG_HCCHAR_MPSIZ_Pos)
        | ((channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) ? USB_OTG_HCCHAR_EPDIR : 0)
        | (lowSpeedDevice ? USB_OTG_HCCHAR_LSDEV : 0);

    if (endpointType == USB_TransferType::INTERRUPT || endpointType == USB_TransferType::ISOCHRONOUS) {
        channelRegs.HCCHAR |= USB_OTG_HCCHAR_ODDFRM;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::StartTransfer(USB_PipeIndex pipeIndex, bool dma)
{
    USBHostChannelData& channel = m_ChannelStates[pipeIndex];

    USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

    if (channel.Speed == USB_Speed::HIGH)
    {
        // In DMA mode host core automatically issues ping in case of NYET/NAK.
        if (dma && (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)) {
            channelRegs.HCINTMSK &= ~(USB_OTG_HCINTMSK_NYET | USB_OTG_HCINTMSK_ACKM | USB_OTG_HCINTMSK_NAKM);
        }

        if (!dma && channel.DoPing)
        {
            DoPing(pipeIndex);
            return true;
        }
    }

    uint32_t packetCount;
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

    channelRegs.HCTSIZ
        = (channel.XferSize & USB_OTG_HCTSIZ_XFRSIZ)
        | ((packetCount << USB_OTG_HCTSIZ_PKTCNT_Pos) & USB_OTG_HCTSIZ_PKTCNT_Msk)
        | ((channel.InitialDataPID << USB_OTG_HCTSIZ_DPID_Pos) & USB_OTG_HCTSIZ_DPID);

    if (dma) {
        channelRegs.HCDMA = reinterpret_cast<uint32_t>(channel.TransferBuffer); // TransferBuffer must be 32-bit aligned.
    }

    const bool isOddFrame = (m_Host->HFNUM & 0x01) == 0;
    if (isOddFrame) {
        channelRegs.HCCHAR &= ~USB_OTG_HCCHAR_ODDFRM;
    } else {
        channelRegs.HCCHAR |= USB_OTG_HCCHAR_ODDFRM;
    }

    uint32_t tmpreg = channelRegs.HCCHAR;
    tmpreg &= ~USB_OTG_HCCHAR_CHDIS;

    if (channel.Direction == USB_RequestDirection::DEVICE_TO_HOST) {
        tmpreg |= USB_OTG_HCCHAR_EPDIR;
    } else {
        tmpreg &= ~USB_OTG_HCCHAR_EPDIR;
    }
    tmpreg |= USB_OTG_HCCHAR_CHENA;
    channelRegs.HCCHAR = tmpreg;

    if (dma) {
        return true;
    }

    if (channel.Direction ==  USB_RequestDirection::HOST_TO_DEVICE && channel.RequestedTransferLength > 0)
    {
        const uint32_t lengthWords = (channel.RequestedTransferLength + 3) / 4;
        // Non-periodic transfer.
        if (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)
        {
            const uint32_t fifoSpace = (m_Port->HNPTXSTS & USB_OTG_GNPTXSTS_NPTXFSAV_Msk) >> USB_OTG_GNPTXSTS_NPTXFSAV_Pos;
            if (lengthWords > fifoSpace) {
                m_Port->GINTMSK |= USB_OTG_GINTMSK_NPTXFEM;
            }
        }
        else
        {
            const uint32_t fifoSpace = (m_Host->HPTXSTS & USB_OTG_HPTXSTS_PTXFSAVL_Msk) >> USB_OTG_HPTXSTS_PTXFSAVL_Pos;
            if (lengthWords > fifoSpace) {
                m_Port->GINTMSK |= USB_OTG_GINTMSK_PTXFEM;
            }
        }
        m_Driver->WriteToFIFO(pipeIndex, channel.TransferBuffer, channel.RequestedTransferLength);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost_STM32::HaltChannel(USB_PipeIndex pipeIndex)
{
    if (pipeIndex < 0 || pipeIndex >= CHANNEL_COUNT) {
        return false;
    }
    USB_OTG_HostChannelTypeDef& channelRegs     = m_HostChannels[pipeIndex];
    USB_TransferType            endpointType    = USB_TransferType((channelRegs.HCCHAR & USB_OTG_HCCHAR_EPTYP) >> USB_OTG_HCCHAR_EPTYP_Pos);
    const bool                  channelEnabled  = (channelRegs.HCCHAR & USB_OTG_HCCHAR_CHENA) != 0;
    const bool                  dmaEnabled      = (m_Port->GAHBCFG & USB_OTG_GAHBCFG_DMAEN) != 0;

    if (dmaEnabled && !channelEnabled)
    {
        // Enable channel halt interrupt.
        channelRegs.HCINTMSK |= USB_OTG_HCINTMSK_CHHM;
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
        if (interrupts & USB_OTG_GINTSTS_PTXFE) {
            m_Port->GINTSTS = USB_OTG_GINTSTS_PTXFE;
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
                m_HostChannels[i].HCINTMSK |= USB_OTG_HCINT_NAK;
            }
            m_Driver->IRQStartOfFrame();
            m_Port->GINTSTS = USB_OTG_GINTSTS_SOF;
        }

        // Handle RX FIFO level interrupt.
        if (interrupts & USB_OTG_GINTSTS_RXFLVL)
        {
            m_Port->GINTMSK &= ~USB_OTG_GINTSTS_RXFLVL;
            HandleRxFIFONotEmptyIRQ();
            m_Port->GINTMSK |= USB_OTG_GINTSTS_RXFLVL;
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

    if (interrupts & USB_OTG_HCINT_AHBERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_AHBERR;
        channel.ChannelState = USB_HostChannelState::XACTERR;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_BBERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_BBERR;
        channel.ChannelState = USB_HostChannelState::BBLERR;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_ACK)
    {
        channelRegs.HCINT = USB_OTG_HCINT_ACK;
    }
    else if (interrupts & USB_OTG_HCINT_STALL)
    {
        channelRegs.HCINT = USB_OTG_HCINT_STALL;
        channel.ChannelState = USB_HostChannelState::STALL;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_DTERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_DTERR;
        channel.ChannelState = USB_HostChannelState::DATATGLERR;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_TXERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_TXERR;
        channel.ChannelState = USB_HostChannelState::XACTERR;
        HaltChannel(pipeIndex);
    }

    if (interrupts & USB_OTG_HCINT_FRMOR)
    {
        HaltChannel(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_FRMOR;
    }
    else if (interrupts & USB_OTG_HCINT_XFRC)
    {
        if (m_Driver->UseDMA()) {
            channel.BytesTransferred = channel.XferSize - (channelRegs.HCTSIZ & USB_OTG_HCTSIZ_XFRSIZ);
        }
        channel.ChannelState = USB_HostChannelState::XFRC;
        channel.ErrorCount   = 0;

        channelRegs.HCINT = USB_OTG_HCINT_XFRC;

        if (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)
        {
            HaltChannel(pipeIndex);
            channelRegs.HCINT = USB_OTG_HCINT_NAK;
        }
        else if (channel.EndpointType == USB_TransferType::INTERRUPT || channel.EndpointType == USB_TransferType::ISOCHRONOUS)
        {
            channelRegs.HCCHAR |= USB_OTG_HCCHAR_ODDFRM;
            SetChannelURBState(pipeIndex, USB_URBState::Done);
        }

        if (m_Driver->UseDMA())
        {
            const bool oddPacketCount = ((channel.XferSize / channel.MaxPacketSize) & 1) != 0;
            if (oddPacketCount) {
                channel.ToggleIn ^= 1;
            }
        }
        else
        {
            channel.ToggleIn ^= 1;
        }
    }
    else if (interrupts & USB_OTG_HCINT_CHH)
    {
        // Disable host channel Halt interrupt.
//        channelRegs.HCINTMSK &= ~USB_OTG_HCINTMSK_CHHM;

        if (channel.ChannelState == USB_HostChannelState::XFRC)
        {
            SetChannelURBState(pipeIndex, USB_URBState::Done);
        }
        else if (channel.ChannelState == USB_HostChannelState::STALL)
        {
            SetChannelURBState(pipeIndex, USB_URBState::Stall);
        }
        else if (channel.ChannelState == USB_HostChannelState::XACTERR || channel.ChannelState == USB_HostChannelState::DATATGLERR)
        {
            if (++channel.ErrorCount > 2)
            {
                channel.ErrorCount = 0;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else
            {
                SetChannelURBState(pipeIndex, USB_URBState::NotReady);

                // Re-activate the channel.
                set_bit_group(channelRegs.HCCHAR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS, USB_OTG_HCCHAR_CHENA);
            }
        }
        else if (channel.ChannelState == USB_HostChannelState::NAK)
        {
            SetChannelURBState(pipeIndex, USB_URBState::NotReady);

            // Re-activate the channel.
            set_bit_group(channelRegs.HCCHAR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS, USB_OTG_HCCHAR_CHENA);
        }
        else if (channel.ChannelState == USB_HostChannelState::BBLERR)
        {
            channel.ErrorCount++;
            SetChannelURBState(pipeIndex, USB_URBState::Error);
        }
        channelRegs.HCINT = USB_OTG_HCINT_CHH;
    }
    else if (interrupts & USB_OTG_HCINT_NAK)
    {
        if (channel.EndpointType == USB_TransferType::INTERRUPT)
        {
            channel.ErrorCount = 0;
            HaltChannel(pipeIndex);
        }
        else if (channel.EndpointType == USB_TransferType::CONTROL || channel.EndpointType == USB_TransferType::BULK)
        {
            channel.ErrorCount = 0;

            if (!m_Driver->UseDMA())
            {
                // Workaround NAK interrupt flood issue.
                channelRegs.HCINTMSK &= ~USB_OTG_HCINT_NAK;
                channel.ChannelState = USB_HostChannelState::NAK;
                HaltChannel(pipeIndex);
            }
        }
        channelRegs.HCINT = USB_OTG_HCINT_NAK;
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

    if (interrupts & USB_OTG_HCINT_AHBERR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_AHBERR;
        channel.ChannelState = USB_HostChannelState::XACTERR;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_ACK)
    {
        channelRegs.HCINT = USB_OTG_HCINT_ACK;

        if (channel.DoPing)
        {
            channel.DoPing = false;
            SetChannelURBState(pipeIndex, USB_URBState::NotReady);
            HaltChannel(pipeIndex);
        }
    }
    else if (interrupts & USB_OTG_HCINT_FRMOR)
    {
        channelRegs.HCINT = USB_OTG_HCINT_FRMOR;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_XFRC)
    {
        channel.ErrorCount = 0;

        // Transaction completed with NYET state, update do ping state.
        if (interrupts & USB_OTG_HCINT_NYET)
        {
            channel.DoPing = true;
            channelRegs.HCINT = USB_OTG_HCINT_NYET;
        }
        channelRegs.HCINT = USB_OTG_HCINT_XFRC;
        channel.ChannelState = USB_HostChannelState::XFRC;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_NYET)
    {
        channel.ChannelState = USB_HostChannelState::NYET;
        channel.DoPing = true;
        channel.ErrorCount = 0;
        HaltChannel(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_NYET;
    }
    else if (interrupts & USB_OTG_HCINT_STALL)
    {
        channelRegs.HCINT = USB_OTG_HCINT_STALL;
        channel.ChannelState = USB_HostChannelState::STALL;
        HaltChannel(pipeIndex);
    }
    else if (interrupts & USB_OTG_HCINT_NAK)
    {
        channel.ErrorCount = 0;
        channel.ChannelState = USB_HostChannelState::NAK;

        if (!channel.DoPing)
        {
            if (channel.Speed == USB_Speed::HIGH)
            {
                channel.DoPing = true;
            }
        }

        HaltChannel(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_NAK;
    }
    else if (interrupts & USB_OTG_HCINT_TXERR)
    {
        if (!m_Driver->UseDMA())
        {
            channel.ChannelState = USB_HostChannelState::XACTERR;
            HaltChannel(pipeIndex);
        }
        else
        {
            if (++channel.ErrorCount > 2)
            {
                channel.ErrorCount = 0;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else
            {
                SetChannelURBState(pipeIndex, USB_URBState::NotReady);
            }
        }
        channelRegs.HCINT = USB_OTG_HCINT_TXERR;
    }
    else if (interrupts & USB_OTG_HCINT_DTERR)
    {
        channel.ChannelState = USB_HostChannelState::DATATGLERR;
        HaltChannel(pipeIndex);
        channelRegs.HCINT = USB_OTG_HCINT_DTERR;
    }
    else if (interrupts & USB_OTG_HCINT_CHH)
    {
        // Disable host channel Halt interrupt.
//        channelRegs.HCINTMSK &= ~USB_OTG_HCINTMSK_CHHM;
        if (channel.ChannelState == USB_HostChannelState::XFRC)
        {
            SetChannelURBState(pipeIndex, USB_URBState::Done);
            if (channel.EndpointType == USB_TransferType::BULK || channel.EndpointType == USB_TransferType::INTERRUPT)
            {
                if (!m_Driver->UseDMA())
                {
                    channel.ToggleOut ^= 1;
                }
                else if (channel.RequestedTransferLength > 0)
                {
                    const uint32_t packetCount = (channel.RequestedTransferLength + channel.MaxPacketSize - 1) / channel.MaxPacketSize;

                    if (packetCount & 1) {
                        channel.ToggleOut ^= 1;
                    }
                }
            }
        }
        else if (channel.ChannelState == USB_HostChannelState::NAK)
        {
            SetChannelURBState(pipeIndex, USB_URBState::NotReady);
        }
        else if (channel.ChannelState == USB_HostChannelState::NYET)
        {
            SetChannelURBState(pipeIndex, USB_URBState::NotReady);
        }
        else if (channel.ChannelState == USB_HostChannelState::STALL)
        {
            SetChannelURBState(pipeIndex, USB_URBState::Stall);
        }
        else if (channel.ChannelState == USB_HostChannelState::XACTERR || channel.ChannelState == USB_HostChannelState::DATATGLERR)
        {
            if (++channel.ErrorCount > 2)
            {
                channel.ErrorCount = 0;
                SetChannelURBState(pipeIndex, USB_URBState::Error);
            }
            else
            {
                SetChannelURBState(pipeIndex, USB_URBState::NotReady);
                // Re-activate the channel.
                set_bit_group(channelRegs.HCCHAR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS, USB_OTG_HCCHAR_CHENA);
            }
        }
        channelRegs.HCINT = USB_OTG_HCINT_CHH;
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

        USBHostChannelData&         channel = m_ChannelStates[pipeIndex];
        USB_OTG_HostChannelTypeDef& channelRegs = m_HostChannels[pipeIndex];

        if (bytesReceived > 0 && channel.TransferBuffer != nullptr)
        {
            if ((channel.BytesTransferred + bytesReceived) <= channel.RequestedTransferLength)
            {
                m_Driver->ReadFromFIFO(channel.TransferBuffer, bytesReceived);

                channel.TransferBuffer += bytesReceived;
                channel.BytesTransferred += bytesReceived;

                const uint32_t transferPacketCount = (channelRegs.HCTSIZ & USB_OTG_HCTSIZ_PKTCNT) >> USB_OTG_HCTSIZ_PKTCNT_Pos;

                if (channel.MaxPacketSize == bytesReceived && transferPacketCount > 0)
                {
                    // Re-activate the channel when more packets are expected.
                    set_bit_group(channelRegs.HCCHAR, USB_OTG_HCCHAR_CHENA | USB_OTG_HCCHAR_CHDIS, USB_OTG_HCCHAR_CHENA);
                    channel.ToggleIn ^= 1;
                }
            }
            else
            {
                SetChannelURBState(pipeIndex, USB_URBState::Error);
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

    hprt0Dst &= ~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET | USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG);

    // Check whether port connect detected.
    if (hprt0Src & USB_OTG_HPRT_PCDET)
    {
        if (hprt0Src & USB_OTG_HPRT_PCSTS) {
            m_Driver->IRQDeviceConnected();
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
            m_Driver->IRQPortEnableChange(false);
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
