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
// Created: 13.06.2022 23:00


#pragma once

#include <stdint.h>

#include <Threads/Thread.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Signals/SignalTarget.h>
#include <Utils/CircularBuffer.h>

#include <Kernel/USB/USBEndpointState.h>
#include <Kernel/USB/USBProtocol.h>
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
    uint8_t         m_SelectedConfiguration = 0;
    bool            m_SupportRemoteWakeup   = false;
    bool            m_SelfPowered           = false;
    USB_Speed       m_Speed                 = USB_Speed::FULL;

    os::String      m_ManufacturerString;
    os::String      m_ProductString;
    os::String      m_SerialNumberString;
    USB_DescDevice  m_DeviceDesc;
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


class USBHost : public os::Thread, public SignalTarget
{
public:
    USBHost();
    ~USBHost();

    // From Thread:
    IFLASHC virtual void* Run() override;

    IFLASHC bool Setup(USBDriver* driver);
    IFLASHC void Shutdown();

    IFLASHC USBHostControl& GetControlHandler();
    IFLASHC USBDeviceNode*  CreateDeviceNode();
    IFLASHC USBDeviceNode*  GetDevice(uint8_t deviceAddr);

    IFLASHC void            RestartDeviceInitialization();

    IFLASHC bool            AddClassDriver(Ptr<USBClassDriverHost> pclass);
    IFLASHC bool            ReEnumerate();

    IFLASHC bool            IsPortEnabled();
    IFLASHC bool            OpenPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize);
    IFLASHC bool            ClosePipe(USB_PipeIndex pipeIndex);
    IFLASHC USB_URBState    GetURBState(USB_PipeIndex pipeIndex);
    IFLASHC bool            SetDataToggle(USB_PipeIndex pipeIndex, bool toggle);
    IFLASHC bool            GetDataToggle(USB_PipeIndex pipeIndex);
    IFLASHC bool            SubmitURB(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType enpointType, USBH_InitialTransactionPID initialPID, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback);
    IFLASHC bool            ControlSendSetup(USB_PipeIndex pipeIndex, USB_ControlRequest* request, USB_TransactionCallback&& callback);
    IFLASHC bool            ControlSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback);
    IFLASHC bool            ControlReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    IFLASHC bool            BulkSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback);
    IFLASHC bool            BulkReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    IFLASHC bool            InterruptReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    IFLASHC bool            InterruptSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    IFLASHC bool            IsochronousReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    IFLASHC bool            IsochronousSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback);
    IFLASHC USB_PipeIndex   AllocPipe(uint8_t endpointAddr);
    IFLASHC void            FreePipe(USB_PipeIndex pipeIndex);

    IFLASHC bool ConfigureDevice(const USB_DescConfiguration* configDesc, uint8_t deviceAddr);

    KMutex& GetMutex() { return m_Mutex; }

    Signal<void, bool>              SignalConnectionChanged;
    VFConnector<uint8_t, USBHost*>  VFSelectConfiguration;

private:
    static constexpr float DEVICE_RESET_TIMEOUT = 1.0f;

    IFLASHC bool                PushEvent(USBHostEventID eventID);
    IFLASHC bool                PushEvent(const USBHostEvent& event);
    IFLASHC bool                PopEvent(USBHostEvent& event);

    IFLASHC void                Reset();
    IFLASHC bool                Stop();

    IFLASHC USBHostPipeData*    GetPipeData(USB_PipeIndex pipeIndex);

    IFLASHC void SetupClassDrivers();

    IFLASHC void HandleEnumerationDone(bool result, uint8_t deviceAddr);
    IFLASHC void HandleSetConfigurationResult(bool result, uint8_t deviceAddr);
    IFLASHC void HandleSetWakeupFeatureResult(bool result, uint8_t deviceAddr);
    IFLASHC void HandleURBStateChanged(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transferLength);

    IFLASHC bool IRQDeviceConnected();
    IFLASHC void IRQDeviceDisconnected();
    IFLASHC void IRQPortEnableChange(bool isEnabled);
    IFLASHC void IRQStartOfFrame();
    IFLASHC void IRQPipeURBStateChanged(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t length);

    USBHostControl                          m_ControlHandler;
    USBHostEnumerator                       m_Enumerator;
    USBDriver*                              m_Driver = nullptr;

    KMutex                                  m_Mutex;
    KConditionVariable                      m_EventQueueCondition;
    CircularBuffer<USBHostEvent, 256>       m_EventQueue;

    USBDeviceNode                           m_Device0;
    std::vector<USBDeviceNode>              m_Devices;

    std::vector<Ptr<USBClassDriverHost>>    m_ClassDrivers;
    std::vector<USBHostPipeData>            m_Pipes;
    TimeValNanos                            m_DeviceAttachDeadline  = TimeValNanos::infinit;
    uint8_t                                 m_ResetErrorCount       = 0;
    uint8_t                                 m_EnumErrorCount        = 0;
    bool                                    m_PortEnabled           = false;
};


} // namespace kernel
