// This file is part of PadOS.
//
// Copyright (C) 2022-2024 Kurt Skauen <http://kavionic.com/>
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

using namespace os;


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
    Start_trw(PThreadDetachState_Detached);

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
    m_Devices.emplace_back(m_Device0);
    USBDeviceNode* device = &m_Devices.back();
    device->m_Address = uint8_t(m_Devices.size());
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
        if (index < m_Devices.size()) {
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
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Device connected.");

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

                    m_Device0.m_Speed = m_Driver->HostGetSpeed();
                    m_ControlHandler.AllocPipes(0, m_Device0.m_Speed, 0);
                    m_Enumerator.Enumerate(bind_method(this, &USBHost::HandleEnumerationDone));
                    break;
                case USBHostEventID::DeviceDetached:
                    m_PortEnabled = false;
                    break;
                case USBHostEventID::DeviceDisconnected:
                    Stop();
                    Reset();

                    for (const Ptr<USBClassDriverHost>& driver : m_ClassDrivers)
                    {
                        if (driver->IsActive()) {
                            try
                            {
                                driver->Close();
                            }
                            PERROR_CATCH([](PErrorCode error) { kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to close channel."); });
                        }
                    }
                    SignalConnectionChanged(false);
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Device disconnected.");

                    m_Driver->StartHost();
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
    PushEvent(USBHostEventID::DeviceConnected);
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
            m_Pipes[i].EndpointAddr = endpointAddr;
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
    if (pipeIndex >= 0 && pipeIndex < m_Pipes.size()) {
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

    const void* endDesc = reinterpret_cast<const uint8_t*>(configDesc) + configDesc->wTotalLength;

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

bool USBHost::PushEvent(USBHostEventID eventID)
{
    return PushEvent(USBHostEvent(eventID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHost::PushEvent(const USBHostEvent& event)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);

    static volatile uint32_t maxEvents = 0;

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
                if (waitResult == PErrorCode::Timeout)
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

    m_Device0.m_Address = 0;
    m_Device0.m_Speed   = USB_Speed::FULL;

    m_Pipes.clear();
    m_ControlHandler.Reset();
    m_Enumerator.Reset();
    m_Devices.clear();
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

void USBHost::SetupClassDrivers()
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
                driver->Startup();
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
            m_ControlHandler.ReqSetConfiguration(deviceAddr, device->m_SelectedConfiguration, bind_method(this, &USBHost::HandleSetConfigurationResult));
        }
    }
    else
    {
        m_Enumerator.Reset();
        if (++m_EnumErrorCount > 3)
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
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Configuration set.");

    USBDeviceNode* device = GetDevice(deviceAddr);

    if (device != nullptr && device->m_SupportRemoteWakeup)
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
        m_ControlHandler.SendControlRequest(deviceAddr, request, nullptr, bind_method(this, &USBHost::HandleSetWakeupFeatureResult));
    }
    else
    {
        SetupClassDrivers();
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
    SetupClassDrivers();
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

bool USBHost::IRQDeviceConnected()
{
    PushEvent(USBHostEventID::DeviceConnected);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::IRQDeviceDisconnected()
{
    PushEvent(USBHostEventID::DeviceDisconnected);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHost::IRQPortEnableChange(bool isEnabled)
{
    if (isEnabled) {
        PushEvent(USBHostEventID::DeviceAttached);
    } else {
        PushEvent(USBHostEventID::DeviceDetached);
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
