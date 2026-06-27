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
// Created: 13.06.2022 23:00


#pragma once

#include <deque>
#include <stdint.h>
#include <vector>

#include <Kernel/KThread.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Signals/SignalTarget.h>
#include <Utils/CircularBuffer.h>

#include <Kernel/USB/USBEndpointState.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/USB/USBProtocolHub.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBHostControl.h>
#include <Kernel/USB/USBHostEnumerator.h>

namespace kernel
{
class USBDriver;
class USBClassDriverHost;

using USB_TransactionCallback = std::function<void(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)>;

enum class USB_URBState : uint8_t
{
    Idle,
    Done,
    NotReady,
    Stall,
    Error
};

enum class USBH_InitialTransactionPID : uint8_t
{
    Setup,
    Data
};

enum class USBHostEventID : uint8_t
{
    None,
    DeviceConnected,
    DeviceAttached,
    DeviceDetached,
    DeviceDisconnected,
    ReEnumerate,
    URBStateChanged
};

struct USBHostPipeData
{
    USBHostPipeData(uint8_t endpointAddr = 0, bool allocated = false) noexcept : EndpointAddr(endpointAddr), Claimed(allocated) {}

    USB_TransactionCallback TransactionCallback;
    uint8_t                 EndpointAddr = 0;
    USB_URBState            URBState = USB_URBState::Idle;
    bool                    Claimed = false;
};

class USBDeviceNode
{
public:
    uint8_t         m_Address               = 0;
    uint8_t         m_ParentHubAddress      = 0;
    uint8_t         m_ParentHubPort         = 0;
    uint8_t         m_SelectedConfiguration = 0;
    bool            m_SupportRemoteWakeup   = false;
    bool            m_SelfPowered           = false;
    bool            m_IsConnected           = false;
    bool            m_IsConfigured          = false;
    bool            m_IsHub                 = false;
    USB_Speed       m_Speed                 = USB_Speed::FULL;

    PString         m_ManufacturerString;
    PString         m_ProductString;
    PString         m_SerialNumberString;
    USB_DescDevice  m_DeviceDesc;

    USB_PipeIndex   m_HubStatusPipe         = USB_INVALID_PIPE;
    uint8_t         m_HubStatusEndpoint     = USB_INVALID_ENDPOINT;
    size_t          m_HubStatusEndpointSize = 0;
    uint8_t         m_HubPortCount          = 0;
    uint16_t        m_HubPowerOnDelayMS     = 0;
    std::vector<uint8_t> m_HubStatusBuffer;
};

struct USBHostHubPortEvent
{
    USBHostHubPortEvent(uint8_t hubAddress = 0, uint8_t portIndex = 0) noexcept : HubAddress(hubAddress), PortIndex(portIndex) {}

    uint8_t HubAddress;
    uint8_t PortIndex;
};

struct USBHostEvent
{
    USBHostEvent(USBHostEventID eventID = USBHostEventID::None) : EventID(eventID) {}

    USBHostEventID EventID;

    union
    {
        struct
        {
            USB_PipeIndex   PipeIndex;
            USB_URBState    URBState;
            size_t          TransferLength;
        } URBStateChanged;
    };
};


class USBHost : public KThread, public SignalTarget
{
public:
    USBHost();
    ~USBHost();

    // From Thread:
    virtual void* Run() override;

    bool Setup(USBDriver* driver);
    void Shutdown();

    USBHostControl& GetControlHandler();
    USBDeviceNode*  CreateDeviceNode();
    USBDeviceNode*  GetDevice(uint8_t deviceAddr);

    void            RestartDeviceInitialization();

    bool            AddClassDriver(Ptr<USBClassDriverHost> pclass);
    bool            ReEnumerate();

    bool            IsPortEnabled();
    bool            OpenPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize);
    bool            ClosePipe(USB_PipeIndex pipeIndex);
    USB_URBState    GetURBState(USB_PipeIndex pipeIndex);
    bool            SetDataToggle(USB_PipeIndex pipeIndex, bool toggle);
    bool            GetDataToggle(USB_PipeIndex pipeIndex);
    bool            SubmitURB(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType enpointType, USBH_InitialTransactionPID initialPID, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback);
    bool            ControlSendSetup(USB_PipeIndex pipeIndex, USB_ControlRequest* request, USB_TransactionCallback&& callback);
    bool            ControlSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback);
    bool            ControlReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    bool            BulkSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback);
    bool            BulkReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    bool            InterruptReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    bool            InterruptSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    bool            IsochronousReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    bool            IsochronousSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    USB_PipeIndex   AllocPipe(uint8_t endpointAddr);
    void            FreePipe(USB_PipeIndex pipeIndex);

    bool ConfigureDevice(const USB_DescConfiguration* configDesc, uint8_t deviceAddr);

    KMutex& GetMutex() { return m_Mutex; }

    Signal<void, bool>              SignalConnectionChanged;
    VFConnector<uint8_t, USBHost*>  VFSelectConfiguration;

