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
#include <Kernel/HAL/STM32/USBDevice_STM32.h>
#include <Kernel/HAL/STM32/USBHost_STM32.h>

namespace kernel
{
enum class USB_OTG_ID : int;

static constexpr uint32_t USB_PKTSTS_NAK            = 1; // Global OUT NAK (triggers an interrupt).
static constexpr uint32_t USB_PKTSTS_OUT_DATA_RCV   = 2; // OUT data packet received.
static constexpr uint32_t USB_PKTSTS_OUT_XFR_DONE   = 3; // OUT transfer completed (triggers an interrupt).
static constexpr uint32_t USB_PKTSTS_SETUP_XFR_DONE = 4; // SETUP transaction completed (triggers an interrupt).
static constexpr uint32_t USB_PKTSTS_SETUP_DATA_RCV = 6; // SETUP data packet received.

static constexpr uint32_t USB_PKTSTS_HOST_IN_DATA_RCV       = 2; // IN data packet received
static constexpr uint32_t USB_PKTSTS_HOST_IN_XFR_DONE       = 3; // IN transfer completed(triggers an interrupt)
static constexpr uint32_t USB_PKTSTS_HOST_DATA_TOGGLE_ERR   = 5; // Data toggle error(triggers an interrupt)
static constexpr uint32_t USB_PKTSTS_HOST_CHANNEL_HALTED    = 7; // Channel halted(triggers an interrupt)


enum class USB_Mode : uint8_t
{
    Device,
    Host,
    Dual
};

enum class USB_OTG_Phy : uint8_t
{
    ULPI,
    Embedded
};

class USB_STM32 : public USBDriver
{
public:
    USB_STM32();
    ~USB_STM32();

    bool Setup(USB_OTG_ID portID, USB_Mode mode, USB_Speed speed, USB_OTG_Phy phyInterface, bool enableDMA, bool useExternalVBus, bool batteryChargingEnabled, const PinMuxTarget& pinDM, const PinMuxTarget& pinDP, const PinMuxTarget& pinID, DigitalPinID pinVBus, bool useSOF = false);
    void Shutdown();

    USB_OTG_Phy         GetPhyInterface() const { return m_PhyInterface; }
    bool                UseDMA() const { return m_UseDMA; }
    USB_Speed           GetConfigSpeed() const { return m_ConfigSpeed; }

    bool        SetUSBMode(USB_Mode mode);
    USB_Mode    GetUSBMode() const;

    // Device interface:
    virtual USB_Speed   HostGetSpeed() const override                                       { return m_HostDriver.HostGetSpeed(); }
    virtual USB_Speed   DeviceGetSpeed() const override                                     { return m_DeviceDriver.DeviceGetSpeed(); }
    virtual void        EndpointStall(uint8_t endpointAddr) override                        { m_DeviceDriver.EndpointStall(endpointAddr); }
    virtual void        EndpointClearStall(uint8_t endpointAddr) override                   { m_DeviceDriver.EndpointClearStall(endpointAddr);  }
    virtual bool        EndpointOpen(const USB_DescEndpoint& endpointDescriptor) override   { return m_DeviceDriver.EndpointOpen(endpointDescriptor); }
    virtual void        EndpointClose(uint8_t endpointAddr) override                        { m_DeviceDriver.EndpointClose(endpointAddr); }
    virtual void        EndpointCloseAll() override                                         { m_DeviceDriver.EndpointCloseAll(); }
    virtual bool        EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength) override { return m_DeviceDriver.EndpointTransfer(endpointAddr, buffer, totalLength); }
    virtual bool        SetAddress(uint8_t deviceAddr) override                             { return m_DeviceDriver.SetAddress(deviceAddr); }

    // Host interface:
    virtual uint32_t    GetMaxPipeCount() const override { return m_HostDriver.GetMaxPipeCount(); }
    virtual bool        StartHost() override { return m_HostDriver.StartHost(); }
    virtual bool        StopHost() override { return m_HostDriver.StopHost(); }
    virtual bool        ResetPort() override { return m_HostDriver.ResetPort(); }
    virtual uint32_t    GetCurrentHostFrame() override { return m_HostDriver.GetCurrentFrame(); }

    virtual bool        SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize) override { return m_HostDriver.SetupPipe(pipeIndex, endpointAddr, deviceAddr, speed, endpointType, maxPacketSize); }
    virtual bool        HaltChannel(USB_PipeIndex pipeIndex) override { return m_HostDriver.HaltChannel(pipeIndex); }
    virtual bool        HostSubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, void* buffer, size_t length, bool doPing) override { return m_HostDriver.SubmitRequest(pipeIndex, direction, endpointType, initialPID, buffer, length, doPing); }
    virtual bool        SetDataToggle(USB_PipeIndex pipeIndex, bool toggle) override { return m_HostDriver.SetDataToggle(pipeIndex, toggle); }
    virtual bool        GetDataToggle(USB_PipeIndex pipeIndex) const override { return m_HostDriver.GetDataToggle(pipeIndex); }

    void        ReadFromFIFO(void* buffer, size_t length);
    void        WriteToFIFO(uint32_t fifoIndex, const void* buffer, size_t length);

    bool        FlushTxFifo(uint32_t count);
    bool        FlushRxFifo();

    virtual void EnableIRQ(bool enable) override;
private:
    bool                SetupCore(bool useExternalVBus, bool batteryChargingEnabled);
    bool                CoreReset();
    bool                WaitForAHBIdle();
    volatile uint32_t*  GetFIFOBase(uint32_t endpoint);


    USB_OTG_GlobalTypeDef*  m_Port = nullptr;

    volatile uint32_t*      m_FIFOBase = nullptr;

    USBDevice_STM32         m_DeviceDriver;
    USBHost_STM32           m_HostDriver;

    USB_OTG_Phy             m_PhyInterface = USB_OTG_Phy::Embedded;
    bool                    m_UseDMA = false;
    USB_Speed               m_ConfigSpeed = USB_Speed::FULL;
};


} // namespace kernel
