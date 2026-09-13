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

#pragma once

#include <stdint.h>
#include <strings.h>
#include <System/Platform.h>
#include <System/Sections.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/USB/USBCommon.h>

class PString;
enum class USB_Speed : uint8_t;
enum class USB_OTG_ID : int;

namespace kernel
{
class USB_STM32;
enum class IRQResult : int;
enum class USBH_InitialTransactionPID : uint8_t;

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

#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
struct USBHostChannelDiagnostics
{
    uint32_t SubmitRequestCount = 0;
    uint32_t SubmitRequestFailureCount = 0;
    uint32_t StartTransferCount = 0;
    uint32_t StartTransferFailureCount = 0;
    uint32_t TransferCompleteIRQCount = 0;
    uint32_t NakNyetIRQCount = 0;
    uint32_t ChannelHaltIRQCount = 0;
    uint32_t FrameOverrunIRQCount = 0;
    uint32_t AckIRQCount = 0;
    uint32_t StallIRQCount = 0;
    uint32_t TransactionErrorIRQCount = 0;
    uint32_t NotifyDoneCount = 0;
    uint32_t NotifyNotReadyCount = 0;
    uint32_t NotifyStallCount = 0;
    uint32_t NotifyErrorCount = 0;
};
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

struct USBHostChannelData
{
    USB_RequestDirection        Direction;                      // Endpoint direction.
    USB_Speed                   Speed;                          // USB Host Channel speed.
    USB_TransferType            EndpointType;                   // Endpoint Type.
    uint8_t                     DoPing;                         // Enable or disable the use of the PING protocol for HS mode.
    uint16_t                    MaxPacketSize;                  // Endpoint Max packet size.
    uint16_t                    MaxDMAPacketCount;              // Maximum packets in one direct DMA submission.
    uint16_t                    BounceDMAPacketCount;           // Maximum packets in one bounce-buffer DMA submission.
    uint16_t                    InitialDataPID;                 // Initial data PID.
    uint8_t*                    TransferBuffer;                 // Single buffer base or current vector buffer position.
    const USB_TransferSegment*  TransferSegments = nullptr;     // DMA buffers forming one continuous USB data phase.
    uint8_t*                    DMATransferBuffer = nullptr;    // Buffer currently owned by HCDMA.
    size_t                      XferSize;                       // Current OTG Channel transfer size.
    size_t                      TransferDataLength = 0;         // Caller-visible bytes represented by the current DMA transfer.
    size_t                      RequestedTransferLength;        // Transfer length as requested by user.
    size_t                      BytesTransferred;               // Bytes transferred so far during the transaction.
    uint32_t                    TransferPacketCount = 0;        // Packets programmed for the current DMA transfer.
    USB_URBState                PendingHaltURBState = USB_URBState::Idle; // Terminal state reported after the channel halt completes.
    bool                        TransferActive = false;         // True while the current transfer may be continued internally.
    bool                        DMATransferActive = false;      // True while HCDMA owns the current DMA buffer.
    bool                        DMAUsesBounceBuffer = false;    // True when the current DMA transfer uses the staging buffer.
    bool                        ShortPacketReceived = false;    // True after a DMA IN transfer terminates with a short packet.
    bool                        CancelHaltPending = false;      // True while a synchronous cancellation waits for channel halt.
    bool                        StartOnNextSOF = false;         // Deferred start for frame-sensitive transfers.
    bool                        RetryOnNextSOF = false;         // Deferred retry requested from channel halt handling.
    bool                        ToggleIn;                       // IN transfer current toggle flag.
    bool                        ToggleOut;                      // OUT transfer current toggle flag.
    uint16_t                    TransferSegmentIndex = 0;       // Current segment in a DMA vector request.
    uint32_t                    ErrorCount;                     // Host channel error count.
    USB_URBState                URBState;                       // URB state.
    USB_HostChannelState        ChannelState;                   // Host Channel state.
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    USBHostChannelDiagnostics   Diagnostics;
    uint32_t                    LastInterrupts = 0;             // Interrupt snapshot for the current channel event.
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
};

#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
struct USBHostChannelErrorSnapshot
{
    USB_PipeIndex        PipeIndex = USB_INVALID_PIPE;
    uint32_t             Interrupts = 0;
    uint32_t             HCCHAR = 0;
    uint32_t             HCINT = 0;
    uint32_t             HCTSIZ = 0;
    uint32_t             HCDMA = 0;
    size_t               BytesTransferred = 0;
    size_t               RequestedTransferLength = 0;
    size_t               TransferDataLength = 0;
    size_t               XferSize = 0;
    USB_HostChannelState ChannelState = USB_HostChannelState::IDLE;
    bool                 CancelHaltPending = false;
    bool                 DMATransferActive = false;
    bool                 Pending = false;
};
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

class USBHost_STM32
{
public:
    static constexpr uint32_t CHANNEL_COUNT = 16;
    static constexpr size_t DMA_BOUNCE_BUFFER_SIZE = 1024;

