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
// Created: 22.05.2022 17:00

#pragma once

#include <stdint.h>
#include <System/Sections.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/USB/USBDriver.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/HAL/STM32/USBDevice_STM32.h>
#ifdef PADOS_MODULE_USB_HOST
#include <Kernel/HAL/STM32/USBHost_STM32.h>
#endif

enum class USB_OTG_ID : int;

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

namespace kernel
{

class USB_STM32 : public USBDriver
{
public:
    USB_STM32();
    ~USB_STM32();

    // Internal DMA uses the final 18 FIFO words for endpoint information.
    static constexpr uint32_t DMA_FIFO_USABLE_WORD_COUNT = USB_OTG_FIFO_SIZE / sizeof(uint32_t) - 18;

    // Checks USB DMA memory accessibility.
    static bool IsDMABufferAccessible(const void* buffer, size_t length);

    // Transmit (host OUT/device IN) reads memory; receive (host IN/device OUT) writes memory.
    // Transmit payload bytes must remain stable until DMA completes; neighboring bytes may be accessed by the CPU.
    static bool IsDirectDMATransmitBuffer(const void* buffer, size_t length);
    // Receive buffers and bounce allocations must own complete cache lines.
    static bool IsDirectDMAReceiveBuffer(const void* buffer, size_t length);
    // length is bounded by the current caller segment and hardware transfer limits; packetSize must be nonzero.
    static size_t GetDirectDMAReceiveLength(const void* buffer, size_t length, size_t packetSize);
    static void CleanDMATransmitBuffer(const void* buffer, size_t length);

    bool Setup(USB_OTG_ID portID, USB_Mode mode, USB_Speed speed, USB_OTG_Phy phyInterface, bool useExternalVBus, bool batteryChargingEnabled, const PinMuxTarget& pinDM, const PinMuxTarget& pinDP, const PinMuxTarget& pinID, DigitalPinID pinVBus, bool useSOF = false);
    void Shutdown();
    bool ResetHostCore();
    void HoldCoreInReset();
    bool ResetDeviceCore();

    USB_OTG_Phy         GetPhyInterface() const { return m_PhyInterface; }
    USB_Speed           GetConfigSpeed() const { return m_ConfigSpeed; }

    bool        SetUSBMode(USB_Mode mode);
    USB_Mode    GetUSBMode() const;

    // Device interface:
    virtual USB_Speed   DeviceGetSpeed() const override                                     { return m_DeviceDriver.DeviceGetSpeed(); }
    virtual void        EndpointStall(uint8_t endpointAddr) override                        { m_DeviceDriver.EndpointStall(endpointAddr); }
    virtual void        EndpointClearStall(uint8_t endpointAddr) override                   { m_DeviceDriver.EndpointClearStall(endpointAddr);  }
    virtual bool        EndpointOpen(const USB_DescEndpoint& endpointDescriptor) override   { return m_DeviceDriver.EndpointOpen(endpointDescriptor); }
    virtual void        EndpointClose(uint8_t endpointAddr) override                        { m_DeviceDriver.EndpointClose(endpointAddr); }
    virtual void        EndpointCloseAll() override                                         { m_DeviceDriver.EndpointCloseAll(); }
    virtual bool        EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength) override { return m_DeviceDriver.EndpointTransfer(endpointAddr, buffer, totalLength); }
    virtual bool        SetAddress(uint8_t deviceAddr) override                             { return m_DeviceDriver.SetAddress(deviceAddr); }

    virtual bool        CompleteDeviceReset(uint32_t generation) override { return m_DeviceDriver.CompleteDeviceReset(generation); }
    virtual bool        RecoverDevice() override { return m_DeviceDriver.Recover(); }

    // Host interface:
#ifdef PADOS_MODULE_USB_HOST
    virtual USB_Speed   HostGetSpeed() const override { return m_HostDriver.HostGetSpeed(); }
    virtual bool        HostSupportsLowSpeedHubDevices() const override { return false; }
    virtual uint32_t    GetMaxPipeCount() const override { return m_HostDriver.GetMaxPipeCount(); }
    virtual bool        StartHost() override { return m_HostDriver.StartHost(); }
    virtual bool        StopHost() override { return m_HostDriver.StopHost(); }
    virtual bool        SetPortReset(bool resetActive) override { return m_HostDriver.SetPortReset(resetActive); }
    virtual uint32_t    GetCurrentHostFrame() override { return m_HostDriver.GetCurrentFrame(); }

    virtual bool        SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize) override { return m_HostDriver.SetupPipe(pipeIndex, endpointAddr, deviceAddr, speed, endpointType, maxPacketSize); }
    virtual bool        HaltChannel(USB_PipeIndex pipeIndex) override { return m_HostDriver.HaltChannel(pipeIndex); }
    virtual bool        HostSubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, const USB_TransferSegment* segments, size_t segmentCount, size_t length) override { return m_HostDriver.SubmitRequest(pipeIndex, direction, endpointType, initialPID, segments, segmentCount, length); }
    virtual bool        SetDataToggle(USB_PipeIndex pipeIndex, bool toggle) override { return m_HostDriver.SetDataToggle(pipeIndex, toggle); }
    virtual bool        GetDataToggle(USB_PipeIndex pipeIndex) const override { return m_HostDriver.GetDataToggle(pipeIndex); }
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    virtual size_t      GetHostPipeDebugEntryCount(USB_PipeIndex pipeIndex) const override { return m_HostDriver.GetPipeDebugEntryCount(pipeIndex); }
    virtual bool        GetHostPipeDebugEntryLabel(USB_PipeIndex pipeIndex, size_t entryIndex, PString* outLabel) const override { return m_HostDriver.GetPipeDebugEntryLabel(pipeIndex, entryIndex, outLabel); }
    virtual bool        GetHostPipeDebugEntryValue(USB_PipeIndex pipeIndex, size_t entryIndex, PString* outValue) const override { return m_HostDriver.GetPipeDebugEntryValue(pipeIndex, entryIndex, outValue); }
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
#endif

    bool        FlushTxFifo(uint32_t count);
    bool        FlushRxFifo();

    virtual void EnableIRQ(bool enable) override;

private:
    virtual bool DisableIRQDelivery() override;
    virtual void RestoreIRQDelivery(bool wasEnabled) override;

    void                ReleaseCoreReset();
    bool                SetupCore(bool useExternalVBus, bool batteryChargingEnabled);
    bool                CoreReset();
    bool                WaitForAHBIdle();


    USB_OTG_GlobalTypeDef*  m_Port = nullptr;

    USBDevice_STM32         m_DeviceDriver;
#ifdef PADOS_MODULE_USB_HOST
    USBHost_STM32           m_HostDriver;
#endif

    USB_OTG_Phy             m_PhyInterface = USB_OTG_Phy::Embedded;
    IRQn_Type               m_IRQ{};
    USB_Speed               m_ConfigSpeed = USB_Speed::FULL;
    bool                    m_UseExternalVBus = false;
    bool                    m_BatteryChargingEnabled = false;
};


} // namespace kernel