private:
    static constexpr float DEVICE_RESET_TIMEOUT = 1.0f;

    bool                PushEvent(USBHostEventID eventID, bool clearQueue = false);
    bool                PushEvent(const USBHostEvent& event, bool clearQueue = false);
    bool                PopEvent(USBHostEvent& event);

    void                Reset();
    bool                Stop();
    bool                CloseActiveClassDrivers();
    void                HandleDeviceDisconnected();
    void                PrepareDevice0(USB_Speed speed, uint8_t parentHubAddress, uint8_t parentHubPort);
    void                CloseDevice(uint8_t deviceAddr);
    void                CloseDeviceClassDrivers(uint8_t deviceAddr);
    USBDeviceNode*      GetDeviceOnHubPort(uint8_t hubAddress, uint8_t portIndex);

    USBHostPipeData*    GetPipeData(USB_PipeIndex pipeIndex);

    void SetupClassDrivers(uint8_t deviceAddr);

    void HandleEnumerationDone(bool result, uint8_t deviceAddr);
    void HandleSetConfigurationResult(bool result, uint8_t deviceAddr);
    void HandleSetWakeupFeatureResult(bool result, uint8_t deviceAddr);
    void FinishDeviceConfiguration(uint8_t deviceAddr);
    void HandleURBStateChanged(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transferLength);

    bool ConfigureHubInterface(uint8_t deviceAddr, const USB_DescInterface* interfaceDesc, const void* endDesc, const USB_DescriptorHeader** nextDesc);
    void InitializeHub(uint8_t deviceAddr);
    void HandleGetHubDescriptorResult(bool result, uint8_t deviceAddr);
    void PowerHubPort(uint8_t deviceAddr, uint8_t portIndex);
    void StartHubInterruptReceive(uint8_t deviceAddr);
    void StopHubInterruptReceive(USBDeviceNode& hub);
    void HandleHubStatusTransaction(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    void QueueHubPortChange(uint8_t hubAddress, uint8_t portIndex);
    void ProcessNextHubPortChange();
    void CompleteHubPortChange(uint8_t hubAddress);
    void QueueHubPollRestart(uint8_t hubAddress);
    void RestartPendingHubPolls();
    void HandleHubPortStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex);
    void ClearHubPortConnectionChange(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus);
    void ResetHubPort(uint8_t hubAddress, uint8_t portIndex);
    void HandleHubPortResetStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex);
    void HandleHubPortResetChangeCleared(bool result, uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus);
    void EnumerateHubPortDevice(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus);
    USB_Speed GetHubPortDeviceSpeed(uint16_t portStatus) const;

    bool IRQDeviceConnected();
    void IRQDeviceDisconnected();
    void IRQPortEnableChange(bool isEnabled);
    void IRQStartOfFrame();
    void IRQPipeURBStateChanged(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t length);

    USBHostControl                          m_ControlHandler;
    USBHostEnumerator                       m_Enumerator;
    USBDriver*                              m_Driver = nullptr;

    KMutex                                  m_Mutex;
    KConditionVariable                      m_EventQueueCondition;
    PCircularBuffer<USBHostEvent, 256>       m_EventQueue;

    USBDeviceNode                           m_Device0;
    std::vector<USBDeviceNode>              m_Devices;

    std::vector<Ptr<USBClassDriverHost>>    m_ClassDrivers;
    std::vector<USBHostPipeData>            m_Pipes;
    std::deque<USBHostHubPortEvent>         m_PendingHubPortChanges;
    std::vector<uint8_t>                    m_HubPollRestartList;
    TimeValNanos                            m_DeviceAttachDeadline  = TimeValNanos::infinit;
    uint8_t                                 m_ResetErrorCount       = 0;
    uint8_t                                 m_EnumErrorCount        = 0;
    bool                                    m_HubPortChangeActive   = false;
    bool                                    m_PortEnabled           = false;
};


} // namespace kernel
