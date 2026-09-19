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

enum class USB_Speed : uint8_t;
enum class USB_OTG_ID : int;

namespace kernel
{
class USB_STM32;
enum class IRQResult : int;


static constexpr uint32_t USB_OTG_ENUMERATED_SPEED_HIGH                 = 0; // High-speed mode
static constexpr uint32_t USB_OTG_ENUMERATED_SPEED_FULL_SPEED_USE_HS    = 1; // Full speed with high-speed PHY.
static constexpr uint32_t USB_OTG_ENUMERATED_SPEED_FULL                 = 3; // Full speed with internal PHY

class USBDevice_STM32
{
public:
    ~USBDevice_STM32();

    bool Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense, bool useSOF);

    bool        CompleteDeviceReset(uint32_t generation);
    bool        Recover();

    USB_Speed   DeviceGetSpeed() const;
    void        EndpointStall(uint8_t endpointAddr);
    void        EndpointClearStall(uint8_t endpointAddr);
    bool        EndpointOpen(const USB_DescEndpoint& endpointDescriptor);
    void        EndpointClose(uint8_t endpointAddr);
    void        EndpointCloseAll();
    bool        EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength);
    bool        SetAddress(uint8_t deviceAddr);
    bool        ActivateRemoteWakeup(bool activate);

private:
    static constexpr uint32_t ENDPOINT_COUNT = 9;
    static constexpr size_t DMA_BOUNCE_BUFFER_SIZE = 1024;
    static constexpr size_t DMA_BUFFER_COUNT = ENDPOINT_COUNT * 2;
    static_assert((DMA_BOUNCE_BUFFER_SIZE % __SCB_DCACHE_LINE_SIZE) == 0);

    void        ConfigureDevice();
    void        RequestRecovery();
    void        SetSpeed(USB_Speed speed);
    void        DeviceConnect();
    void        DeviceDisconnect();

    uint32_t    CalculateRXFIFOSize(uint32_t maxEndpointSize) const;
    void        ResetReceived();
    bool        DisableInEndpointsFromIRQ();
    void        SetTurnaround(USB_Speed speed);

    bool        EndpointDisable(uint8_t endpointAddr, bool stall);
    bool        StartDMATransfer(uint8_t endpointAddr, uint32_t transferGeneration);
    bool        FinishDMATransfer(uint8_t endpointAddr, bool commitTransfer, bool* shortPacketReceived);
    void        CancelEndpointTransfer(uint8_t endpointAddr);
    void        CancelAllEndpointTransfers();
    void        PrepareSetupPackets();
    void        EndpointSchedulePackets(uint8_t endpointAddr, uint32_t packetCount, uint32_t totalLength);
    bool        FlushTxFifoFromIRQ(uint32_t fifoIndex);
    bool        FlushRxFifoFromIRQ();

    static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IRQResult HandleIRQ();

    void HandleOutEndpointIRQ();
    void HandleInEndpointIRQ();


    USB_STM32* m_Driver = nullptr;

    uint8_t*                    m_DMABounceBuffers = nullptr;
    USB_OTG_GlobalTypeDef*      m_Port = nullptr;
    USB_OTG_DeviceTypeDef*      m_Device = nullptr;
    USB_OTG_OUTEndpointTypeDef* m_OutEndpoints = nullptr;
    USB_OTG_INEndpointTypeDef*  m_InEndpoints = nullptr;


    struct EndpointTransferState
    {
        void ResetTransfer()
        {
            ++Generation;
            Buffer = nullptr;
            BufferSize = 0;
            BytesTransferred = 0;
            DMATransferBuffer = nullptr;
            DMATransferSize = 0;
            DMATransferDataLength = 0;
            TransferActive = false;
            DMATransferActive = false;
            DMAUsesBounceBuffer = false;
        }

        void Reset()
        {
            ResetTransfer();
            EndpointMaxSize = 0;
            Interval = 0;
        }

        uint8_t*    Buffer = nullptr;
        size_t      BufferSize = 0;
        size_t      BytesTransferred = 0;
        uint8_t*    DMATransferBuffer = nullptr;
        size_t      DMATransferSize = 0;
        size_t      DMATransferDataLength = 0;
        uint32_t    EndpointMaxSize = 0;
        uint32_t    Generation = 0;
        uint8_t     Interval = 0;
        bool        TransferActive = false;
        bool        DMATransferActive = false;
        bool        DMAUsesBounceBuffer = false;
    };

    EndpointTransferState* GetEndpointTranferState(uint8_t endpointAddr)
    {
        const uint8_t endpointNumber = USB_ADDRESS_EPNUM(endpointAddr);

        if (endpointNumber < ENDPOINT_COUNT) {
            return (endpointAddr & USB_ADDRESS_DIR_IN) ? &m_TransferStatusIn[endpointNumber] : &m_TransferStatusOut[endpointNumber];
        }
        return nullptr;
    }

    uint8_t* GetDMABounceBuffer(uint8_t endpointAddr)
    {
        const size_t directionOffset = (endpointAddr & USB_ADDRESS_DIR_IN) ? ENDPOINT_COUNT : 0;
        return m_DMABounceBuffers + (directionOffset + USB_ADDRESS_EPNUM(endpointAddr)) * DMA_BOUNCE_BUFFER_SIZE;
    }

    EndpointTransferState  m_TransferStatusIn[ENDPOINT_COUNT];
    EndpointTransferState  m_TransferStatusOut[ENDPOINT_COUNT];

    uint32_t    m_AllocatedTXFIFOWords = 0; // TX FIFO size in words (IN endpoints).

    bool        m_EnableVBusSense = false;
    bool        m_UseSOF = false;
    bool        m_RecoveryPending = false;
    bool        m_SupportHighSpeed = false;
    bool        m_ResetComplete = false; // Protected by the USB IRQ guard; false until hardware reset succeeds.
    bool        m_DeviceReady = false; // Protected by the USB IRQ guard; true after device-thread reset cleanup.
    uint32_t    m_DeviceGeneration = 0; // Protected by the USB IRQ guard; advanced at reset/recovery entry.
    USB_ControlRequest  m_ControlRequestPackage = {};

};


} // namespace kernel
