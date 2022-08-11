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

namespace kernel
{
class USB_STM32;
enum class IRQResult : int;
enum class USB_OTG_ID : int;


static constexpr uint32_t USB_OTG_ENUMERATED_SPEED_HIGH                 = 0; // High-speed mode
static constexpr uint32_t USB_OTG_ENUMERATED_SPEED_FULL_SPEED_USE_HS    = 1; // Full speed with high-speed PHY.
static constexpr uint32_t USB_OTG_ENUMERATED_SPEED_FULL                 = 3; // Full speed with internal PHY

class USBDevice_STM32
{
public:
    bool Setup(USB_STM32* driver, USB_OTG_ID portID, bool enableVBusSense, bool useSOF);

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

    void        SetSpeed(USB_Speed speed);
    void        DeviceConnect();
    void        DeviceDisconnect();

    uint32_t    CalculateRXFIFOSize(uint32_t maxEndpointSize) const;
    void        ResetReceived();
    void        UpdateGRXFSIZ();
    void        SetTurnaround(USB_Speed speed);

    void        EndpointDisable(uint8_t endpointAddr, bool stall);
    void        EndpointSchedulePackets(uint8_t endpointAddr, uint32_t packetCount, uint32_t totalLength);

    IFLASHC static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IFLASHC IRQResult HandleIRQ();

    IFLASHC void HandleRxFIFONotEmptyIRQ();
    IFLASHC void HandleOutEndpointIRQ();
    IFLASHC void HandleInEndpointIRQ();


    USB_STM32* m_Driver;

    USB_OTG_GlobalTypeDef*      m_Port = nullptr;
    USB_OTG_DeviceTypeDef*      m_Device = nullptr;
    USB_OTG_OUTEndpointTypeDef* m_OutEndpoints = nullptr;
    USB_OTG_INEndpointTypeDef*  m_InEndpoints = nullptr;


    struct EndpointTransferState
    {
        void Reset()
        {
            Buffer = nullptr;
            TotalLength = 0;
            EndpointMaxSize = 0;
            Interval = 0;
        }

        uint8_t* Buffer;
        uint32_t    TotalLength;
        uint32_t    EndpointMaxSize;
        uint8_t     Interval;
    };

    EndpointTransferState* GetEndpointTranferState(uint8_t endpointAddr)
    {
        const uint8_t epnum = USB_ADDRESS_EPNUM(endpointAddr);

        if (epnum < ENDPOINT_COUNT) {
            return (endpointAddr & USB_ADDRESS_DIR_IN) ? &m_TransferStatusIn[epnum] : &m_TransferStatusOut[epnum];
        }
        return nullptr;
    }

    EndpointTransferState  m_TransferStatusIn[ENDPOINT_COUNT];
    EndpointTransferState  m_TransferStatusOut[ENDPOINT_COUNT];

    uint32_t    m_Enpoint0InPending = 0;
    uint32_t    m_Enpoint0OutPending = 0;

    uint32_t    m_AllocatedTXFIFOWords = 0; // TX FIFO size in words (IN endpoints).
    bool        m_UpdateRXFIFOSize = false; // Flag set if RX FIFO size needs an update.

    bool        m_SupportHighSpeed = false;
    USB_ControlRequest  m_ControlRequestPackage;
};


} // namespace kernel
