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
// Created: 27.05.2022 18:00

#pragma once

#include <atomic>
#include <map>
#include <vector>
#include <Kernel/KThread.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Signals/SignalTarget.h>
#include <Signals/Signal.h>
#include <Signals/VFConnector.h>
#include <Utils/CircularBuffer.h>

#include <Kernel/USB/USBClientControl.h>
#include <Kernel/USB/USBEndpointState.h>
#include <Kernel/USB/USBLanguages.h>

namespace kernel
{
class USBDriver;

enum class USBDeviceEventID : int
{
    None,
    BusReset,
    SessionEnded,
    RecoveryNeeded,
    StartOfFrame,
    Suspend,
    Resume,
    ControlRequestReceived,
    TransferComplete
};

struct USBDeviceEvent
{
    USBDeviceEvent(USBDeviceEventID eventID = USBDeviceEventID::None) : EventID(eventID) {}

    USBDeviceEventID EventID;

    union
    {
        struct
        {
            USB_Speed Speed;
            uint32_t Generation;
        } BusReset;

        struct
        {
            USB_ControlRequest Request;
        } ControlRequestReceived;
        
        struct
        {
            uint8_t             EndpointAddr;
            USB_TransferResult  Result;
            uint32_t            Length;
        } TransferComplete;
    };
};

class USBDevice : public KThread, public SignalTarget
{
public:
    USBDevice();
    virtual ~USBDevice();

    // From Thread:
    virtual void* Run() override;

    KMutex& GetMutex() { return m_Mutex; }

    bool Setup(USBDriver* driver, uint32_t endpoint0Size, int threadPriority);
    void AddClassDriver(Ptr<USBClassDriverDevice> driver);
    void RemoveClassDriver(Ptr<USBClassDriverDevice> driver);

    void SetDeviceDescriptor(const USB_DescDevice& descriptor);
    void SetDeviceQualifier(const USB_DescDeviceQualifier& qualifier);
    void AddConfigDescriptor(uint32_t index, const void* data, size_t length);
    void AddOtherConfigDescriptor(uint32_t index, const void* data, size_t length);
    void AddBOSDescriptor(const void* data, size_t length);
    void SetStringDescriptor(USB_LanguageID languageID, uint32_t index, const PString& string);

    void ClearConfigDescriptors()       { m_ConfigDescriptors.clear(); }
    void ClearOtherConfigDescriptors()  { m_OtherConfigDescriptors.clear(); }
    void ClearBOSDescriptors()          { m_BOSDescriptors.clear(); }
    void ClearStringDescriptors()       { m_StringDescriptors.clear(); }

    const USB_DescConfiguration* GetConfigDescriptor(uint32_t index) const;
    const USB_DescConfiguration* GetOtherConfigDescriptor(uint32_t index) const;
    const USB_DescBOS*           GetBOSDescriptor() const;

    Ptr<USBClassDriverDevice> GetInterfaceDriver(uint8_t interfaceNum);
    Ptr<USBClassDriverDevice> GetEndpointDriver(uint8_t endpointAddress);

    USBEndpointState&       GetEndpoint(uint8_t endpointAddr);
    const USBEndpointState& GetEndpoint(uint8_t endpointAddr) const { return const_cast<USBDevice*>(this)->GetEndpoint(endpointAddr); }

    USBClientControl& GetControlEndpointHandler() { return m_ControlTransfer; }

    bool IsReady() const     { return IsMounted() && !IsSuspended(); }
    bool IsConnected() const { return m_IsConnected; }
    bool IsMounted() const   { return m_SelectedConfigNum != 0; }
    bool IsSuspended() const { return m_IsSuspended; }

    bool OpenEndpoint(const USB_DescEndpoint& endpointDescriptor);
    const USB_DescriptorHeader* OpenEndpointPair(const USB_DescriptorHeader* desc, USB_TransferType transferType, uint8_t& endpointOut, uint8_t& endpointIn, uint16_t& endpointOutMaxSize, uint16_t& endpointInMaxSize);
    void CloseEndpoint(uint8_t endpointAddr);
    bool ClaimEndpoint(uint8_t endpointAddr);
    bool ReleaseEndpoint(uint8_t endpointAddr);
    bool IsEndpointBusy(uint8_t endpointAddr);
    void EndpointSetStall(uint8_t endpointAddr);
    bool EndpointClearStall(uint8_t endpointAddr);
    bool IsEndpointStalled(uint8_t endpointAddr);
    // receiveCapacity follows USB_TransferSegment::ReceiveCapacity; ignored for transmit.
    bool EndpointTransfer(uint8_t endpointAddr, uint8_t* buffer, size_t length, size_t receiveCapacity = 0);

