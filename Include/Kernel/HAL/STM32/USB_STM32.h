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
// Created: 22.05.2022 17:00

#pragma once

#include <stdint.h>
#include <System/Sections.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/USB/USBDriver.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBProtocol.h>

namespace kernel
{

enum class IRQResult : int;
enum class USB_OTG_ID : int;

static constexpr uint32_t USB_PKTSTS_NAK            = 1; // Global OUT NAK (triggers an interrupt).
static constexpr uint32_t USB_PKTSTS_OUT_DATA_RCV   = 2; // OUT data packet received.
static constexpr uint32_t USB_PKTSTS_OUT_XFR_DONE   = 3; // OUT transfer completed (triggers an interrupt).
static constexpr uint32_t USB_PKTSTS_SETUP_XFR_DONE = 4; // SETUP transaction completed (triggers an interrupt).
static constexpr uint32_t USB_PKTSTS_SETUP_DATA_RCV = 6; // SETUP data packet received.

enum class USB_Mode : int
{
    Device,
    Host,
    Dual
};

class USB_STM32 : public USBDriver
{
public:
    USB_STM32();
    ~USB_STM32();

    bool Setup(USB_OTG_ID portID, USB_Mode mode, const PinMuxTarget& pinDM, const PinMuxTarget& pinDP, const PinMuxTarget& pinID, DigitalPinID pinVBus, bool useSOF = false);

    virtual void EndpointStall(uint8_t endpointAddr) override;
    virtual void EndpointClearStall(uint8_t endpointAddr) override;
    virtual bool EndpointOpen(const USB_DescEndpoint& endpointDescriptor) override;
    virtual void EndpointClose(uint8_t endpointAddr) override;
    virtual void EndpointCloseAll() override;
    virtual bool EndpointTransfer(uint8_t endpointAddr, uint8_t* buffer, size_t totalLength) override;
    virtual bool SetAddress(uint8_t deviceAddr) override;

private:
    static constexpr uint32_t ENDPOINT_COUNT = 9;

    volatile uint32_t* GetFIFOBase(uint32_t endpoint) { return m_FIFOBase + endpoint * USB_OTG_FIFO_SIZE / 4; }
    uint32_t    CalculateRXFIFOSize(uint32_t maxEndpointSize) const;
    void        Connect();
    void        Disconnect();
    void        ResetReceived();
    void        UpdateGRXFSIZ();
    void        SetSpeed(USB_Speed speed);
    USB_Speed   GetSpeed() const;
    void        SetTurnaround(USB_Speed speed);

    void        ReadFromFIFO(void* buffer, size_t length);
    void        WriteToFIFO(uint32_t fifoIndex, const void* buffer, size_t length);
    void        EndpointDisable(uint8_t endpointAddr, bool stall);
    void        EndpointSchedulePackets(uint8_t endpointAddr, uint32_t packetCount, uint32_t totalLength);

    IFLASHC static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IFLASHC IRQResult HandleIRQ();

    IFLASHC void HandleRXFIFOLevelIRQ();
    IFLASHC void HandleOutEndpointIRQ();
    IFLASHC void HandleInEndpointIRQ();

    USB_OTG_GlobalTypeDef* m_Port = nullptr;
    USB_OTG_DeviceTypeDef* m_Device = nullptr;

    USB_OTG_OUTEndpointTypeDef* m_OutEndpoints = nullptr;
    USB_OTG_INEndpointTypeDef*  m_InEndpoints = nullptr;

    volatile uint32_t* m_PCGCCTL = nullptr;
    volatile uint32_t* m_FIFOBase = nullptr;

    struct EndpointTransferState
    {
        void Reset()
        {
            Buffer = nullptr;
            TotalLength = 0;
            EndpointMaxSize = 0;
            Interval = 0;
        }

        uint8_t*    Buffer;
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
