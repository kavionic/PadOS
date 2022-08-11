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

#pragma once

#include <stdint.h>
#include <strings.h>
#include <System/Platform.h>
#include <System/Sections.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/USB/USBCommon.h>

enum class USB_Speed : uint8_t;

namespace kernel
{
class USB_STM32;
enum class IRQResult : int;
enum class USB_OTG_ID : int;
enum class USBH_InitialTransactionPID : uint8_t;
enum class USB_URBState : uint8_t;

static constexpr uint32_t USB_OTG_CON_DEVICE_SPEED_HIGH = 0;
static constexpr uint32_t USB_OTG_CON_DEVICE_SPEED_FULL = 1;
static constexpr uint32_t USB_OTG_CON_DEVICE_SPEED_LOW  = 2;

static constexpr uint32_t USB_OTG_DATA_PID_DATA0 = 0;
static constexpr uint32_t USB_OTG_DATA_PID_DATA2 = 1;
static constexpr uint32_t USB_OTG_DATA_PID_DATA1 = 2;
static constexpr uint32_t USB_OTG_DATA_PID_SETUP = 3;

static constexpr uint32_t USB_OTG_HCFG_48_MHZ   = 1;
static constexpr uint32_t USB_OTG_HCFG_6_MHZ    = 2;

enum class USB_HostChannelState : uint8_t
{
    IDLE = 0,
    XFRC,
    NAK,
    NYET,
    STALL,
    XACTERR,
    BBLERR,
    DATATGLERR
};

struct USBHostChannelData
{
    USB_RequestDirection    Direction;                  // Endpoint direction.
    USB_Speed               Speed;                      // USB Host Channel speed.
    USB_TransferType        EndpointType;               // Endpoint Type.
    uint8_t                 DoPing;                     // Enable or disable the use of the PING protocol for HS mode.
    size_t                  MaxPacketSize;              // Endpoint Max packet size.
    uint32_t                InitialDataPID;             // Initial data PID.
    uint8_t*                TransferBuffer;             // Pointer to transfer buffer.
    size_t                  XferSize;                   // Current OTG Channel transfer size.
    size_t                  RequestedTransferLength;    // Transfer length as requested by user.
    size_t                  BytesTransferred;           // Bytes transferred so far during the transaction.
    bool                    ToggleIn;                   // IN transfer current toggle flag.
    bool                    ToggleOut;                  // OUT transfer current toggle flag.
    uint32_t                ErrorCount;                 // Host channel error count.
    USB_URBState            URBState;                   // URB state.
    USB_HostChannelState    ChannelState;               // Host Channel state.
};

class USBHost_STM32
{
public:
    static constexpr uint32_t CHANNEL_COUNT = 16;

    bool Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense);
    void Shutdown();

    USB_Speed   HostGetSpeed() const;
    int32_t     GetMaxPipeCount() const { return CHANNEL_COUNT; }

    IFLASHC bool        StartHost();
    IFLASHC bool        StopHost();
    IFLASHC bool        ResetPort();
    IFLASHC uint32_t    GetCurrentFrame();
    IFLASHC bool        SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize);
    IFLASHC bool        HaltChannel(USB_PipeIndex pipeIndex);
    IFLASHC bool        SubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, void* buffer, size_t length, bool doPing);


    IFLASHC bool SetDataToggle(USB_PipeIndex pipeIndex, bool toggle);
    IFLASHC bool GetDataToggle(USB_PipeIndex pipeIndex) const;

private:
    IFLASHC void SetChannelURBState(USB_PipeIndex pipeIndex, USB_URBState state);

    void DriveVBus(bool state);

    IFLASHC bool SelectPhyClock(uint32_t clock);
  
    IFLASHC bool StartTransfer(USB_PipeIndex pipeIndex, bool dma);
    IFLASHC bool DoPing(USB_PipeIndex pipeIndex);


    IFLASHC static IRQResult IRQCallback(IRQn_Type irq, void* userData);

    IFLASHC IRQResult HandleIRQ();
    IFLASHC void HandleChannelInIRQ(USB_PipeIndex pipeIndex);
    IFLASHC void HandleChannelOutIRQ(USB_PipeIndex pipeIndex);
    IFLASHC void HandleRxFIFONotEmptyIRQ();
    IFLASHC void HandlePortIRQ();


    USB_STM32*                  m_Driver = nullptr;

    USB_OTG_GlobalTypeDef*      m_Port = nullptr;
    USB_OTG_HostTypeDef*        m_Host = nullptr;
    volatile uint32_t*          m_HPRT = nullptr;
    USB_OTG_HostChannelTypeDef* m_HostChannels = nullptr;
    volatile uint32_t*          m_PCGCCTL = nullptr;

    USBHostChannelData          m_ChannelStates[CHANNEL_COUNT];
};


} // namespace kernel
