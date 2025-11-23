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
// Created: 27.05.2022 18:00

#include <string.h>
#include <Kernel/KLogging.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBDevice.h>
#include <Kernel/USB/USBDriver.h>
#include <Kernel/USB/USBClassDriverDevice.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/USB/USBLanguages.h>
#include <Utils/Utils.h>

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDevice::USBDevice()
    : KThread("usb_device")
    , m_Mutex("usb_device", PEMutexRecursionMode_RaiseError)
    , m_EventQueueCondition("usb_device_queue")
    , m_DeviceQualifier(0, USB_ClassCode::UNSPECIFIED, 0, 0, 0, 0)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDevice::~USBDevice()
{
    while (!m_ClassDrivers.empty())
    {
        RemoveClassDriver(m_ClassDrivers.back());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* USBDevice::Run()
{
    CRITICAL_SCOPE(m_Mutex);
    for(;;)
    {
        USBDeviceEvent event;

        if (!PopEvent(event)) {
            continue;
        }
        switch (event.EventID)
        {
            case USBDeviceEventID::BusReset:
                kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "BusReset. Speed: {}.", USB_GetSpeedName(event.BusReset.speed));
                BusReset();
                m_SelectedSpeed = event.BusReset.speed;
                break;
            case USBDeviceEventID::SessionEnded:
                kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "SessionEnded.");
                BusReset();
                break;
            case USBDeviceEventID::ControlRequestReceived:
            {
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "ControlRequestReceived.");

                SetIsConnected(true);

                USBEndpointState& outEndpoint = GetEndpoint(USB_MK_OUT_ADDRESS(0));
                outEndpoint.Busy    = false;
                outEndpoint.Claimed = false;

                USBEndpointState& inEndpoint = GetEndpoint(USB_MK_IN_ADDRESS(0));
                inEndpoint.Busy    = false;
                inEndpoint.Claimed = false;

                if (!HandleControlRequest(event.ControlRequestReceived.Request))
                {
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "ControlRequestReceived, Stall endpoint0.");
                    // Stall control endpoints on failure.
                    m_Driver->EndpointStall(USB_MK_OUT_ADDRESS(0));
                    m_Driver->EndpointStall(USB_MK_IN_ADDRESS(0));
                }
                break;
            }
            case USBDeviceEventID::TransferComplete:
            {
                const uint8_t endpointAddr = event.TransferComplete.EndpointAddr;

                kernel_log<PLogSeverity::INFO_FLOODING>(LogCategoryUSBDevice, "TransferComplete on endpoint {:02x} with {} bytes.", endpointAddr, event.TransferComplete.Length);

                USBEndpointState& endpoint = GetEndpoint(endpointAddr);
                endpoint.Busy    = false;
                endpoint.Claimed = false;

                if (USB_ADDRESS_EPNUM(endpointAddr) == 0)
                {
                    m_ControlTransfer.ControlTransferComplete(endpointAddr, event.TransferComplete.Result, event.TransferComplete.Length);
                }
                else
                {
                    Ptr<USBClassDriverDevice> driver = GetEndpointDriver(endpointAddr);
                    if (driver != nullptr)
                    {
                        kernel_log<PLogSeverity::INFO_FLOODING>(LogCategoryUSBDevice, "TransferComplete, call '{}' HandleDataTransfer().", driver->GetName());
                        driver->HandleDataTransfer(endpointAddr, event.TransferComplete.Result, event.TransferComplete.Length);
                    }
                    else
                    {
                        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "TransferComplete, no driver for endpoint {:02x}.", endpointAddr);
                    }
                }
                break;
            }
            case USBDeviceEventID::Suspend:
                if (m_IsConnected)
                {
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Suspend (Remote Wakeup is {}).", (m_RemoteWakeupEnabled) ? "enabled" : "disabled");
                    SetIsSuspended(true);
                }
                break;
            case USBDeviceEventID::Resume:
                if (m_IsConnected)
                {
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Resume.");
                    SetIsSuspended(false);
                }
                break;
            case USBDeviceEventID::StartOfFrame:
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "StartOfFrame.");

                if (m_IsSuspended) {
                    SetIsSuspended(false);
                }
                for (Ptr<USBClassDriverDevice> driver : m_ClassDrivers) {
                    driver->StartOfFrame();
                }
                break;
            default:
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Unknown event {}.", int(event.EventID));
                break;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::Setup(USBDriver* driver, uint32_t endpoint0Size, int threadPriority)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    m_Driver = driver;
    m_Endpoint0Size = endpoint0Size;

    m_ControlTransfer.Setup(this, driver, m_Endpoint0Size);

    m_Driver->IRQBusReset.Connect(this, &USBDevice::IRQBusReset);
    m_Driver->IRQSuspend.Connect(this, &USBDevice::IRQSuspend);
    m_Driver->IRQResume.Connect(this, &USBDevice::IRQResume);
    m_Driver->IRQSessionEnded.Connect(this, &USBDevice::IRQSessionEnded);
    m_Driver->IRQStartOfFrame.Connect(this, &USBDevice::IRQStartOfFrame);

    m_Driver->IRQControlRequestReceived.Connect(this, &USBDevice::IRQControlRequestReceived);
    m_Driver->IRQTransferComplete.Connect(this, &USBDevice::IRQTransferComplete);

    SetDeleteOnExit(false);
    Start_trw(PThreadDetachState_Detached, threadPriority);

    m_Driver->EnableIRQ(true);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::AddClassDriver(Ptr<USBClassDriverDevice> driver)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    driver->Init(this);
    m_ClassDrivers.push_back(driver);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::RemoveClassDriver(Ptr<USBClassDriverDevice> driver)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    auto i = std::find(m_ClassDrivers.begin(), m_ClassDrivers.end(), driver);
    if (i != m_ClassDrivers.end())
    {
        driver->Shutdown();
        m_ClassDrivers.erase(i);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::SetDeviceDescriptor(const USB_DescDevice& descriptor)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    m_DeviceDescriptor = descriptor;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::SetDeviceQualifier(const USB_DescDeviceQualifier& qualifier)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    m_DeviceQualifier = qualifier;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::AddConfigDescriptor(uint32_t index, const void* data, size_t length)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    std::vector<uint8_t>& buffer = m_ConfigDescriptors[index];
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::AddOtherConfigDescriptor(uint32_t index, const void* data, size_t length)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    std::vector<uint8_t>& buffer = m_OtherConfigDescriptors[index];
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::AddBOSDescriptor(const void* data, size_t length)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    m_BOSDescriptors.insert(m_BOSDescriptors.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::SetStringDescriptor(USB_LanguageID languageID, uint32_t index, const os::String& string)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    USB_DescString desc;
    static_assert(sizeof(desc) == sizeof(wchar16_t));

    // Convert the string to UTF-16.
    std::vector<wchar16_t> strBuffer;
    strBuffer.resize((255 - sizeof(USB_DescriptorHeader)) / sizeof(wchar16_t));
    const size_t charCount = string.copy_utf16(strBuffer.data(), strBuffer.size());

    desc.bLength = uint8_t(sizeof(USB_DescriptorHeader) + charCount * sizeof(wchar16_t));
    desc.bDescriptorType = USB_DescriptorType::STRING;

    std::vector<uint8_t>& descBuffer = m_StringDescriptors[languageID][index];
    descBuffer.resize(desc.bLength);
    memcpy(descBuffer.data(), &desc, sizeof(desc));
    memcpy(descBuffer.data() + sizeof(desc), strBuffer.data(), charCount * sizeof(wchar16_t));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescConfiguration* USBDevice::GetConfigDescriptor(uint32_t index) const
{
    kassert(m_Mutex.IsLocked());

    auto buffer = m_ConfigDescriptors.find(index);
    if (buffer != m_ConfigDescriptors.end() && buffer->second.size() >= sizeof(USB_DescConfiguration))
    {
        USB_DescConfiguration* desc = reinterpret_cast<USB_DescConfiguration*>(buffer->second.data());
        desc->wTotalLength = HostToLittleEndian(uint16_t(buffer->second.size()));
        return desc;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescConfiguration* USBDevice::GetOtherConfigDescriptor(uint32_t index) const
{
    kassert(m_Mutex.IsLocked());

    auto buffer = m_OtherConfigDescriptors.find(index);
    if (buffer != m_OtherConfigDescriptors.end() && buffer->second.size() >= sizeof(USB_DescConfiguration))
    {
        USB_DescConfiguration* desc = reinterpret_cast<USB_DescConfiguration*>(buffer->second.data());
        desc->wTotalLength = HostToLittleEndian(uint16_t(buffer->second.size()));
        return desc;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescBOS* USBDevice::GetBOSDescriptor() const
{
    kassert(m_Mutex.IsLocked());

    if (m_BOSDescriptors.size() >= sizeof(USB_DescBOS))
    {
        USB_DescBOS* desc = reinterpret_cast<USB_DescBOS*>(m_BOSDescriptors.data());
        desc->wTotalLength = HostToLittleEndian(uint16_t(m_BOSDescriptors.size()));
        return desc;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<USBClassDriverDevice> USBDevice::GetInterfaceDriver(uint8_t interfaceNum)
{
    kassert(m_Mutex.IsLocked());

    auto i = m_InterfaceToDriverMap.find(interfaceNum);
    if (i != m_InterfaceToDriverMap.end()) {
        return i->second;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<USBClassDriverDevice> USBDevice::GetEndpointDriver(uint8_t endpointAddress)
{
    kassert(m_Mutex.IsLocked());

    auto i = m_EndpointToDriverMap.find(endpointAddress);
    if (i != m_EndpointToDriverMap.end()) {
        return i->second;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBEndpointState& USBDevice::GetEndpoint(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    uint32_t index = endpointAddr & USB_ADDRESS_EPNUM_Msk;
    if (endpointAddr & USB_ADDRESS_DIR_IN) {
        index += USB_ADDRESS_MAX_EP_COUNT;
    }
    return m_EndpointStates[index];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::OpenEndpoint(const USB_DescEndpoint& endpointDescriptor)
{
    kassert(m_Mutex.IsLocked());

    if (!endpointDescriptor.Validate(m_SelectedSpeed)) {
        return false;
    }
    return m_Driver->EndpointOpen(endpointDescriptor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///
/// Parse consecutive in/out endpoint descriptors.
///////////////////////////////////////////////////////////////////////////////

const USB_DescriptorHeader* USBDevice::OpenEndpointPair(const USB_DescriptorHeader* desc, USB_TransferType transferType, uint8_t& endpointOut, uint8_t& endpointIn, uint16_t& endpointOutMaxSize, uint16_t& endpointInMaxSize)
{
    kassert(m_Mutex.IsLocked());

    bool endpointOutFound = false;
    bool endpointInFound  = false;

    for (int i = 0; i < 2; ++i)
    {
        if (desc->bDescriptorType != USB_DescriptorType::ENDPOINT) {
            break;
        }
        const USB_DescEndpoint& endpointDescriptor = *static_cast<const USB_DescEndpoint*>(desc);

        if (transferType != endpointDescriptor.GetTransferType()) {
            break;
        }
        if (!OpenEndpoint(endpointDescriptor)) {
            break;
        }

        if (endpointDescriptor.bEndpointAddress & USB_ADDRESS_DIR_IN)
        {
            endpointIn = endpointDescriptor.bEndpointAddress;
            endpointInMaxSize = endpointDescriptor.GetMaxPacketSize();
            endpointInFound = true;
        }
        else
        {
            endpointOut = endpointDescriptor.bEndpointAddress;
            endpointOutMaxSize = endpointDescriptor.GetMaxPacketSize();
            endpointOutFound = true;
        }
        desc = desc->GetNext();
    }
    if (endpointOutFound && endpointInFound)
    {
        return desc;
    }
    else
    {
        if (endpointOutFound) {
            CloseEndpoint(endpointOut);
        } else if (endpointInFound) {
            CloseEndpoint(endpointIn);
        }
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::CloseEndpoint(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    m_Driver->EndpointClose(endpointAddr);

    USBEndpointState& endpoint = GetEndpoint(endpointAddr);

    endpoint.Stalled = false;
    endpoint.Busy    = false;
    endpoint.Claimed = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::ClaimEndpoint(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    USBEndpointState& endpoint = GetEndpoint(endpointAddr);
    return endpoint.Claim();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::ReleaseEndpoint(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    USBEndpointState& endpoint = GetEndpoint(endpointAddr);
    return endpoint.Release();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::IsEndpointBusy(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    const USBEndpointState& endpoint = GetEndpoint(endpointAddr);
    return endpoint.Busy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::EndpointSetStall(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    USBEndpointState& endpoint = GetEndpoint(endpointAddr);
    if (!endpoint.Stalled)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Stall endpoint {:02x}.", endpointAddr);
        m_Driver->EndpointStall(endpointAddr);
        endpoint.Stalled = true;
        endpoint.Busy = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::EndpointClearStall(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    USBEndpointState& endpoint = GetEndpoint(endpointAddr);

    if (endpoint.Stalled)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Clear stall on endpoint {:02x}.", endpointAddr);
        m_Driver->EndpointClearStall(endpointAddr);
        endpoint.Stalled = false;
        endpoint.Busy = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::IsEndpointStalled(uint8_t endpointAddr)
{
    kassert(m_Mutex.IsLocked());

    const USBEndpointState& endpoint = GetEndpoint(endpointAddr);
    return endpoint.Stalled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::EndpointTransfer(uint8_t endpointAddr, uint8_t* buffer, size_t length)
{
    kassert(m_Mutex.IsLocked());

    kernel_log<PLogSeverity::INFO_FLOODING>(LogCategoryUSBDevice, "USBDevice::EndpointTransfer() transfer {} bytes on endpoint {:02x}.", length, endpointAddr);

    USBEndpointState& endpoint = GetEndpoint(endpointAddr);

    if (endpoint.Busy) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::EndpointTransfer() endpoint {:02x} is busy.", endpointAddr);
        return false;
    }
    endpoint.Busy = true;

    if (m_Driver->EndpointTransfer(endpointAddr, buffer, length))
    {
        return true;
    }
    else
    {
        endpoint.Busy    = false;
        endpoint.Claimed = false;
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::EndpointTransfer() failed.");
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::SetIsConnected(bool connected)
{
    kassert(m_Mutex.IsLocked());

    if (connected != m_IsConnected)
    {
        m_IsConnected = connected;
        SignalConnected(m_IsConnected);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::SetIsSuspended(bool suspended)
{
    kassert(m_Mutex.IsLocked());

    if (suspended != m_IsSuspended)
    {
        m_IsSuspended = suspended;
        if (m_IsSuspended) {
            SignalSuspend(m_RemoteWakeupEnabled);
        } else {
            SignalResume();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::BusReset()
{
    kassert(m_Mutex.IsLocked());

    UnsetConfiguration();
    m_SelectedSpeed = USB_Speed::LOW;
    m_ControlTransfer.Reset();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::UnsetConfiguration()
{
    kassert(m_Mutex.IsLocked());

    for (Ptr<USBClassDriverDevice> driver : m_ClassDrivers)
    {
        driver->Reset();
    }
    for (USBEndpointState& endpoint : m_EndpointStates)
    {
        endpoint.Reset();
    }
    m_IsAddressed           = false;
    m_RemoteWakeupEnabled   = false;
    m_RemoteWakeupSupport   = false;
    m_SelfPowered           = false;

    if (m_SelectedConfigNum != 0)
    {
        m_SelectedConfigNum = 0;
        SignalMounted(false);
    }
    m_InterfaceToDriverMap.clear();
    m_EndpointToDriverMap.clear();

    SetIsSuspended(false);
    SetIsConnected(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::HandleControlRequest(const USB_ControlRequest& request)
{
    kassert(m_Mutex.IsLocked());

    m_ControlTransfer.SetControlTransferHandler(ControlTransferHandler::None);

    USB_RequestType requestType = USB_RequestType((request.bmRequestType & USB_ControlRequest::REQUESTTYPE_TYPE_Msk) >> USB_ControlRequest::REQUESTTYPE_TYPE_Pos);
    if (requestType >= USB_RequestType::INVALID) {
        return false;
    }
    if (requestType == USB_RequestType::VENDOR)
    {
        if (!SignalHandleVendorControlTransfer.Empty())
        {
            m_ControlTransfer.SetControlTransferHandler(ControlTransferHandler::VendorSpecific);
            if (!SignalHandleVendorControlTransfer(USB_ControlStage::SETUP, request))
            {
                m_ControlTransfer.SetControlTransferHandler(ControlTransferHandler::None);
            }
            return true;
        }
        return false;
    }
    else
    {
        USB_RequestRecipient recipient = USB_RequestRecipient((request.bmRequestType & USB_ControlRequest::REQUESTTYPE_RECIPIENT_Msk) >> USB_ControlRequest::REQUESTTYPE_RECIPIENT_Pos);
        switch (recipient)
        {
            case USB_RequestRecipient::DEVICE:      return HandleDeviceControlRequests(request);
            case USB_RequestRecipient::INTERFACE:   return HandleInterfaceControlRequest(request);
            case USB_RequestRecipient::ENDPOINT:    return HandleEndpointControlRequest(request);
            default:
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::HandleControlRequest(): Unknown recipient {}.", int(recipient));
                return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::HandleDeviceControlRequests(const USB_ControlRequest& request)
{
    kassert(m_Mutex.IsLocked());

    const USB_RequestType requestType = USB_RequestType((request.bmRequestType & USB_ControlRequest::REQUESTTYPE_TYPE_Msk) >> USB_ControlRequest::REQUESTTYPE_TYPE_Pos);

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "{}: {}.", __PRETTY_FUNCTION__, int(requestType));

    if (requestType == USB_RequestType::CLASS)
    {
        const uint8_t interfaceNum = uint8_t(request.wIndex & 0xff);
        Ptr<USBClassDriverDevice> driver = GetInterfaceDriver(interfaceNum);
        if (driver != nullptr) {
            return InvokeClassDriverControlTransfer(driver, request); // Forward to class driver.
        }
        return false;
    }
    else if (requestType == USB_RequestType::STANDARD)
    {
        const USB_RequestCode requestCode = USB_RequestCode(request.bRequest);
        switch (requestCode)
        {
            case USB_RequestCode::SET_ADDRESS:
                // Driver returns 'true' if we need to respond with a status package, or 'false' if the response has been sent by the driver.
                if (m_Driver->SetAddress(uint8_t(request.wValue))) {
                    m_ControlTransfer.SendControlStatusReply(request);
                }
                m_IsAddressed = true;
                break;
            case USB_RequestCode::GET_CONFIGURATION:
            {
                uint8_t configNum = m_SelectedConfigNum;
                m_ControlTransfer.SendControlDataReply(request, &configNum, 1);
                break;
            }
            case USB_RequestCode::SET_CONFIGURATION:
            {
                const uint8_t configNum = uint8_t(request.wValue & 0xff);

                if (configNum != m_SelectedConfigNum)
                {
                    if (m_SelectedConfigNum != 0)
                    {
                        // Close all non-control endpoints and cancel any pending transfers.
                        m_Driver->EndpointCloseAll();
                        UnsetConfiguration();
                    }
                    if (configNum != 0)
                    {
                        if (!HandleSelectConfiguration(configNum)) {
                            return false;
                        }
                    }
                }
                m_ControlTransfer.SendControlStatusReply(request);
                break;
            }
            case USB_RequestCode::GET_DESCRIPTOR:
                if (!HandleGetDescriptor(request)) {
                    return false;
                }
                break;
            case USB_RequestCode::SET_FEATURE:
                if (USB_RequestFeatureSelector(request.wValue) == USB_RequestFeatureSelector::DEVICE_REMOTE_WAKEUP)
                {
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Enable remote wakeup.");
                    // Host may enable remote wake up before suspending.
                    m_RemoteWakeupEnabled = true;
                    m_ControlTransfer.SendControlStatusReply(request);
                }
                else // Unsupported feature request.
                {
                    return false;
                }
                break;
            case USB_RequestCode::CLEAR_FEATURE:
                if (USB_RequestFeatureSelector(request.wValue) == USB_RequestFeatureSelector::DEVICE_REMOTE_WAKEUP)
                {
                    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Disable remote wakeup.");

                    // Host may disable remote wake up after resuming.
                    m_RemoteWakeupEnabled = false;
                    m_ControlTransfer.SendControlStatusReply(request);
                }
                else // Unsupported feature request.
                {
                    return false;
                }
                break;
            case USB_RequestCode::GET_STATUS:
            {
                uint16_t status = 0;

                if (m_SelfPowered)          status |= USB_DEVICE_STATUS_SELF_POWERED;
                if (m_RemoteWakeupEnabled)  status |= USB_DEVICE_STATUS_REMOTE_WAKEUP_ENABLED;

                status = HostToLittleEndian(status);
                m_ControlTransfer.SendControlDataReply(request, &status, sizeof(status));
                break;
            }
            default:
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::HandleDeviceControlRequest(): Unknown control request {}.", int(requestCode));
                return false;
        }
    }
    else
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::HandleDeviceControlRequest(): unsupported device request {}.", int(requestType));
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::HandleInterfaceControlRequest(const USB_ControlRequest& request)
{
    kassert(m_Mutex.IsLocked());

    const USB_RequestCode requestCode  = USB_RequestCode(request.bRequest);
    const uint8_t         interfaceNum = uint8_t(request.wIndex & 0xff);

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "{}: {} {}.", __PRETTY_FUNCTION__, int(requestCode), interfaceNum);

    Ptr<USBClassDriverDevice> driver = GetInterfaceDriver(interfaceNum);
    if (driver != nullptr && InvokeClassDriverControlTransfer(driver, request)) {
        return true;
    }
    const USB_RequestType requestType = USB_RequestType((request.bmRequestType & USB_ControlRequest::REQUESTTYPE_TYPE_Msk) >> USB_ControlRequest::REQUESTTYPE_TYPE_Pos);
    if (requestType == USB_RequestType::STANDARD)
    {
        // For GET_INTERFACE, SET_INTERFACE and GET_STATUS it is mandatory to respond even if the class driver don't implement them.
        switch (requestCode)
        {
            case USB_RequestCode::GET_INTERFACE:
            case USB_RequestCode::SET_INTERFACE:
                m_ControlTransfer.SetControlTransferHandler(ControlTransferHandler::None);
                if (requestCode == USB_RequestCode::GET_INTERFACE)
                {
                    uint8_t alternate = 0;
                    m_ControlTransfer.SendControlDataReply(request, &alternate, 1);
                }
                else
                {
                    m_ControlTransfer.SendControlStatusReply(request);
                }
                return true;
            case USB_RequestCode::GET_STATUS:
            {
                uint16_t status = 0;
                m_ControlTransfer.SendControlDataReply(request, &status, sizeof(status));
                return true;
            }
            default:
                return false;
        }
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::HandleEndpointControlRequest(const USB_ControlRequest& request)
{
    kassert(m_Mutex.IsLocked());

    const USB_RequestCode requestCode  = USB_RequestCode(request.bRequest);
    const USB_RequestType requestType  = USB_RequestType((request.bmRequestType & USB_ControlRequest::REQUESTTYPE_TYPE_Msk) >> USB_ControlRequest::REQUESTTYPE_TYPE_Pos);
    const uint8_t         endpointAddr = uint8_t(request.wIndex & 0xff);

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "{}: {} {} {:x}.", __PRETTY_FUNCTION__, int(requestCode), int(requestType), endpointAddr);

    Ptr<USBClassDriverDevice> driver = GetEndpointDriver(endpointAddr);

    if (requestType != USB_RequestType::STANDARD)
    {
        // Forward class request to driver.
        if (driver != nullptr) {
            return InvokeClassDriverControlTransfer(driver, request);
        } else {
            return false;
        }
    }
    else
    {
        switch (requestCode)
        {
            case USB_RequestCode::GET_STATUS:
            {
                uint16_t status = 0;
                if (IsEndpointStalled(endpointAddr)) status |= USB_ENDPOINT_STATUS_HALT;
                status = HostToLittleEndian(status);
                m_ControlTransfer.SendControlDataReply(request, &status, sizeof(status));
                break;
            }
            case USB_RequestCode::CLEAR_FEATURE:
            case USB_RequestCode::SET_FEATURE:
            {
                if (USB_RequestFeatureSelector(request.wValue) == USB_RequestFeatureSelector::ENDPOINT_HALT)
                {
                    if (requestCode == USB_RequestCode::CLEAR_FEATURE) {
                        EndpointClearStall(endpointAddr);
                    } else {
                        EndpointSetStall(endpointAddr);
                    }
                }
                if (driver != nullptr)
                {
                    if (!InvokeClassDriverControlTransfer(driver, request))
                    {
                        // STANDARD requests must always be ACKed.
                        m_ControlTransfer.SetControlTransferHandler(ControlTransferHandler::None);
                        m_ControlTransfer.SendControlStatusReply(request);
                    }
                }
                break;
            }
            default:
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::HandleControlRequest(): Unknown endpoint request {}.", int(requestCode));
                return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::HandleSelectConfiguration(uint8_t configNum)
{
    kassert(m_Mutex.IsLocked());

    const uint32_t descriptorIndex = configNum - 1;
    const USB_DescConfiguration* configDesc = GetConfigDescriptor(descriptorIndex);

    if (configDesc == nullptr || configDesc->bDescriptorType != USB_DescriptorType::CONFIGURATION) {
        return false;
    }
    m_RemoteWakeupSupport   = (configDesc->bmAttributes & USB_DescConfiguration::ATTRIBUTES_REMOTE_WAKEUP) != 0;
    m_SelfPowered           = (configDesc->bmAttributes & USB_DescConfiguration::ATTRIBUTES_SELF_POWERED) != 0;

    const void* endDesc = reinterpret_cast<const uint8_t*>(configDesc) + LittleEndianToHost(configDesc->wTotalLength);
   
    for (const USB_DescriptorHeader* desc = configDesc->GetNext(); desc < endDesc; )
    {
        uint32_t associatedInterfaceCount = 0;

        // If there is an Interface Association Descriptor it should come first.
        if (desc->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION)
        {
            const USB_DescInterfaceAssociation* ifAssociationDesc = static_cast<const USB_DescInterfaceAssociation*>(desc);
            associatedInterfaceCount = ifAssociationDesc->bInterfaceCount;

            desc = desc->GetNext(); // Next should be a USB_DescriptorType::INTERFACE.
        }
        if (desc->bDescriptorType != USB_DescriptorType::INTERFACE) {
            return false;
        }

        const USB_DescInterface* interfaceDesc = static_cast<const USB_DescInterface*>(desc);

        // Find a driver that can handle this interface.
        bool driverFound = false;
        for (Ptr<USBClassDriverDevice> driver : m_ClassDrivers)
        {
            const USB_DescriptorHeader* nextDesc = driver->Open(interfaceDesc, endDesc);

            if (nextDesc == nullptr) {
                continue;
            }
            desc = nextDesc;

            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "'{}' opened.", driver->GetName());

            if (associatedInterfaceCount == 0)
            {
                associatedInterfaceCount = driver->GetInterfaceCount();
            }
            // Map interfaces to drivers.
            for (uint32_t i = 0; i < associatedInterfaceCount; ++i)
            {
                const uint8_t interfaceNum = uint8_t(interfaceDesc->bInterfaceNumber + i);

                if (m_InterfaceToDriverMap.find(interfaceNum) != m_InterfaceToDriverMap.end())
                {
                    kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::HandleSelectConfiguration() interface {} already mapped to '{}'.", interfaceNum, m_InterfaceToDriverMap[interfaceNum]->GetName());
                    return false;
                }
                m_InterfaceToDriverMap[interfaceNum] = driver;
            }
            // Map endpoints to driver.
            for (const USB_DescriptorHeader* endpointDesc = interfaceDesc; endpointDesc < endDesc; endpointDesc = endpointDesc->GetNext())
            {
                if (endpointDesc->bDescriptorType == USB_DescriptorType::ENDPOINT)
                {
                    const uint8_t endpointAddr = reinterpret_cast<const USB_DescEndpoint*>(endpointDesc)->bEndpointAddress;
                    auto driverIt = m_EndpointToDriverMap.find(endpointAddr);
                    if (driverIt == m_EndpointToDriverMap.end())
                    {
                        m_EndpointToDriverMap[endpointAddr] = driver;
                    }
                    else
                    {
                        if (driverIt->second != driver)
                        {
                            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBDevice::HandleSelectConfiguration() endpoint {:02x} already mapped to '{}'. Can't map to '{}'.", endpointAddr, driverIt->second->GetName(), driver->GetName());
                            return false;
                        }
                    }
                }
            }
            driverFound = true;
            break;
        }
        if (!driverFound) {
            return false;
        }
    }
    m_SelectedConfigNum = configNum;
    SignalMounted(true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::HandleGetDescriptor(const USB_ControlRequest& request)
{
    kassert(m_Mutex.IsLocked());

    const USB_DescriptorType    descType  = USB_DescriptorType(request.wValue >> 8);
    const size_t                descIndex = request.wValue & 0xff;

    switch (descType)
    {
        case USB_DescriptorType::DEVICE:
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Get descriptor DEVICE.");

            if (m_DeviceDescriptor.bcdDevice == 0) {
                return false;
            }
            // If endpoint0 can't fit the device descriptor in one package, and we have not been given an address, send a truncated package.
            // The minimum enpoint8 size is 8 bytes, so the truncated package will contain USB_DescDevice::bMaxPacketSize0 and inform the host of the situation.
            if (!m_IsAddressed && m_DeviceDescriptor.bLength > m_Endpoint0Size && request.wLength > m_Endpoint0Size)
            {
                // Adjust the request length to prevent a zero-length status reply at the end of the transfer.
                USB_ControlRequest truncatedRequest = request;
                truncatedRequest.wLength = HostToLittleEndian(uint16_t(m_Endpoint0Size));
                return m_ControlTransfer.SendControlDataReply(truncatedRequest, &m_DeviceDescriptor, m_Endpoint0Size);
            }
            else
            {
                return m_ControlTransfer.SendControlDataReply(request, &m_DeviceDescriptor, m_DeviceDescriptor.bLength);
            }
        case USB_DescriptorType::BOS:
        {
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Get descriptor BOS.");
            const USB_DescBOS* desc = GetBOSDescriptor();
            if (desc != nullptr) {
                return m_ControlTransfer.SendControlDataReply(request, const_cast<USB_DescBOS*>(desc), LittleEndianToHost(desc->wTotalLength));
            }
            return false;
        }
        case USB_DescriptorType::CONFIGURATION:
        case USB_DescriptorType::OTHER_SPEED_CONFIGURATION:
        {
            const USB_DescConfiguration* desc = (descType == USB_DescriptorType::CONFIGURATION) ? GetConfigDescriptor(descIndex) : GetOtherConfigDescriptor(descIndex);
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Get descriptor {}[{}].", ((descType == USB_DescriptorType::CONFIGURATION) ? "CONFIGURATION" : "OTHER_SPEED_CONFIG"), descIndex);
            if (desc != nullptr) {
                return m_ControlTransfer.SendControlDataReply(request, const_cast<USB_DescConfiguration*>(desc), LittleEndianToHost(desc->wTotalLength));
            }
            return false;
        }
        case USB_DescriptorType::STRING:
            if (descIndex == 0)
            {
                kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Get descriptor STRING[0].");
                std::vector<uint16_t> languages;
                languages.resize(1);
                for (auto i : m_StringDescriptors)
                {
                    languages.push_back(HostToLittleEndian(uint16_t(i.first)));
                }
                USB_DescriptorHeader header;
                header.bLength = uint8_t(languages.size() * sizeof(uint16_t));
                header.bDescriptorType = USB_DescriptorType::STRING;

                static_assert(sizeof(header) == sizeof(uint16_t));
                memcpy(&languages[0], &header, sizeof(header));
                return m_ControlTransfer.SendControlDataReply(request, languages.data(), header.bLength);
            }
            else
            {
                USB_LanguageID languageCode = USB_LanguageID(LittleEndianToHost(request.wIndex));
                kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Get descriptor STRING[{:04x}][{}].\n", int(languageCode), descIndex);

                auto languageIter = m_StringDescriptors.find(languageCode);
                if (languageIter == m_StringDescriptors.end()) {
                    languageIter = m_StringDescriptors.find(m_DefaultLanguage);
                }
                if (languageIter != m_StringDescriptors.end())
                {
                    std::map<uint32_t, std::vector<uint8_t>>& descMap = languageIter->second;
                    auto descItr = descMap.find(descIndex);
                    if (descItr != descMap.end())
                    {
                        USB_DescString* descString = reinterpret_cast<USB_DescString*>(descItr->second.data());
                        return m_ControlTransfer.SendControlDataReply(request, descString, descString->bLength);
                    }
                }
                return false;
            }
        case USB_DescriptorType::DEVICE_QUALIFIER:
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Get descriptor DEVICE_QUALIFIER.");
            if (m_DeviceQualifier.bcdUSB != 0) { // We use this to detect if a qualifier has been specified.
                return m_ControlTransfer.SendControlDataReply(request, &m_DeviceQualifier, m_DeviceQualifier.bLength);
            }
            return false;
        default:
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Get unknown descriptor {}.", int(descType));
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::InvokeClassDriverControlTransfer(Ptr<USBClassDriverDevice> driver, const USB_ControlRequest& request)
{
    kassert(m_Mutex.IsLocked());

    m_ControlTransfer.SetControlTransferHandler(ControlTransferHandler::ClassDriver, driver);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "Class {} handle control transfer setup.", driver->GetName());
    if (!driver->HandleControlTransfer(USB_ControlStage::SETUP, request))
    {
        m_ControlTransfer.SetControlTransferHandler(ControlTransferHandler::None);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBDevice::PopEvent(USBDeviceEvent& event)
{
    kassert(m_Mutex.IsLocked());
    m_Mutex.Unlock();

    bool result;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        while (m_EventQueue.GetLength() == 0)
        {
            m_EventQueueCondition.IRQWait();
        }
        result = m_EventQueue.Read(&event, 1) == 1;
    } CRITICAL_END;

    m_Mutex.Lock();

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::PushEvent(const USBDeviceEvent& event)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    static volatile uint32_t maxEvents = 0;
    m_EventQueue.Write(&event, 1);
    if (m_EventQueue.GetLength() > maxEvents) {
        maxEvents = m_EventQueue.GetLength();
    }
    m_EventQueueCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::IRQControlRequestReceived(const USB_ControlRequest& request)
{
    USBDeviceEvent event(USBDeviceEventID::ControlRequestReceived);
    event.ControlRequestReceived.Request = request;
    PushEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::IRQTransferComplete(uint8_t endpointAddr, uint32_t length, USB_TransferResult result)
{
    USBDeviceEvent event(USBDeviceEventID::TransferComplete);
    event.TransferComplete.EndpointAddr = endpointAddr;
    event.TransferComplete.Length       = length;
    event.TransferComplete.Result       = result;
    PushEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::IRQBusReset(USB_Speed speed)
{
    USBDeviceEvent event(USBDeviceEventID::BusReset);
    event.BusReset.speed = speed;
    PushEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::IRQSuspend()
{
    const USBDeviceEvent event(USBDeviceEventID::Suspend);
    PushEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::IRQResume()
{
    const USBDeviceEvent event(USBDeviceEventID::Resume);
    PushEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::IRQSessionEnded()
{
    const USBDeviceEvent event(USBDeviceEventID::SessionEnded);
    PushEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDevice::IRQStartOfFrame()
{
    const USBDeviceEvent event(USBDeviceEventID::StartOfFrame);
    PushEvent(event);
}

} // namespace kernel
