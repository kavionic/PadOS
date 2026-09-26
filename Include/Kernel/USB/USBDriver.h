// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 26.05.2022 13:00

#pragma once

#include <stdint.h>
#include <functional>
#include <System/Platform.h>
#include <Signals/SignalUnguarded.h>
#include <Kernel/USB/USBCommon.h>

class PString;
struct USB_ControlRequest;
struct USB_DescEndpoint;

enum class USB_Speed : uint8_t;
enum class USB_TransferType : uint8_t;
enum class USB_RequestDirection : uint8_t;
enum class USB_URBState : uint8_t;


namespace kernel
{
enum class USBH_InitialTransactionPID : uint8_t;

class USBIRQDisabler;

enum class USB_TransferResult : uint8_t
{
    Invalid,
    Success,
    Failed,
    Stalled,
    Timeout
};

class USBDriver
{
public:
    // Device interface:
    virtual USB_Speed   DeviceGetSpeed() const = 0;
    virtual void        EndpointStall(uint8_t endpointAddr) = 0;
    // Clear halt and reset DATA0, including on an unhalted bulk/interrupt endpoint.
    // Preserve pending transfers and their completion events. Hardware failure must block submissions until recovery.
    // Returns false while reset or controller recovery prevents the operation.
    virtual bool        EndpointClearStall(uint8_t endpointAddr) = 0;
    virtual bool        EndpointOpen(const USB_DescEndpoint& endpointDescriptor) = 0;
    virtual void        EndpointClose(uint8_t endpointAddr) = 0;
    virtual void        EndpointCloseAll() = 0;
    // receiveCapacity follows USB_TransferSegment::ReceiveCapacity; ignored for transmit.
    virtual bool        EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength, size_t receiveCapacity = 0) = 0;
    virtual bool        SetAddress(uint8_t deviceAddr) = 0;  // Return 'true' if a response needs to be sent.

    // Called after the device thread has cleaned up the matching BusReset event.
    virtual bool        CompleteDeviceReset(uint32_t generation) = 0;
    // Called by the device thread after IRQDeviceRecoveryNeeded, once old class instances have been closed.
    virtual bool        RecoverDevice() = 0;

    // Host interface:
#ifdef PADOS_MODULE_USB_HOST
    virtual USB_Speed   HostGetSpeed() const = 0;
    virtual bool        HostSupportsLowSpeedHubDevices() const { return true; }
    virtual uint32_t    GetMaxPipeCount() const = 0;
    virtual bool        StartHost() = 0;
    virtual bool        StopHost() = 0;
    virtual bool        SetPortReset(bool resetActive) = 0;
    virtual uint32_t    GetCurrentHostFrame() = 0;
    virtual bool        SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize) = 0;
    virtual bool        HaltChannel(USB_PipeIndex pipeIndex) = 0;
    virtual bool        HostSubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, const USB_TransferSegment* segments, size_t segmentCount, size_t length) = 0;
    virtual bool        SetDataToggle(USB_PipeIndex pipeIndex, bool toggle) = 0;
    virtual bool        GetDataToggle(USB_PipeIndex pipeIndex) const = 0;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    virtual size_t      GetHostPipeDebugEntryCount(USB_PipeIndex) const { return 0; }
    virtual bool        GetHostPipeDebugEntryLabel(USB_PipeIndex, size_t, PString*) const { return false; }
    virtual bool        GetHostPipeDebugEntryValue(USB_PipeIndex, size_t, PString*) const { return false; }
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
#endif


    virtual void        EnableIRQ(bool enable) = 0;

    SignalUnguarded<void>                                                                   IRQSuspend;
    SignalUnguarded<void>                                                                   IRQResume;
    SignalUnguarded<void>                                                                   IRQDebounceDone;
    SignalUnguarded<void>                                                                   IRQSessionEnded;
    SignalUnguarded<void>                                                                   IRQDeviceRecoveryNeeded;
    SignalUnguarded<void>                                                                   IRQDeviceDisconnected;  // Host & device mode.
    SignalUnguarded<void>                                                                   IRQStartOfFrame;        // Host & device mode.
    SignalUnguarded<void>                                                                   IRQBusResetStarted;
    SignalUnguarded<void (USB_Speed speed, uint32_t generation)>                            IRQBusReset;
    SignalUnguarded<void (const USB_ControlRequest& request)>                               IRQControlRequestReceived;
    SignalUnguarded<void(uint8_t endpointAddr, uint32_t length, USB_TransferResult result)> IRQTransferComplete;
    SignalUnguarded<void>                                                                   IRQIncompleteIsochronousINTransfer;

#ifdef PADOS_MODULE_USB_HOST
    SignalUnguarded<void>                                                                   IRQDeviceConnected;     // Host mode only.
    SignalUnguarded<void (bool isEnabled)>                                                  IRQPortEnableChange;
    SignalUnguarded<void (USB_PipeIndex pipeIndex, USB_URBState urbState, size_t length)>   IRQPipeURBStateChanged;
#endif

private:
    friend class USBIRQDisabler;

    virtual bool DisableIRQDelivery() = 0;
    virtual void RestoreIRQDelivery(bool wasEnabled) = 0;
};

class USBIRQDisabler
{
public:
    explicit USBIRQDisabler(USBDriver& driver) : m_Driver(driver), m_WasEnabled(driver.DisableIRQDelivery()) {}
    ~USBIRQDisabler() { m_Driver.RestoreIRQDelivery(m_WasEnabled); }

    USBIRQDisabler(const USBIRQDisabler&) = delete;
    USBIRQDisabler& operator=(const USBIRQDisabler&) = delete;

private:
    USBDriver& m_Driver;
    bool       m_WasEnabled;
};

} // namespace kernel
