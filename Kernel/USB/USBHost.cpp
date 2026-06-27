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

#include <algorithm>
#include <string.h>

#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/USBClassDriverHost.h>
#include <Kernel/USB/USBDriver.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/USB/USBCommon.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHost::USBHost() : KThread("usb_host"), m_Mutex("usb_host", PEMutexRecursionMode_RaiseError), m_EventQueueCondition("usbh_event_queue")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHost::~USBHost()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::Setup(USBDriver* driver)
{
    if (m_Driver != nullptr) return true;

    m_Driver = driver;

    m_ControlHandler.Setup(this);
    m_Enumerator.Setup(this);

    m_Driver->IRQStartOfFrame.Connect(this, &USBHost::IRQStartOfFrame);
    m_Driver->IRQDeviceConnected.Connect(this, &USBHost::IRQDeviceConnected);
    m_Driver->IRQDeviceDisconnected.Connect(this, &USBHost::IRQDeviceDisconnected);
    m_Driver->IRQPortEnableChange.Connect(this, &USBHost::IRQPortEnableChange);
    m_Driver->IRQPipeURBStateChanged.Connect(this, &USBHost::IRQPipeURBStateChanged);

    Reset();

    if (!m_Driver->StartHost())
    {
        return false;
    }
    SetDeleteOnExit(false);
    Start_trw(KSpawnThreadFlag::None, PThreadDetachState_Detached);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::Shutdown()
{
    Reset();
    m_Driver->StopHost();
    m_EventQueue.Clear();
    m_Driver = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostControl& USBHost::GetControlHandler()
{
    return m_ControlHandler;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDeviceNode* USBHost::CreateDeviceNode()
{
    for (size_t index = 0; index < m_Devices.size(); ++index)
    {
        if (!m_Devices[index].m_IsConnected)
        {
            m_Devices[index] = m_Device0;
            USBDeviceNode* device = &m_Devices[index];
            device->m_Address = uint8_t(index + 1);
            device->m_IsConnected = true;
            device->m_IsConfigured = false;
            return device;
        }
    }
    if (m_Devices.size() >= 127) {
        return nullptr;
    }
    m_Devices.emplace_back(m_Device0);
    USBDeviceNode* device = &m_Devices.back();
    device->m_Address = uint8_t(m_Devices.size());
    device->m_IsConnected = true;
    device->m_IsConfigured = false;
    return device;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDeviceNode* USBHost::GetDevice(uint8_t deviceAddr)
{
    if (deviceAddr != 0)
    {
        const size_t index = deviceAddr - 1;
        if (index < m_Devices.size() && m_Devices[index].m_IsConnected) {
            return &m_Devices[index];
        } else {
            return nullptr;
        }
    }
    else
    {
        return &m_Device0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* USBHost::Run()
{
    CRITICAL_SCOPE(m_Mutex);
    for (;;)
    {
        USBHostEvent event;
        if (PopEvent(event))
        {
            switch (event.EventID)
            {
                case USBHostEventID::ReEnumerate:
                    if (IsPortEnabled()) {
                        Stop();
                    }
                    RestartDeviceInitialization();
                    break;
                case USBHostEventID::DeviceConnected:
                    if (!m_DeviceAttachDeadline.IsInfinit() && !m_PortEnabled) {
                        break;
                    }
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Device connected.");

                    Stop();
                    CloseActiveClassDrivers();
                    Reset();
                    m_Driver->StartHost();

                    snooze_ms(200);
                    m_Driver->ResetPort();

                    m_Device0.m_Address = 0;
                    m_DeviceAttachDeadline = kget_monotonic_time() + TimeValNanos::FromSeconds(DEVICE_RESET_TIMEOUT);
                    break;
                case USBHostEventID::DeviceAttached:
                    m_DeviceAttachDeadline = TimeValNanos::infinit;
                    m_PortEnabled = true;
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Device reset Completed.");
                    m_ResetErrorCount = 0;

                    SignalConnectionChanged(true);

                    snooze_ms(100);

                    PrepareDevice0(m_Driver->HostGetSpeed(), 0, 0);
                    m_ControlHandler.AllocPipes(0, m_Device0.m_Speed, (m_Device0.m_Speed == USB_Speed::LOW) ? 8 : 64);
                    m_Enumerator.Enumerate(p_bind_method(this, &USBHost::HandleEnumerationDone));
                    break;
                case USBHostEventID::DeviceDetached:
                    [[fallthrough]];
                case USBHostEventID::DeviceDisconnected:
                    HandleDeviceDisconnected();
                    break;
                case USBHostEventID::URBStateChanged:
                    HandleURBStateChanged(event.URBStateChanged.PipeIndex, event.URBStateChanged.URBState, event.URBStateChanged.TransferLength);
                    break;
                case USBHostEventID::None:
                    break;
            }
        }
        else if (!m_DeviceAttachDeadline.IsInfinit() && kget_monotonic_time() > m_DeviceAttachDeadline)
        {
            m_DeviceAttachDeadline = TimeValNanos::infinit;
            if (++m_ResetErrorCount > 3) {
                kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "Device reset failed.");
            } else {
                RestartDeviceInitialization();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::RestartDeviceInitialization()
{
    PushEvent(USBHostEventID::DeviceConnected, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::AddClassDriver(Ptr<USBClassDriverHost> driver)
{
    if (driver == nullptr)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "USBHost::AddClassDriver() called with nullptr.");
        return false;
    }
    m_ClassDrivers.push_back(driver);

    driver->Init(this);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::ReEnumerate()
{
    PushEvent(USBHostEventID::ReEnumerate);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::IsPortEnabled()
{
    return m_PortEnabled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::OpenPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize)
{
    return m_Driver->SetupPipe(pipeIndex, endpointAddr, deviceAddr, speed, endpointType, maxPacketSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::ClosePipe(USB_PipeIndex pipeIndex)
{
    return m_Driver->HaltChannel(pipeIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_URBState USBHost::GetURBState(USB_PipeIndex pipeIndex)
{
    USBHostPipeData* pipe = GetPipeData(pipeIndex);

    if (pipe != nullptr) {
        return pipe->URBState;
    }
    return USB_URBState::Idle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::SetDataToggle(USB_PipeIndex pipeIndex, bool toggle)
{
    return m_Driver->SetDataToggle(pipeIndex, toggle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::GetDataToggle(USB_PipeIndex pipeIndex)
{
    return m_Driver->GetDataToggle(pipeIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::SubmitURB(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType enpointType, USBH_InitialTransactionPID initialPID, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback)
{
    USBHostPipeData* pipe = GetPipeData(pipeIndex);
    if (pipe != nullptr)
    {
        pipe->TransactionCallback = std::move(callback);
        pipe->URBState = USB_URBState::NotReady;
        return m_Driver->HostSubmitRequest(pipeIndex, direction, enpointType, initialPID, buffer, length, doPing);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::ControlSendSetup(USB_PipeIndex pipeIndex, USB_ControlRequest* request, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::HOST_TO_DEVICE, USB_TransferType::CONTROL, USBH_InitialTransactionPID::Setup, request, sizeof(USB_ControlRequest), false, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::ControlSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::HOST_TO_DEVICE, USB_TransferType::CONTROL, USBH_InitialTransactionPID::Data, buffer, length, doPing, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::ControlReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::DEVICE_TO_HOST, USB_TransferType::CONTROL, USBH_InitialTransactionPID::Data, buffer, length, false, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::BulkSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, bool doPing, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::HOST_TO_DEVICE, USB_TransferType::BULK, USBH_InitialTransactionPID::Data, buffer, length, doPing, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::BulkReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::DEVICE_TO_HOST, USB_TransferType::BULK, USBH_InitialTransactionPID::Data, buffer, length, false, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::InterruptReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::DEVICE_TO_HOST, USB_TransferType::INTERRUPT, USBH_InitialTransactionPID::Data, buffer, length, false, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::InterruptSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::HOST_TO_DEVICE, USB_TransferType::INTERRUPT, USBH_InitialTransactionPID::Data, buffer, length, false, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::IsochronousReceiveData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::DEVICE_TO_HOST, USB_TransferType::ISOCHRONOUS, USBH_InitialTransactionPID::Data, buffer, length, false, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::IsochronousSendData(USB_PipeIndex pipeIndex, void* buffer, size_t length, USB_TransactionCallback&& callback)
{
    return SubmitURB(pipeIndex, USB_RequestDirection::HOST_TO_DEVICE, USB_TransferType::ISOCHRONOUS, USBH_InitialTransactionPID::Data, buffer, length, false, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_PipeIndex USBHost::AllocPipe(uint8_t endpointAddr)
{
    for (USB_PipeIndex i = 0; i < m_Pipes.size(); ++i)
    {
        if (!m_Pipes[i].Claimed)
        {
            m_Pipes[i].TransactionCallback = nullptr;
            m_Pipes[i].EndpointAddr = endpointAddr;
            m_Pipes[i].URBState = USB_URBState::Idle;
            m_Pipes[i].Claimed = true;
            return i;
        }
    }
    if (m_Pipes.size() < m_Driver->GetMaxPipeCount())
    {
        m_Pipes.emplace_back(endpointAddr, true);
        return m_Pipes.size() - 1;
    }
    return USB_INVALID_PIPE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::FreePipe(USB_PipeIndex pipeIndex)
{
    if (pipeIndex >= 0 && pipeIndex < m_Pipes.size())
    {
        m_Pipes[pipeIndex].TransactionCallback = nullptr;
        m_Pipes[pipeIndex].EndpointAddr = 0;
        m_Pipes[pipeIndex].URBState = USB_URBState::Idle;
        m_Pipes[pipeIndex].Claimed = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::ConfigureDevice(const USB_DescConfiguration* configDesc, uint8_t deviceAddr)
{
    USBDeviceNode* device = GetDevice(deviceAddr);
    if (device == nullptr) {
        return false;
    }

    const void* endDesc = reinterpret_cast<const uint8_t*>(configDesc) + PLittleEndianToHost(configDesc->wTotalLength);

    for (const USB_DescriptorHeader* desc = configDesc->GetNext(); desc < endDesc; )
    {
        const USB_DescInterfaceAssociation* desc_iad = nullptr;
        if (desc->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION)
        {
            desc_iad = static_cast<const USB_DescInterfaceAssociation*>(desc);

            desc = desc->GetNext(); // Next desc should be Interface.
        }

        if (desc->bDescriptorType != USB_DescriptorType::INTERFACE) {
            return false;
        }

        const USB_DescInterface* interfaceDesc = static_cast<const USB_DescInterface*>(desc);
        desc = desc->GetNext();

        if (desc_iad != nullptr)
        {
            // IAD's first interface number and class should match current interface
            if (desc_iad->bFirstInterface != interfaceDesc->bInterfaceNumber || desc_iad->bFunctionClass != interfaceDesc->bInterfaceClass) {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Invalid interface association.");
                return false;
            }
        }

        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Class    : {:x}h", int(interfaceDesc->bInterfaceClass));
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "SubClass : {:x}h", interfaceDesc->bInterfaceSubClass);
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Protocol : {:x}h", interfaceDesc->bInterfaceProtocol);

        if (interfaceDesc->bInterfaceClass == USB_ClassCode::HUB)
        {
            const USB_DescriptorHeader* nextDesc = nullptr;
            if (!ConfigureHubInterface(deviceAddr, interfaceDesc, endDesc, &nextDesc)) {
                return false;
            }
            desc = nextDesc;
            continue;
        }

        // Find driver for this interface.
        bool driverFound = false;
        for (const Ptr<USBClassDriverHost>& driver : m_ClassDrivers)
        {
            try
            {
                const USB_DescriptorHeader* nextDesc = driver->Open(deviceAddr, interfaceDesc, desc_iad, endDesc);
                driver->m_IsActive = true;

                kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSB, "{} opened", driver->GetName());

                desc = nextDesc;

                driverFound = true;
                break;
            }
            PERROR_CATCH([](PErrorCode error) { kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to open channel."); });
        }
        if (!driverFound)
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Interface {}: class = {}, subclass = {}, protocol = {} is not supported.",
                interfaceDesc->bInterfaceNumber, std::to_underlying(interfaceDesc->bInterfaceClass), interfaceDesc->bInterfaceSubClass, interfaceDesc->bInterfaceProtocol);

            // Find the next interface.
            for (desc = desc->GetNext(); desc < endDesc; desc = desc->GetNext())
            {
                if (desc->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION || desc->bDescriptorType == USB_DescriptorType::INTERFACE) {
                    break;
                }
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::PushEvent(USBHostEventID eventID, bool clearQueue)
{
    return PushEvent(USBHostEvent(eventID), clearQueue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::PushEvent(const USBHostEvent& event, bool clearQueue)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);

    static volatile uint32_t maxEvents = 0;

    if (clearQueue)
    {
        m_EventQueue.Clear();
    }

    m_EventQueue.Write(&event, 1);
    if (m_EventQueue.GetLength() > maxEvents) {
        maxEvents = m_EventQueue.GetLength();
    }

    m_EventQueueCondition.WakeupAll();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::PopEvent(USBHostEvent& event)
{
    kassert(m_Mutex.IsLocked());
    m_Mutex.Unlock();

    bool result;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        while (m_EventQueue.GetLength() == 0)
        {
            const PErrorCode waitResult = m_EventQueueCondition.IRQWaitDeadline(m_DeviceAttachDeadline);
            if (waitResult != PErrorCode::Success)
            {
                if (waitResult == PErrorCode::TIMEDOUT)
                {
                    set_last_error(waitResult);
                    break;
                }
            }
        }
        result = m_EventQueue.Read(&event, 1) == 1;
    } CRITICAL_END;
    m_Mutex.Lock();
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::Reset()
{
    m_PortEnabled     = false;
    m_ResetErrorCount = 0;
    m_EnumErrorCount  = 0;
    m_HubPortChangeActive = false;
    m_DeviceAttachDeadline = TimeValNanos::infinit;

    m_Device0 = USBDeviceNode();
    m_Device0.m_Address = 0;
    m_Device0.m_Speed   = USB_Speed::FULL;

    m_ControlHandler.Reset();
    m_Enumerator.Reset();
    m_Pipes.clear();
    m_Devices.clear();
    m_PendingHubPortChanges.clear();
    m_HubPollRestartList.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::Stop()
{
    m_Driver->StopHost();
    m_ControlHandler.FreePipes();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::CloseActiveClassDrivers()
{
    bool closedClassDrivers = false;

    for (const Ptr<USBClassDriverHost>& driver : m_ClassDrivers)
    {
        if (driver->IsActive())
        {
            try
            {
                driver->Close();
            }
            PERROR_CATCH(
                [](PErrorCode error)
                {
                    kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to close channel.");
                }
            );

            driver->m_IsActive = false;
            closedClassDrivers = true;
        }
    }
    return closedClassDrivers;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleDeviceDisconnected()
{
    const bool wasConnected = m_PortEnabled;
    const bool wasConnecting = !m_DeviceAttachDeadline.IsInfinit();

    Stop();
    const bool closedClassDrivers = CloseActiveClassDrivers();
    Reset();

    if (wasConnected || wasConnecting || closedClassDrivers)
    {
        SignalConnectionChanged(false);
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Device disconnected.");
    }
    m_Driver->StartHost();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::PrepareDevice0(USB_Speed speed, uint8_t parentHubAddress, uint8_t parentHubPort)
{
    m_Device0 = USBDeviceNode();
    m_Device0.m_Address = 0;
    m_Device0.m_ParentHubAddress = parentHubAddress;
    m_Device0.m_ParentHubPort = parentHubPort;
    m_Device0.m_Speed = speed;
    m_Device0.m_IsConnected = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::CloseDevice(uint8_t deviceAddr)
{
    USBDeviceNode* device = GetDevice(deviceAddr);
    if (device == nullptr) {
        return;
    }

    for (USBDeviceNode& childDevice : m_Devices)
    {
        if (childDevice.m_IsConnected && childDevice.m_ParentHubAddress == deviceAddr) {
            CloseDevice(childDevice.m_Address);
        }
    }

    if (device->m_IsHub) {
        StopHubInterruptReceive(*device);
    }
    CloseDeviceClassDrivers(deviceAddr);
    *device = USBDeviceNode();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::CloseDeviceClassDrivers(uint8_t deviceAddr)
{
    for (const Ptr<USBClassDriverHost>& driver : m_ClassDrivers)
    {
        if (driver->IsActive()) {
            driver->CloseDevice(deviceAddr);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDeviceNode* USBHost::GetDeviceOnHubPort(uint8_t hubAddress, uint8_t portIndex)
{
    for (USBDeviceNode& device : m_Devices)
    {
        if (device.m_IsConnected && device.m_ParentHubAddress == hubAddress && device.m_ParentHubPort == portIndex) {
            return &device;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostPipeData* USBHost::GetPipeData(USB_PipeIndex pipeIndex)
{
    if (pipeIndex >= 0 && pipeIndex < m_Pipes.size() && m_Pipes[pipeIndex].Claimed) {
        return &m_Pipes[pipeIndex];
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::SetupClassDrivers(uint8_t deviceAddr)
{
    if (m_ClassDrivers.empty())
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "No class drivers has been registered.");
    }
    else
    {
        for (const Ptr<USBClassDriverHost>& driver : m_ClassDrivers)
        {
            if (driver->IsActive()) {
                driver->StartupDevice(deviceAddr);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleEnumerationDone(bool result, uint8_t deviceAddr)
{
    if (result)
    {
        USBDeviceNode* device = GetDevice(deviceAddr);
        if (device != nullptr)
        {
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Manufacturer : {}",   (device->m_ManufacturerString.empty())  ? "N/A" : device->m_ManufacturerString.c_str());
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Product : {}",        (device->m_ProductString.empty())       ? "N/A" : device->m_ProductString.c_str());
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Serial Number : {}",  (device->m_SerialNumberString.empty())  ? "N/A" : device->m_SerialNumberString.c_str());

            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Enumeration done. Device has {} configurations.", device->m_DeviceDesc.bNumConfigurations);

            if (!VFSelectConfiguration.Empty()) {
                device->m_SelectedConfiguration = VFSelectConfiguration(this);
            }
            m_ControlHandler.ReqSetConfiguration(deviceAddr, device->m_SelectedConfiguration, p_bind_method(this, &USBHost::HandleSetConfigurationResult));
        }
    }
    else
    {
        m_Enumerator.Reset();
        if (m_Device0.m_ParentHubAddress != 0)
        {
            kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "Device enumeration failed on hub {} port {}.", m_Device0.m_ParentHubAddress, m_Device0.m_ParentHubPort);
            CompleteHubPortChange(m_Device0.m_ParentHubAddress);
        }
        else if (++m_EnumErrorCount > 3)
        {
            kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "Device enumeration failed.");
        }
        else
        {
            m_ControlHandler.FreePipes();
            RestartDeviceInitialization();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleSetConfigurationResult(bool result, uint8_t deviceAddr)
{
    if (result)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Configuration set.");

        USBDeviceNode* device = GetDevice(deviceAddr);

        if (device != nullptr && device->m_IsHub)
        {
            InitializeHub(deviceAddr);
        }
        else if (device != nullptr && device->m_SupportRemoteWakeup)
        {
            USB_ControlRequest request(
                USB_RequestRecipient::DEVICE,
                USB_RequestType::STANDARD,
                USB_RequestDirection::HOST_TO_DEVICE,
                uint8_t(USB_RequestCode::SET_FEATURE),
                uint16_t(USB_RequestFeatureSelector::DEVICE_REMOTE_WAKEUP),
                0,
                0
            );
            m_ControlHandler.SendControlRequest(deviceAddr, request, nullptr, p_bind_method(this, &USBHost::HandleSetWakeupFeatureResult));
        }
        else
        {
            SetupClassDrivers(deviceAddr);
            FinishDeviceConfiguration(deviceAddr);
        }
    }
    else
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Set configuration request failed.");
        USBDeviceNode* device = GetDevice(deviceAddr);
        if (device != nullptr && device->m_ParentHubAddress != 0)
        {
            const uint8_t parentHubAddress = device->m_ParentHubAddress;
            CloseDevice(deviceAddr);
            CompleteHubPortChange(parentHubAddress);
        }
        else
        {
            m_ControlHandler.FreePipes();
            RestartDeviceInitialization();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleSetWakeupFeatureResult(bool result, uint8_t deviceAddr)
{
    if (result) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Device remote wakeup enabled.");
    } else {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Remote wakeup not supported by device.");
    }
    SetupClassDrivers(deviceAddr);
    FinishDeviceConfiguration(deviceAddr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::FinishDeviceConfiguration(uint8_t deviceAddr)
{
    USBDeviceNode* device = GetDevice(deviceAddr);
    if (device == nullptr) {
        return;
    }
    device->m_IsConfigured = true;
    if (device->m_ParentHubAddress != 0) {
        CompleteHubPortChange(device->m_ParentHubAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleURBStateChanged(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transferLength)
{
    USBHostPipeData* pipe = GetPipeData(pipeIndex);

    if (pipe != nullptr && pipe->TransactionCallback)
    {
        if (urbState == USB_URBState::Done || urbState == USB_URBState::Error)
        {
            USB_TransactionCallback callback = std::move(pipe->TransactionCallback);
            pipe->TransactionCallback = nullptr;
            pipe->URBState = USB_URBState::Idle;
            callback(pipeIndex, urbState, transferLength);
        }
        else
        {
            pipe->URBState = urbState;
            pipe->TransactionCallback(pipeIndex, urbState, transferLength);
        }
    }
    else if (pipe != nullptr)
    {
        pipe->URBState = urbState;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::ConfigureHubInterface(uint8_t deviceAddr, const USB_DescInterface* interfaceDesc, const void* endDesc, const USB_DescriptorHeader** nextDesc)
{
    USBDeviceNode* device = GetDevice(deviceAddr);
    if (device == nullptr) {
        return false;
    }

    device->m_IsHub = true;
    device->m_HubStatusEndpoint = USB_INVALID_ENDPOINT;
    device->m_HubStatusEndpointSize = 0;

    const USB_DescriptorHeader* desc = interfaceDesc->GetNext();
    for (; desc < endDesc; desc = desc->GetNext())
    {
        if (desc->bDescriptorType == USB_DescriptorType::INTERFACE || desc->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION) {
            break;
        }
        if (desc->bDescriptorType != USB_DescriptorType::ENDPOINT) {
            continue;
        }

        const USB_DescEndpoint* endpointDesc = static_cast<const USB_DescEndpoint*>(desc);
        if (endpointDesc->GetTransferType() == USB_TransferType::INTERRUPT && (endpointDesc->bEndpointAddress & USB_ADDRESS_DIR_IN) != 0)
        {
            if (!endpointDesc->Validate(device->m_Speed)) {
                return false;
            }
            device->m_HubStatusEndpoint = endpointDesc->bEndpointAddress;
            device->m_HubStatusEndpointSize = endpointDesc->GetMaxPacketSize();
            break;
        }
    }

    if (device->m_HubStatusEndpoint == USB_INVALID_ENDPOINT || device->m_HubStatusEndpointSize == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Hub interface has no valid interrupt status endpoint.");
        return false;
    }

    device->m_HubStatusPipe = AllocPipe(device->m_HubStatusEndpoint);
    if (device->m_HubStatusPipe == USB_INVALID_PIPE)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to allocate hub status pipe.");
        return false;
    }

    if (!OpenPipe(device->m_HubStatusPipe, device->m_HubStatusEndpoint, device->m_Address, device->m_Speed, USB_TransferType::INTERRUPT, device->m_HubStatusEndpointSize))
    {
        FreePipe(device->m_HubStatusPipe);
        device->m_HubStatusPipe = USB_INVALID_PIPE;
        return false;
    }

    SetDataToggle(device->m_HubStatusPipe, false);
    *nextDesc = desc;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::InitializeHub(uint8_t deviceAddr)
{
    uint8_t* buffer = m_ControlHandler.GetCtrlDataBuffer();
    m_ControlHandler.ReqGetHubDescriptor(deviceAddr, buffer, sizeof(USB_DescHub), p_bind_method(this, &USBHost::HandleGetHubDescriptorResult));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleGetHubDescriptorResult(bool result, uint8_t deviceAddr)
{
    USBDeviceNode* device = GetDevice(deviceAddr);
    if (device == nullptr) {
        return;
    }

    if (!result)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to get hub descriptor.");
        FinishDeviceConfiguration(deviceAddr);
        return;
    }

    const USB_DescHub* hubDesc = reinterpret_cast<const USB_DescHub*>(m_ControlHandler.GetCtrlDataBuffer());
    if (hubDesc->bDescriptorType != USB_DescriptorType::HUB || hubDesc->bNbrPorts == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Invalid hub descriptor.");
        FinishDeviceConfiguration(deviceAddr);
        return;
    }

    device->m_HubPortCount = hubDesc->bNbrPorts;
    device->m_HubPowerOnDelayMS = std::max<uint16_t>(hubDesc->GetPowerOnDelayMS(), 100);

    const size_t statusBitmapBytes = (size_t(device->m_HubPortCount) + 1 + 7) / 8;
    device->m_HubStatusBuffer.resize(std::max(device->m_HubStatusEndpointSize, statusBitmapBytes));

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Hub {} has {} ports.", deviceAddr, device->m_HubPortCount);

    PowerHubPort(deviceAddr, 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::PowerHubPort(uint8_t deviceAddr, uint8_t portIndex)
{
    USBDeviceNode* device = GetDevice(deviceAddr);
    if (device == nullptr) {
        return;
    }

    if (portIndex > device->m_HubPortCount)
    {
        if (device->m_HubPowerOnDelayMS != 0) {
            snooze_ms(device->m_HubPowerOnDelayMS);
        }
        for (uint8_t port = 1; port <= device->m_HubPortCount; ++port)
        {
            QueueHubPortChange(deviceAddr, port);
        }
        QueueHubPollRestart(deviceAddr);
        FinishDeviceConfiguration(deviceAddr);
        ProcessNextHubPortChange();
        return;
    }

    m_ControlHandler.ReqSetHubPortFeature(deviceAddr, portIndex, USB_HubFeatureSelector::PORT_POWER,
        [this, portIndex](bool result, uint8_t deviceAddr)
        {
            if (!result) {
                kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "Failed to power hub {} port {}.", deviceAddr, portIndex);
            }
            PowerHubPort(deviceAddr, uint8_t(portIndex + 1));
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::StartHubInterruptReceive(uint8_t deviceAddr)
{
    USBDeviceNode* hub = GetDevice(deviceAddr);
    if (hub == nullptr || !hub->m_IsHub || hub->m_HubStatusPipe == USB_INVALID_PIPE || hub->m_HubStatusBuffer.empty()) {
        return;
    }
    if (GetURBState(hub->m_HubStatusPipe) != USB_URBState::Idle) {
        return;
    }
    std::fill(hub->m_HubStatusBuffer.begin(), hub->m_HubStatusBuffer.end(), 0);
    InterruptReceiveData(hub->m_HubStatusPipe, hub->m_HubStatusBuffer.data(), hub->m_HubStatusBuffer.size(), p_bind_method(this, &USBHost::HandleHubStatusTransaction));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::StopHubInterruptReceive(USBDeviceNode& hub)
{
    if (hub.m_HubStatusPipe != USB_INVALID_PIPE)
    {
        ClosePipe(hub.m_HubStatusPipe);
        FreePipe(hub.m_HubStatusPipe);
        hub.m_HubStatusPipe = USB_INVALID_PIPE;
    }
    hub.m_HubStatusBuffer.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleHubStatusTransaction(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    USBDeviceNode* hub = nullptr;
    for (USBDeviceNode& device : m_Devices)
    {
        if (device.m_IsConnected && device.m_IsHub && device.m_HubStatusPipe == pipeIndex)
        {
            hub = &device;
            break;
        }
    }
    if (hub == nullptr) {
        return;
    }
    if (urbState == USB_URBState::NotReady) {
        return;
    }
    if (urbState != USB_URBState::Done)
    {
        QueueHubPollRestart(hub->m_Address);
        ProcessNextHubPortChange();
        return;
    }

    bool hasPortChanges = false;
    const size_t bitCount = std::min(transactionLength * 8, size_t(hub->m_HubPortCount) + 1);
    for (size_t portIndex = 1; portIndex < bitCount; ++portIndex)
    {
        const size_t byteIndex = portIndex / 8;
        const uint8_t bitMask = uint8_t(1u << (portIndex & 7));

        if ((hub->m_HubStatusBuffer[byteIndex] & bitMask) != 0)
        {
            QueueHubPortChange(hub->m_Address, uint8_t(portIndex));
            hasPortChanges = true;
        }
    }

    if (hasPortChanges)
    {
        QueueHubPollRestart(hub->m_Address);
        ProcessNextHubPortChange();
    }
    else
    {
        StartHubInterruptReceive(hub->m_Address);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::QueueHubPortChange(uint8_t hubAddress, uint8_t portIndex)
{
    if (hubAddress == 0 || portIndex == 0) {
        return;
    }
    for (const USBHostHubPortEvent& event : m_PendingHubPortChanges)
    {
        if (event.HubAddress == hubAddress && event.PortIndex == portIndex) {
            return;
        }
    }
    m_PendingHubPortChanges.emplace_back(hubAddress, portIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::ProcessNextHubPortChange()
{
    if (m_HubPortChangeActive) {
        return;
    }

    while (!m_PendingHubPortChanges.empty())
    {
        const USBHostHubPortEvent event = m_PendingHubPortChanges.front();
        m_PendingHubPortChanges.pop_front();

        USBDeviceNode* hub = GetDevice(event.HubAddress);
        if (hub == nullptr || !hub->m_IsHub || event.PortIndex == 0 || event.PortIndex > hub->m_HubPortCount) {
            continue;
        }

        m_HubPortChangeActive = true;
        USB_HubPortStatus* status = reinterpret_cast<USB_HubPortStatus*>(m_ControlHandler.GetCtrlDataBuffer());
        if (m_ControlHandler.ReqGetHubPortStatus(event.HubAddress, event.PortIndex, status,
            [this, event](bool result, uint8_t deviceAddr)
            {
                HandleHubPortStatusResult(result, event.HubAddress, event.PortIndex);
            }
        ))
        {
            return;
        }
        m_HubPortChangeActive = false;
    }
    RestartPendingHubPolls();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::CompleteHubPortChange(uint8_t hubAddress)
{
    if (hubAddress != 0) {
        QueueHubPollRestart(hubAddress);
    }
    m_HubPortChangeActive = false;
    ProcessNextHubPortChange();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::QueueHubPollRestart(uint8_t hubAddress)
{
    if (hubAddress == 0) {
        return;
    }
    if (std::find(m_HubPollRestartList.begin(), m_HubPollRestartList.end(), hubAddress) == m_HubPollRestartList.end()) {
        m_HubPollRestartList.push_back(hubAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::RestartPendingHubPolls()
{
    std::vector<uint8_t> hubList;
    hubList.swap(m_HubPollRestartList);

    for (uint8_t hubAddress : hubList)
    {
        StartHubInterruptReceive(hubAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleHubPortStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex)
{
    if (!result)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "Failed to get hub {} port {} status.", hubAddress, portIndex);
        CompleteHubPortChange(hubAddress);
        return;
    }

    const USB_HubPortStatus* status = reinterpret_cast<const USB_HubPortStatus*>(m_ControlHandler.GetCtrlDataBuffer());
    const uint16_t portStatus = PLittleEndianToHost(status->wPortStatus);
    const uint16_t portChange = PLittleEndianToHost(status->wPortChange);

    if ((portChange & USB_HubPortStatus::PORT_CHANGE_CONNECTION) != 0)
    {
        ClearHubPortConnectionChange(hubAddress, portIndex, portStatus);
        return;
    }

    if ((portStatus & USB_HubPortStatus::PORT_STATUS_CONNECTION) == 0)
    {
        USBDeviceNode* device = GetDeviceOnHubPort(hubAddress, portIndex);
        if (device != nullptr) {
            CloseDevice(device->m_Address);
        }
        CompleteHubPortChange(hubAddress);
        return;
    }

    if ((portStatus & USB_HubPortStatus::PORT_STATUS_ENABLE) == 0 || GetDeviceOnHubPort(hubAddress, portIndex) == nullptr)
    {
        ResetHubPort(hubAddress, portIndex);
        return;
    }

    CompleteHubPortChange(hubAddress);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::ClearHubPortConnectionChange(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus)
{
    m_ControlHandler.ReqClearHubPortFeature(hubAddress, portIndex, USB_HubFeatureSelector::C_PORT_CONNECTION,
        [this, hubAddress, portIndex, portStatus](bool result, uint8_t deviceAddr)
        {
            if (!result)
            {
                CompleteHubPortChange(hubAddress);
                return;
            }

            USBDeviceNode* device = GetDeviceOnHubPort(hubAddress, portIndex);
            if (device != nullptr) {
                CloseDevice(device->m_Address);
            }

            if ((portStatus & USB_HubPortStatus::PORT_STATUS_CONNECTION) != 0) {
                ResetHubPort(hubAddress, portIndex);
            } else {
                CompleteHubPortChange(hubAddress);
            }
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::ResetHubPort(uint8_t hubAddress, uint8_t portIndex)
{
    USBDeviceNode* device = GetDeviceOnHubPort(hubAddress, portIndex);
    if (device != nullptr) {
        CloseDevice(device->m_Address);
    }

    m_ControlHandler.ReqSetHubPortFeature(hubAddress, portIndex, USB_HubFeatureSelector::PORT_RESET,
        [this, hubAddress, portIndex](bool result, uint8_t deviceAddr)
        {
            if (!result)
            {
                CompleteHubPortChange(hubAddress);
                return;
            }

            snooze_ms(100);

            USB_HubPortStatus* status = reinterpret_cast<USB_HubPortStatus*>(m_ControlHandler.GetCtrlDataBuffer());
            m_ControlHandler.ReqGetHubPortStatus(hubAddress, portIndex, status,
                [this, hubAddress, portIndex](bool result, uint8_t deviceAddr)
                {
                    HandleHubPortResetStatusResult(result, hubAddress, portIndex);
                }
            );
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleHubPortResetStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex)
{
    if (!result)
    {
        CompleteHubPortChange(hubAddress);
        return;
    }

    const USB_HubPortStatus* status = reinterpret_cast<const USB_HubPortStatus*>(m_ControlHandler.GetCtrlDataBuffer());
    const uint16_t portStatus = PLittleEndianToHost(status->wPortStatus);
    const uint16_t portChange = PLittleEndianToHost(status->wPortChange);

    if ((portChange & USB_HubPortStatus::PORT_CHANGE_RESET) != 0)
    {
        m_ControlHandler.ReqClearHubPortFeature(hubAddress, portIndex, USB_HubFeatureSelector::C_PORT_RESET,
            [this, hubAddress, portIndex, portStatus](bool result, uint8_t deviceAddr)
            {
                HandleHubPortResetChangeCleared(result, hubAddress, portIndex, portStatus);
            }
        );
    }
    else
    {
        HandleHubPortResetChangeCleared(true, hubAddress, portIndex, portStatus);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::HandleHubPortResetChangeCleared(bool result, uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus)
{
    if (!result)
    {
        CompleteHubPortChange(hubAddress);
        return;
    }

    if ((portStatus & USB_HubPortStatus::PORT_STATUS_CONNECTION) == 0 || (portStatus & USB_HubPortStatus::PORT_STATUS_ENABLE) == 0)
    {
        CompleteHubPortChange(hubAddress);
        return;
    }

    EnumerateHubPortDevice(hubAddress, portIndex, portStatus);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::EnumerateHubPortDevice(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus)
{
    const USB_Speed speed = GetHubPortDeviceSpeed(portStatus);

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Enumerating hub {} port {} at {} speed.", hubAddress, portIndex, USB_GetSpeedName(speed));

    PrepareDevice0(speed, hubAddress, portIndex);
    m_ControlHandler.AllocPipes(0, speed, (speed == USB_Speed::LOW) ? 8 : 64);
    if (!m_Enumerator.Enumerate(p_bind_method(this, &USBHost::HandleEnumerationDone))) {
        CompleteHubPortChange(hubAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_Speed USBHost::GetHubPortDeviceSpeed(uint16_t portStatus) const
{
    if ((portStatus & USB_HubPortStatus::PORT_STATUS_HIGH_SPEED) != 0) {
        return USB_Speed::HIGH;
    }
    if ((portStatus & USB_HubPortStatus::PORT_STATUS_LOW_SPEED) != 0) {
        return USB_Speed::LOW;
    }
    return USB_Speed::FULL;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::IRQDeviceConnected()
{
    PushEvent(USBHostEventID::DeviceConnected, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::IRQDeviceDisconnected()
{
    PushEvent(USBHostEventID::DeviceDisconnected, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::IRQPortEnableChange(bool isEnabled)
{
    if (isEnabled) {
        PushEvent(USBHostEventID::DeviceAttached);
    } else {
        PushEvent(USBHostEventID::DeviceDetached, true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::IRQStartOfFrame()
{
    for (const Ptr<USBClassDriverHost>& driver : m_ClassDrivers)
    {
        if (driver->IsActive()) {
            driver->StartOfFrame();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::IRQPipeURBStateChanged(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t length)
{
    USBHostEvent event(USBHostEventID::URBStateChanged);

    event.URBStateChanged.PipeIndex         = pipeIndex;
    event.URBStateChanged.TransferLength    = length;
    event.URBStateChanged.URBState          = urbState;

    PushEvent(event);
}

} // namespace kernel
