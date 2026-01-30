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
// Created: 30.07.2022 23:00

#include <Utils/Utils.h>
#include <Kernel/KLogging.h>
#include <Kernel/USB/USBHostEnumerator.h>
#include <Kernel/USB/USBHost.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::Reset()
{
    SendResult(false, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostEnumerator::Enumerate(USBHostControlRequestCallback&& callback)
{
    USBDeviceNode* device = m_HostHandler->GetDevice(0);
    if (device != nullptr)
    {
        m_ResultCallback = std::move(callback);
        m_HostHandler->GetControlHandler().ReqGetDescriptor(0, USB_RequestRecipient::DEVICE, USB_RequestType::STANDARD, USB_DescriptorType::DEVICE, 0, 0, &device->m_DeviceDesc, sizeof(USB_DescDeviceHeader), p_bind_method(this, &USBHostEnumerator::HandleConfigurationHeaderResult));
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::SendResult(bool result, uint8_t deviceAddr)
{
    if (m_ResultCallback)
    {
        USBHostControlRequestCallback callback = std::move(m_ResultCallback);
        m_ResultCallback = nullptr;
        callback(result, deviceAddr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::HandleConfigurationHeaderResult(bool result, uint8_t deviceAddr)
{
    if (result)
    {
        USBDeviceNode* device = m_HostHandler->GetDevice(deviceAddr);
        if (device != nullptr)
        {
            m_HostHandler->GetControlHandler().UpdatePipes(deviceAddr, device->m_Speed, device->m_DeviceDesc.bMaxPacketSize0);
            m_HostHandler->GetControlHandler().ReqGetDescriptor(deviceAddr, USB_RequestRecipient::DEVICE, USB_RequestType::STANDARD, USB_DescriptorType::DEVICE, 0, 0, &device->m_DeviceDesc, sizeof(USB_DescDevice), p_bind_method(this, &USBHostEnumerator::HandleConfigurationFullResult));
        }
    }
    else
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Get device descriptor request failed.");
        SendResult(result, deviceAddr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::HandleConfigurationFullResult(bool result, uint8_t deviceAddr)
{
    if (result)
    {
        USBDeviceNode* device = m_HostHandler->CreateDeviceNode();
        if (device != nullptr)
        {
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "PID: {:04x}h", int(device->m_DeviceDesc.idProduct));
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "VID: {:04x}h", int(device->m_DeviceDesc.idVendor));

            m_HostHandler->GetControlHandler().ReqSetAddress(device->m_Address, p_bind_method(this, &USBHostEnumerator::HandleSetAddressResult));
        }
        else
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to allocate new device node.");
            SendResult(result, deviceAddr);
        }
    }
    else
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Get full device descriptor request failed.");
        SendResult(result, deviceAddr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::HandleSetAddressResult(bool result, uint8_t deviceAddr)
{
    if (result)
    {
        USBDeviceNode* device = m_HostHandler->GetDevice(deviceAddr);
        if (device != nullptr)
        {
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Address {} assigned.", device->m_Address);

            USBHostControl& control = m_HostHandler->GetControlHandler();
            uint8_t* buffer = control.GetCtrlDataBuffer();

            control.UpdatePipes(device->m_Address, device->m_Speed, device->m_DeviceDesc.bMaxPacketSize0);

            control.ReqGetDescriptor(deviceAddr, USB_RequestRecipient::DEVICE, USB_RequestType::STANDARD, USB_DescriptorType::CONFIGURATION, 0, 0, buffer, sizeof(USB_DescConfiguration),
                [this, buffer, device, &control](bool result, uint8_t deviceAddr)
                {
                    if (result)
                    {
                        const USB_DescConfiguration* configHeader = reinterpret_cast<const USB_DescConfiguration*>(buffer);

                        control.ReqGetDescriptor(deviceAddr, USB_RequestRecipient::DEVICE, USB_RequestType::STANDARD, USB_DescriptorType::CONFIGURATION, 0, 0, buffer, configHeader->wTotalLength,
                            [this, buffer, device](bool result, uint8_t deviceAddr)
                            {
                                if (result)
                                {
                                    const USB_DescConfiguration* config = reinterpret_cast<const USB_DescConfiguration*>(buffer);

                                    device->m_SelectedConfiguration = config->bConfigurationValue;
                                    device->m_SupportRemoteWakeup   = (config->bmAttributes & USB_DescConfiguration::ATTRIBUTES_REMOTE_WAKEUP) != 0;
                                    device->m_SelfPowered           = (config->bmAttributes & USB_DescConfiguration::ATTRIBUTES_SELF_POWERED) != 0;

                                    m_HostHandler->ConfigureDevice(config, deviceAddr);
                                    GetStringDescriptors(deviceAddr);
                                }
                                else
                                {
                                    kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to get full configuration descriptor.");
                                    SendResult(result, deviceAddr);
                                }

                            }
                        );
                    }
                    else
                    {
                        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to get configuration descriptor header.");
                        SendResult(result, deviceAddr);
                    }
                }
            );
        }
    }
    else
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to set device address.");
        SendResult(result, deviceAddr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::GetStringDescriptors(uint8_t deviceAddr)
{
    USBDeviceNode* device = m_HostHandler->GetDevice(deviceAddr);
    if (device != nullptr)
    {
        if (device->m_DeviceDesc.iManufacturer != 0) {
            m_HostHandler->GetControlHandler().ReqGetStringDescriptor(deviceAddr, device->m_DeviceDesc.iManufacturer, device->m_ManufacturerString, p_bind_method(this, &USBHostEnumerator::HandleGetManufacturerStringResult));
        } else {
            HandleGetManufacturerStringResult(false, deviceAddr);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::HandleGetManufacturerStringResult(bool result, uint8_t deviceAddr)
{
    USBDeviceNode* device = m_HostHandler->GetDevice(deviceAddr);
    if (device != nullptr)
    {
        if (device->m_DeviceDesc.iProduct != 0) {
            m_HostHandler->GetControlHandler().ReqGetStringDescriptor(deviceAddr, device->m_DeviceDesc.iProduct, device->m_ProductString, p_bind_method(this, &USBHostEnumerator::HandleGetProductStringResult));
        } else {
            HandleGetProductStringResult(false, deviceAddr);
        }
    }
    else
    {
        SendResult(false, deviceAddr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::HandleGetProductStringResult(bool result, uint8_t deviceAddr)
{
    USBDeviceNode* device = m_HostHandler->GetDevice(deviceAddr);
    if (device != nullptr)
    {
        if (device->m_DeviceDesc.iSerialNumber != 0) {
            m_HostHandler->GetControlHandler().ReqGetStringDescriptor(deviceAddr, device->m_DeviceDesc.iSerialNumber, device->m_SerialNumberString, p_bind_method(this, &USBHostEnumerator::HandleGetSerialNumberResult));
        } else {
            HandleGetSerialNumberResult(false, deviceAddr);
        }
    }
    else
    {
        SendResult(false, deviceAddr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostEnumerator::HandleGetSerialNumberResult(bool result, uint8_t deviceAddr)
{
    SendResult(true, deviceAddr);
}


} // namespace kernel