    Signal<void, bool/*isConnected*/>           SignalConnected;
    Signal<void, bool/*isMounted*/>             SignalMounted;
    Signal<void, bool/*remoteWakeupEnabled*/>   SignalSuspend;
    Signal<void>                                SignalResume;

    VFConnector<bool, USB_ControlStage, const USB_ControlRequest&> SignalHandleVendorControlTransfer;
private:
    friend class USBClientCDCRecoveryTest;

    static size_t GetEndpointIndex(uint8_t endpointAddr);

    void SetIsConnected(bool connected);
    void SetIsSuspended(bool suspended);

    void BusReset();

    void UnsetConfiguration();
    
    bool HandleControlRequest(const USB_ControlRequest& request);
    bool HandleDeviceControlRequests(const USB_ControlRequest& request);
    bool HandleInterfaceControlRequest(const USB_ControlRequest& request);
    bool HandleEndpointControlRequest(const USB_ControlRequest& request);

    bool HandleSelectConfiguration(uint8_t configNum);
    bool HandleGetDescriptor(const USB_ControlRequest& request);
    bool InvokeClassDriverControlTransfer(Ptr<USBClassDriverDevice> driver, const USB_ControlRequest& request);

    bool PopEvent_pl(USBDeviceEvent& event);
    void DiscardTransferEvents_pl(uint32_t endpointMask, USBDeviceEvent* discardedCompletion = nullptr);
    void PushEvent(const USBDeviceEvent& event, bool clearQueue = false);

    void IRQControlRequestReceived(const USB_ControlRequest& request);
    void IRQTransferComplete(uint8_t endpointAddr, uint32_t length, USB_TransferResult result);

    void IRQBusResetStarted();
    void IRQBusReset(USB_Speed speed, uint32_t generation);
    void IRQSuspend();
    void IRQResume();
    void IRQSessionEnded();
    void IRQDeviceRecoveryNeeded();
    void IRQStartOfFrame();

    KMutex              m_Mutex;
    KConditionVariable  m_EventQueueCondition;

    USBDriver*          m_Driver = nullptr;

    PCircularBuffer<USBDeviceEvent, 128>      m_EventQueue;
    // Reset can invalidate the dequeued event before the device thread dispatches it.
    std::atomic<bool>                        m_DispatchEventValid = false;
    std::vector<Ptr<USBClassDriverDevice>>        m_ClassDrivers;
    std::map<uint8_t, Ptr<USBClassDriverDevice>>  m_InterfaceToDriverMap;
    std::map<uint8_t, Ptr<USBClassDriverDevice>>  m_EndpointToDriverMap;

    USB_DescDevice                                      m_DeviceDescriptor;
    USB_DescDeviceQualifier                             m_DeviceQualifier;
    mutable std::map<uint32_t, std::vector<uint8_t>>    m_ConfigDescriptors;
    mutable std::map<uint32_t, std::vector<uint8_t>>    m_OtherConfigDescriptors;
    mutable std::vector<uint8_t>                        m_BOSDescriptors;

    std::map<USB_LanguageID, std::map<uint32_t, std::vector<uint8_t>>>    m_StringDescriptors;
    USB_LanguageID      m_DefaultLanguage = USB_LanguageID::ENGLISH_UNITED_STATES;
    uint32_t            m_Endpoint0Size = 0;

    uint8_t    m_SelectedConfigNum = 0; // Currently active configuration. Set to 0 if not configured.
    bool       m_IsConnected = false;
    bool       m_IsAddressed = false;
    bool       m_IsSuspended = false;

    bool        m_RemoteWakeupEnabled = false;
    bool        m_RemoteWakeupSupport = false;
    bool        m_SelfPowered         = false;
    USB_Speed   m_SelectedSpeed       = USB_Speed::LOW;

    USBClientControl    m_ControlTransfer;
    USBEndpointState    m_EndpointStates[USB_ADDRESS_MAX_EP_COUNT * 2];
};

} // namespace kernel