    USBHost_STM32();

    bool Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense);
    bool InitializeController();
    void Shutdown();

    USB_Speed   HostGetSpeed() const;
    int32_t     GetMaxPipeCount() const { return CHANNEL_COUNT; }

    bool        StartHost();
    bool        StopHost();
    bool        SetPortReset(bool resetActive);
    uint32_t    GetCurrentFrame();
    bool        SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize);
    bool        HaltChannel(USB_PipeIndex pipeIndex);
    bool        SubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, const USB_TransferSegment* segments, size_t segmentCount, size_t length, bool doPing);


    bool        SetDataToggle(USB_PipeIndex pipeIndex, bool toggle);
    bool        GetDataToggle(USB_PipeIndex pipeIndex) const;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    size_t      GetPipeDebugEntryCount(USB_PipeIndex pipeIndex) const;
    bool        GetPipeDebugEntryLabel(USB_PipeIndex pipeIndex, size_t entryIndex, PString* outLabel) const;
    bool        GetPipeDebugEntryValue(USB_PipeIndex pipeIndex, size_t entryIndex, PString* outValue) const;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS

private:
    void SetChannelURBState(USB_PipeIndex pipeIndex, USB_URBState state);

    void DriveVBus(bool state);

    bool SelectPhyClock(uint32_t clock);
    void ActivateChannel(USB_PipeIndex pipeIndex);
  
    uint32_t PrepareDMATransfer(USB_PipeIndex pipeIndex);
    bool StartTransfer(USB_PipeIndex pipeIndex, bool dma);
    bool FinishDMATransfer(USB_PipeIndex pipeIndex, bool commitTransfer, bool transferComplete, bool* madeProgress);
    void CompleteChannelCancellation(USB_PipeIndex pipeIndex);
    void RecoverDMATransferError(USB_PipeIndex pipeIndex);
    void UpdateDataToggle(USBHostChannelData& channel, uint32_t packetCount);
    bool HaltChannelInternal(USB_PipeIndex pipeIndex);
    bool DoPing(USB_PipeIndex pipeIndex);


    static IRQResult IRQCallback(IRQn_Type irq, void* userData);

    IRQResult HandleIRQ();
    void HandleChannelInIRQ(USB_PipeIndex pipeIndex);
    void HandleChannelOutIRQ(USB_PipeIndex pipeIndex);
    void DiscardFromFIFO(size_t length);
    void HandleRxFIFONotEmptyIRQ();
    void HandlePortIRQ();


    USB_STM32*                  m_Driver = nullptr;
    uint8_t                     (*m_DMABounceBuffers)[DMA_BOUNCE_BUFFER_SIZE] = nullptr;

    USB_OTG_GlobalTypeDef*      m_Port = nullptr;
    USB_OTG_HostTypeDef*        m_Host = nullptr;
    volatile uint32_t*          m_HPRT = nullptr;
    USB_OTG_HostChannelTypeDef* m_HostChannels = nullptr;
    volatile uint32_t*          m_PCGCCTL = nullptr;

    USBHostChannelData          m_ChannelStates[CHANNEL_COUNT];
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    USBHostChannelErrorSnapshot m_ChannelErrorSnapshot;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    KConditionVariable          m_ChannelHaltCondition;
};


} // namespace kernel
