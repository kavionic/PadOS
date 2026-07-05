// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 03.07.2026 18:30

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/pados_types.h>

#include <argparse/argparse.hpp>

#include <DeviceControl/USB.h>
#include <System/AppDefinition.h>
#include <System/Endian.h>
#include <System/ExceptionHandling.h>
#include <Utils/String.h>

namespace shutil_ls_usb
{

static constexpr const char* LSUSB_TOPOLOGY_ROOT = "/dev/usb/topology";

struct NameValue
{
    PString Name;
    PString Value;
};

class CmdLSUSB
{
public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    int Invoke(int argc, char* argv[])
    {
        argparse::ArgumentParser program(argv[0], "1.0", argparse::default_arguments::none);

        program.add_argument("-h", "--help")
            .help("Print argument help.")
            .flag();

        program.add_argument("-c", "--compact")
            .help("Print compact one-line summaries.")
            .flag();

        program.add_argument("-n", "--no-descriptors")
            .help("Do not print interface descriptor blocks.")
            .flag();

        try
        {
            program.parse_args(argc, argv);
        }
        catch (const std::exception& exception)
        {
            Print("{}\n", exception.what());
            Print("{}", program.help().str());
            return 1;
        }

        if (program.get<bool>("--help"))
        {
            Print("{}", program.help().str());
            return 0;
        }

        m_CompactOutput = program.get<bool>("--compact");
        m_ShowDescriptorBlocks = !program.get<bool>("--no-descriptors");

        PrintTopology();
        return m_HadError ? 1 : 0;
    }

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintTopology()
    {
        std::vector<PString> busNames = GetSortedPrefixedNames(LSUSB_TOPOLOGY_ROOT, "bus", true);

        if (busNames.empty() && !m_HadError) {
            Print("No USB devices found under {}.\n", LSUSB_TOPOLOGY_ROOT);
        }

        if (m_CompactOutput)
        {
            for (const PString& busName : busNames)
            {
                PrintTopologyNodeCompact(PString(LSUSB_TOPOLOGY_ROOT) + "/" + busName, 0);
            }
        }
        else
        {
            std::vector<bool> treeBranches;
            for (size_t busIndex = 0; busIndex < busNames.size(); ++busIndex)
            {
                const bool isLastBus = busIndex + 1 == busNames.size();
                PrintTopologyNodeTree(PString(LSUSB_TOPOLOGY_ROOT) + "/" + busNames[busIndex], treeBranches, isLastBus);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintTopologyNodeTree(const PString& topologyPath, const std::vector<bool>& treeBranches, bool isLastNode)
    {
        const PString nodeName = GetBaseName(topologyPath);
        const PString deviceLinkPath = topologyPath + "/device";
        const PString devicePath = ReadOptionalSymlink(deviceLinkPath);
        const std::vector<PString> portNames = GetSortedPrefixedNames(topologyPath, "port", true);

        PrintTreeLine(treeBranches, isLastNode, nodeName);

        std::vector<bool> childBranches = treeBranches;
        childBranches.push_back(!isLastNode);

        const bool hasDevice = !devicePath.empty();
        const size_t childCount = (hasDevice ? 1 : 1) + portNames.size();
        size_t childIndex = 0;

        if (hasDevice)
        {
            PrintDeviceTree(devicePath, childBranches, ++childIndex == childCount);
        }
        else
        {
            PrintTreeProperty(childBranches, ++childIndex == childCount, "device", "none");
        }

        for (const PString& portName : portNames)
        {
            PrintTopologyNodeTree(topologyPath + "/" + portName, childBranches, ++childIndex == childCount);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintDeviceTree(const PString& devicePath, const std::vector<bool>& treeBranches, bool isLastDevice)
    {
        const PString controlPath = devicePath + "/control";
        const int controlHandle = open(controlPath.c_str(), O_RDONLY);

        if (controlHandle == -1)
        {
            PrintTreeProperty(treeBranches, isLastDevice, "openError", PString::format_string("{}: {}", controlPath, strerror(errno)));
            m_HadError = true;
            return;
        }
        PScopeExit closeControlHandle([controlHandle]()
            {
                close(controlHandle);
            }
        );

        try
        {
            PUSBDeviceControl deviceControl(controlHandle);
            PUSBDeviceInfo deviceInfo;

            deviceControl.GetDeviceInfo(&deviceInfo);

            const PString manufacturer = ReadDeviceString(deviceControl, PUSBDeviceStringID::Manufacturer);
            const PString product = ReadDeviceString(deviceControl, PUSBDeviceStringID::Product);
            const PString serialNumber = ReadDeviceString(deviceControl, PUSBDeviceStringID::SerialNumber);
            const std::vector<PString> interfaceNames = GetSortedPrefixedNames(devicePath, "interface", false);

            PrintTreeLine(treeBranches, isLastDevice, "device");

            std::vector<bool> childBranches = treeBranches;
            childBranches.push_back(!isLastDevice);

            const std::vector<NameValue> values = MakeDeviceValues(devicePath, deviceInfo, manufacturer, product, serialNumber);
            const bool hasHubInfo = deviceInfo.IsHub;
            const size_t childCount = values.size() + 1 + (hasHubInfo ? 1 : 0) + interfaceNames.size();
            size_t childIndex = 0;

            for (const NameValue& value : values)
            {
                PrintTreeProperty(childBranches, ++childIndex == childCount, value.Name, value.Value);
            }

            if (hasHubInfo) {
                PrintHubInfoTree(deviceInfo, childBranches, ++childIndex == childCount);
            }

            PrintConfigurationInfoTree(deviceInfo, childBranches, ++childIndex == childCount);

            for (const PString& interfaceName : interfaceNames)
            {
                PrintInterfaceTree(devicePath + "/" + interfaceName, childBranches, ++childIndex == childCount);
            }
        }
        catch (const std::exception& exception)
        {
            PrintTreeProperty(treeBranches, isLastDevice, "queryError", PString::format_string("{}: {}", controlPath, exception.what()));
            m_HadError = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintHubInfoTree(const PUSBDeviceInfo& deviceInfo, const std::vector<bool>& treeBranches, bool isLastHubInfo)
    {
        PrintTreeLine(treeBranches, isLastHubInfo, "hub");

        std::vector<bool> childBranches = treeBranches;
        childBranches.push_back(!isLastHubInfo);

        PrintTreeProperty(childBranches, false, "portCount", PString::format_string("{}", static_cast<uint32_t>(deviceInfo.HubPortCount)));
        PrintTreeProperty(childBranches, true, "powerOnDelayMS", PString::format_string("{}", static_cast<uint32_t>(deviceInfo.HubPowerOnDelayMS)));
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintConfigurationInfoTree(const PUSBDeviceInfo& deviceInfo, const std::vector<bool>& treeBranches, bool isLastConfiguration)
    {
        PrintTreeLine(treeBranches, isLastConfiguration, "configuration");

        std::vector<bool> childBranches = treeBranches;
        childBranches.push_back(!isLastConfiguration);

        if (deviceInfo.HasConfigurationDescriptor)
        {
            const USB_DescConfiguration& configurationDescriptor = deviceInfo.ConfigurationDescriptor;
            const std::vector<NameValue> values =
            {
                { "present", "yes" },
                { "value", PString::format_string("{}", static_cast<uint32_t>(configurationDescriptor.bConfigurationValue)) },
                { "interfaceCount", PString::format_string("{}", static_cast<uint32_t>(configurationDescriptor.bNumInterfaces)) },
                { "totalLength", PString::format_string("{}", static_cast<uint32_t>(PLittleEndianToHost(configurationDescriptor.wTotalLength))) },
                { "cachedLength", PString::format_string("{}", deviceInfo.ConfigurationDescriptorSize) },
                { "attributes", FormatConfigurationAttributes(configurationDescriptor.bmAttributes) },
                { "maxPowerMA", PString::format_string("{}", static_cast<uint32_t>(configurationDescriptor.bMaxPower) * 2) },
                { "stringIndex", PString::format_string("{}", static_cast<uint32_t>(configurationDescriptor.iConfiguration)) }
            };

            PrintTreeValues(childBranches, values);
        }
        else
        {
            PrintTreeProperty(childBranches, true, "present", "no");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintInterfaceTree(const PString& interfacePath, const std::vector<bool>& treeBranches, bool isLastInterface)
    {
        const int interfaceHandle = open(interfacePath.c_str(), O_RDONLY);

        if (interfaceHandle == -1)
        {
            PrintTreeProperty(treeBranches, isLastInterface, "openError", PString::format_string("{}: {}", interfacePath, strerror(errno)));
            m_HadError = true;
            return;
        }
        PScopeExit closeInterfaceHandle([interfaceHandle]()
            {
                close(interfaceHandle);
            }
        );

        try
        {
            PUSBDeviceInterface deviceInterface(interfaceHandle);
            PUSBDeviceInterfaceInfo interfaceInfo;

            deviceInterface.GetInterfaceInfo(&interfaceInfo);

            PrintTreeLine(treeBranches, isLastInterface, PString::format_string("interface{}", static_cast<uint32_t>(interfaceInfo.InterfaceNumber)));

            std::vector<bool> childBranches = treeBranches;
            childBranches.push_back(!isLastInterface);

            const std::vector<NameValue> values = MakeInterfaceValues(interfaceInfo);
            const size_t endpointCount = std::min<size_t>(interfaceInfo.EndpointCount, PUSB_MAX_ENDPOINTS_PER_INTERFACE);
            const size_t descriptorCount = m_ShowDescriptorBlocks ? 1 : 0;
            const size_t childCount = values.size() + endpointCount + descriptorCount;
            size_t childIndex = 0;

            for (const NameValue& value : values)
            {
                PrintTreeProperty(childBranches, ++childIndex == childCount, value.Name, value.Value);
            }

            for (size_t endpointIndex = 0; endpointIndex < endpointCount; ++endpointIndex)
            {
                PrintEndpointInfoTree(interfaceInfo.Endpoints[endpointIndex], childBranches, ++childIndex == childCount);
            }

            if (m_ShowDescriptorBlocks) {
                PrintInterfaceDescriptorBlockTree(deviceInterface, interfaceInfo.DescriptorSize, childBranches, ++childIndex == childCount);
            }
        }
        catch (const std::exception& exception)
        {
            PrintTreeProperty(treeBranches, isLastInterface, "queryError", PString::format_string("{}: {}", interfacePath, exception.what()));
            m_HadError = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintEndpointInfoTree(const USB_DescEndpoint& endpointDescriptor, const std::vector<bool>& treeBranches, bool isLastEndpoint)
    {
        PrintTreeLine(treeBranches, isLastEndpoint, PString::format_string("endpoint0x{:02x}", static_cast<uint32_t>(endpointDescriptor.bEndpointAddress)));

        std::vector<bool> childBranches = treeBranches;
        childBranches.push_back(!isLastEndpoint);

        const std::vector<NameValue> values =
        {
            { "address", PString::format_string("0x{:02x}", static_cast<uint32_t>(endpointDescriptor.bEndpointAddress)) },
            { "direction", GetEndpointDirectionName(endpointDescriptor) },
            { "transferType", GetTransferTypeName(endpointDescriptor.GetTransferType()) },
            { "maxPacketSize", PString::format_string("{}", static_cast<uint32_t>(endpointDescriptor.GetMaxPacketSize())) },
            { "extraTransactions", PString::format_string("{}", static_cast<uint32_t>(endpointDescriptor.GetExtraTransactions())) },
            { "interval", PString::format_string("{}", static_cast<uint32_t>(endpointDescriptor.bInterval)) },
            { "sync", GetEndpointSyncName(endpointDescriptor.GetSyncType()) },
            { "usage", GetEndpointUsageName(endpointDescriptor.GetUsageType()) }
        };

        PrintTreeValues(childBranches, values);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintInterfaceDescriptorBlockTree(const PUSBDeviceInterface& deviceInterface, size_t descriptorSize, const std::vector<bool>& treeBranches, bool isLastDescriptorBlock)
    {
        if (descriptorSize == 0)
        {
            PrintTreeProperty(treeBranches, isLastDescriptorBlock, "descriptors", "none");
            return;
        }

        std::vector<uint8_t> descriptorData(descriptorSize);
        const size_t bytesRead = deviceInterface.ReadDescriptor(0, descriptorData.data(), descriptorData.size());
        descriptorData.resize(bytesRead);

        PrintTreeLine(treeBranches, isLastDescriptorBlock, "descriptors");

        std::vector<bool> childBranches = treeBranches;
        childBranches.push_back(!isLastDescriptorBlock);

        if (descriptorData.empty())
        {
            PrintTreeProperty(childBranches, true, "size", "0");
            return;
        }

        const uint8_t* descriptorEnd = descriptorData.data() + descriptorData.size();
        const USB_DescriptorHeader* descriptor = reinterpret_cast<const USB_DescriptorHeader*>(descriptorData.data());
        std::vector<const USB_DescriptorHeader*> descriptors;

        for (; descriptor->ValidateLength(descriptorEnd); descriptor = descriptor->GetNext())
        {
            descriptors.push_back(descriptor);
        }

        const uint8_t* nextDescriptor = reinterpret_cast<const uint8_t*>(descriptor);
        const bool hasMalformedTail = nextDescriptor != descriptorEnd;
        const size_t childCount = 1 + descriptors.size() + (hasMalformedTail ? 1 : 0);
        size_t childIndex = 0;

        PrintTreeProperty(childBranches, ++childIndex == childCount, "size", PString::format_string("{}", descriptorData.size()));

        for (const USB_DescriptorHeader* descriptorHeader : descriptors)
        {
            PrintDescriptorTree(descriptorHeader, childBranches, ++childIndex == childCount);
        }

        if (hasMalformedTail) {
            PrintTreeProperty(childBranches, ++childIndex == childCount, "malformedTrailingBytes", PString::format_string("{}", size_t(descriptorEnd - nextDescriptor)));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintDescriptorTree(const USB_DescriptorHeader* descriptor, const std::vector<bool>& treeBranches, bool isLastDescriptor)
    {
        PrintTreeLine(treeBranches, isLastDescriptor, GetDescriptorTypeName(descriptor->bDescriptorType));

        std::vector<bool> childBranches = treeBranches;
        childBranches.push_back(!isLastDescriptor);

        std::vector<NameValue> values =
        {
            { "length", PString::format_string("{}", static_cast<uint32_t>(descriptor->bLength)) },
            { "type", PString::format_string("0x{:02x}", static_cast<uint32_t>(std::to_underlying(descriptor->bDescriptorType))) }
        };

        std::vector<NameValue> descriptorValues = MakeDescriptorValues(descriptor);
        values.insert(values.end(), descriptorValues.begin(), descriptorValues.end());
        PrintTreeValues(childBranches, values);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintTopologyNodeCompact(const PString& topologyPath, size_t depth)
    {
        const PString nodeName = GetBaseName(topologyPath);
        const PString deviceLinkPath = topologyPath + "/device";
        const PString devicePath = ReadOptionalSymlink(deviceLinkPath);

        if (!devicePath.empty())
        {
            PrintDeviceCompact(nodeName, devicePath, depth);
        }
        else
        {
            Print("{}{}: no device\n", MakeIndent(depth), nodeName);
        }

        const std::vector<PString> portNames = GetSortedPrefixedNames(topologyPath, "port", true);
        for (const PString& portName : portNames)
        {
            PrintTopologyNodeCompact(topologyPath + "/" + portName, depth + 1);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintDeviceCompact(const PString& nodeName, const PString& devicePath, size_t depth)
    {
        const PString controlPath = devicePath + "/control";
        const int controlHandle = open(controlPath.c_str(), O_RDONLY);

        if (controlHandle == -1)
        {
            Print("{}{}: failed to open {}: {}\n", MakeIndent(depth), nodeName, controlPath, strerror(errno));
            m_HadError = true;
            return;
        }
        PScopeExit closeControlHandle([controlHandle]()
            {
                close(controlHandle);
            }
        );

        try
        {
            PUSBDeviceControl deviceControl(controlHandle);
            PUSBDeviceInfo deviceInfo;

            deviceControl.GetDeviceInfo(&deviceInfo);

            const PString manufacturer = ReadDeviceString(deviceControl, PUSBDeviceStringID::Manufacturer);
            const PString product = ReadDeviceString(deviceControl, PUSBDeviceStringID::Product);
            const PString serialNumber = ReadDeviceString(deviceControl, PUSBDeviceStringID::SerialNumber);

            PrintDeviceInfoCompact(nodeName, devicePath, deviceInfo, manufacturer, product, serialNumber, depth);
            PrintInterfaceListCompact(devicePath, depth + 1);
        }
        catch (const std::exception& exception)
        {
            Print("{}{}: failed to query {}: {}\n", MakeIndent(depth), nodeName, controlPath, exception.what());
            m_HadError = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintDeviceInfoCompact(const PString& nodeName, const PString& devicePath, const PUSBDeviceInfo& deviceInfo, const PString& manufacturer, const PString& product, const PString& serialNumber, size_t depth)
    {
        const USB_DescDevice& deviceDescriptor = deviceInfo.DeviceDescriptor;
        const uint16_t vendorID = PLittleEndianToHost(deviceDescriptor.idVendor);
        const uint16_t productID = PLittleEndianToHost(deviceDescriptor.idProduct);
        const PString deviceName = FormatDeviceName(manufacturer, product);
        const PString serialText = serialNumber.empty() ? PString("none") : serialNumber;

        Print("{}{}: dev{} {:04x}:{:04x} {} speed={} state={} name=\"{}\" serial=\"{}\" node={}\n",
            MakeIndent(depth),
            nodeName,
            static_cast<uint32_t>(deviceInfo.DeviceAddress),
            static_cast<uint32_t>(vendorID),
            static_cast<uint32_t>(productID),
            deviceInfo.IsHub ? "hub" : "device",
            GetSpeedName(deviceInfo.Speed),
            FormatDeviceState(deviceInfo),
            deviceName,
            serialText,
            devicePath
        );

        Print("{}  device: usb={} class={} subclass=0x{:02x} protocol=0x{:02x} maxPacket0={} configs={} bcdDevice={} manufacturerIndex={} productIndex={} serialIndex={}\n",
            MakeIndent(depth),
            FormatBCD(deviceDescriptor.bcdUSB),
            FormatClassCode(deviceDescriptor.bDeviceClass),
            static_cast<uint32_t>(deviceDescriptor.bDeviceSubClass),
            static_cast<uint32_t>(deviceDescriptor.bDeviceProtocol),
            static_cast<uint32_t>(deviceDescriptor.bMaxPacketSize0),
            static_cast<uint32_t>(deviceDescriptor.bNumConfigurations),
            FormatBCD(deviceDescriptor.bcdDevice),
            static_cast<uint32_t>(deviceDescriptor.iManufacturer),
            static_cast<uint32_t>(deviceDescriptor.iProduct),
            static_cast<uint32_t>(deviceDescriptor.iSerialNumber)
        );

        Print("{}  links: parentHub={} parentPort={} selectedConfig={} remoteWakeup={} selfPowered={} descriptorSize={} configDescriptorSize={}\n",
            MakeIndent(depth),
            static_cast<uint32_t>(deviceInfo.ParentHubAddress),
            static_cast<uint32_t>(deviceInfo.ParentHubPort),
            static_cast<uint32_t>(deviceInfo.SelectedConfiguration),
            FormatBool(deviceInfo.SupportsRemoteWakeup),
            FormatBool(deviceInfo.SelfPowered),
            sizeof(USB_DescDevice),
            deviceInfo.ConfigurationDescriptorSize
        );

        if (deviceInfo.IsHub)
        {
            Print("{}  hub: ports={} powerOnDelay={}ms\n",
                MakeIndent(depth),
                static_cast<uint32_t>(deviceInfo.HubPortCount),
                static_cast<uint32_t>(deviceInfo.HubPowerOnDelayMS)
            );
        }

        PrintConfigurationInfoCompact(deviceInfo, depth);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintConfigurationInfoCompact(const PUSBDeviceInfo& deviceInfo, size_t depth)
    {
        if (deviceInfo.HasConfigurationDescriptor)
        {
            const USB_DescConfiguration& configurationDescriptor = deviceInfo.ConfigurationDescriptor;
            const uint16_t totalLength = PLittleEndianToHost(configurationDescriptor.wTotalLength);

            Print("{}  config: value={} interfaces={} totalLength={} cachedLength={} attributes={} maxPower={}mA stringIndex={}\n",
                MakeIndent(depth),
                static_cast<uint32_t>(configurationDescriptor.bConfigurationValue),
                static_cast<uint32_t>(configurationDescriptor.bNumInterfaces),
                static_cast<uint32_t>(totalLength),
                deviceInfo.ConfigurationDescriptorSize,
                FormatConfigurationAttributes(configurationDescriptor.bmAttributes),
                static_cast<uint32_t>(configurationDescriptor.bMaxPower) * 2,
                static_cast<uint32_t>(configurationDescriptor.iConfiguration)
            );
        }
        else
        {
            Print("{}  config: none\n", MakeIndent(depth));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintInterfaceListCompact(const PString& devicePath, size_t depth)
    {
        const std::vector<PString> interfaceNames = GetSortedPrefixedNames(devicePath, "interface", false);

        for (const PString& interfaceName : interfaceNames)
        {
            PrintInterfaceCompact(devicePath + "/" + interfaceName, depth);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintInterfaceCompact(const PString& interfacePath, size_t depth)
    {
        const int interfaceHandle = open(interfacePath.c_str(), O_RDONLY);

        if (interfaceHandle == -1)
        {
            Print("{}{}: failed to open: {}\n", MakeIndent(depth), GetBaseName(interfacePath), strerror(errno));
            m_HadError = true;
            return;
        }
        PScopeExit closeInterfaceHandle([interfaceHandle]()
            {
                close(interfaceHandle);
            }
        );

        try
        {
            PUSBDeviceInterface deviceInterface(interfaceHandle);
            PUSBDeviceInterfaceInfo interfaceInfo;

            deviceInterface.GetInterfaceInfo(&interfaceInfo);
            PrintInterfaceInfoCompact(interfaceInfo, depth);

            if (m_ShowDescriptorBlocks) {
                PrintInterfaceDescriptorBlockCompact(deviceInterface, interfaceInfo.DescriptorSize, depth + 1);
            }
        }
        catch (const std::exception& exception)
        {
            Print("{}{}: failed to query: {}\n", MakeIndent(depth), GetBaseName(interfacePath), exception.what());
            m_HadError = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintInterfaceInfoCompact(const PUSBDeviceInterfaceInfo& interfaceInfo, size_t depth)
    {
        const USB_DescInterface& interfaceDescriptor = interfaceInfo.InterfaceDescriptor;
        const size_t endpointCount = std::min<size_t>(interfaceInfo.EndpointCount, PUSB_MAX_ENDPOINTS_PER_INTERFACE);

        Print("{}interface{}: {} subclass=0x{:02x} protocol=0x{:02x} alt={} settings={} endpoints={} declaredEndpoints={} descOffset={} descSize={} stringIndex={}\n",
            MakeIndent(depth),
            static_cast<uint32_t>(interfaceInfo.InterfaceNumber),
            FormatClassCode(interfaceDescriptor.bInterfaceClass),
            static_cast<uint32_t>(interfaceDescriptor.bInterfaceSubClass),
            static_cast<uint32_t>(interfaceDescriptor.bInterfaceProtocol),
            static_cast<uint32_t>(interfaceDescriptor.bAlternateSetting),
            static_cast<uint32_t>(interfaceInfo.AlternateSettingCount),
            static_cast<uint32_t>(interfaceInfo.EndpointCount),
            static_cast<uint32_t>(interfaceDescriptor.bNumEndpoints),
            interfaceInfo.DescriptorOffset,
            interfaceInfo.DescriptorSize,
            static_cast<uint32_t>(interfaceDescriptor.iInterface)
        );

        for (size_t endpointIndex = 0; endpointIndex < endpointCount; ++endpointIndex)
        {
            PrintEndpointInfoCompact(interfaceInfo.Endpoints[endpointIndex], depth + 1);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintEndpointInfoCompact(const USB_DescEndpoint& endpointDescriptor, size_t depth)
    {
        Print("{}endpoint 0x{:02x}: {} {} maxPacket={} extraTransactions={} interval={} sync={} usage={}\n",
            MakeIndent(depth),
            static_cast<uint32_t>(endpointDescriptor.bEndpointAddress),
            GetEndpointDirectionName(endpointDescriptor),
            GetTransferTypeName(endpointDescriptor.GetTransferType()),
            static_cast<uint32_t>(endpointDescriptor.GetMaxPacketSize()),
            static_cast<uint32_t>(endpointDescriptor.GetExtraTransactions()),
            static_cast<uint32_t>(endpointDescriptor.bInterval),
            GetEndpointSyncName(endpointDescriptor.GetSyncType()),
            GetEndpointUsageName(endpointDescriptor.GetUsageType())
        );
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintInterfaceDescriptorBlockCompact(const PUSBDeviceInterface& deviceInterface, size_t descriptorSize, size_t depth)
    {
        if (descriptorSize == 0) {
            return;
        }

        std::vector<uint8_t> descriptorData(descriptorSize);
        const size_t bytesRead = deviceInterface.ReadDescriptor(0, descriptorData.data(), descriptorData.size());
        descriptorData.resize(bytesRead);

        if (descriptorData.empty()) {
            return;
        }

        Print("{}descriptors: {} bytes\n", MakeIndent(depth), descriptorData.size());

        const uint8_t* descriptorEnd = descriptorData.data() + descriptorData.size();
        const USB_DescriptorHeader* descriptor = reinterpret_cast<const USB_DescriptorHeader*>(descriptorData.data());

        for (; descriptor->ValidateLength(descriptorEnd); descriptor = descriptor->GetNext())
        {
            Print("{}  {}: len={} type=0x{:02x}{}\n",
                MakeIndent(depth),
                GetDescriptorTypeName(descriptor->bDescriptorType),
                static_cast<uint32_t>(descriptor->bLength),
                static_cast<uint32_t>(std::to_underlying(descriptor->bDescriptorType)),
                FormatDescriptorDetails(descriptor)
            );
        }

        const uint8_t* nextDescriptor = reinterpret_cast<const uint8_t*>(descriptor);
        if (nextDescriptor != descriptorEnd)
        {
            Print("{}  malformed trailing bytes: {}\n", MakeIndent(depth), size_t(descriptorEnd - nextDescriptor));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    PString ReadDeviceString(const PUSBDeviceControl& deviceControl, PUSBDeviceStringID stringID) const
    {
        const size_t stringLength = deviceControl.GetStringLength(stringID);
        if (stringLength == 0) {
            return PString();
        }

        std::vector<char> stringBuffer(stringLength + 1);
        const size_t bytesRead = deviceControl.ReadString(stringID, stringBuffer.data(), stringBuffer.size());
        return PString(stringBuffer.data(), bytesRead);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    std::vector<PString> GetSortedPrefixedNames(const PString& directoryPath, const char* prefix, bool directoriesOnly)
    {
        std::vector<PString> names;
        const int directoryHandle = open(directoryPath.c_str(), O_RDONLY);

        if (directoryHandle == -1)
        {
            Print("ls_usb: failed to open {}: {}\n", directoryPath, strerror(errno));
            m_HadError = true;
            return names;
        }
        PScopeExit closeDirectoryHandle([directoryHandle]()
            {
                close(directoryHandle);
            }
        );

        dirent_t directoryEntry;
        for (;;)
        {
            const ssize_t readResult = posix_getdents(directoryHandle, &directoryEntry, sizeof(directoryEntry), 0);

            if (readResult == 0) {
                break;
            }
            if (readResult != static_cast<ssize_t>(sizeof(directoryEntry)))
            {
                if (readResult < 0)
                {
                    Print("ls_usb: failed to read {}: {}\n", directoryPath, strerror(errno));
                    m_HadError = true;
                }
                break;
            }

            if (PString::is_dot_or_dot_dot(directoryEntry.d_name, directoryEntry.d_namlen)) {
                continue;
            }

            const PString entryName(directoryEntry.d_name, directoryEntry.d_namlen);
            if (!HasPrefixedNumber(entryName, prefix)) {
                continue;
            }
            if (directoriesOnly && !IsDirectoryEntry(directoryHandle, directoryPath, entryName)) {
                continue;
            }
            names.push_back(entryName);
        }

        std::sort(names.begin(), names.end(), [prefix](const PString& lhs, const PString& rhs)
            {
                return ComparePrefixedNumbers(lhs, rhs, prefix);
            }
        );
        return names;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    bool IsDirectoryEntry(int directoryHandle, const PString& directoryPath, const PString& entryName)
    {
        const int entryHandle = openat(directoryHandle, entryName.c_str(), O_PATH | O_NOFOLLOW);
        if (entryHandle == -1)
        {
            Print("ls_usb: failed to open directory entry {}/{}: {}\n", directoryPath, entryName, strerror(errno));
            m_HadError = true;
            return false;
        }
        PScopeExit closeEntryHandle([entryHandle]()
            {
                close(entryHandle);
            }
        );

        stat_t statBuffer;
        if (fstat(entryHandle, &statBuffer) != 0)
        {
            Print("ls_usb: failed to stat directory entry {}/{}: {}\n", directoryPath, entryName, strerror(errno));
            m_HadError = true;
            return false;
        }
        return S_ISDIR(statBuffer.st_mode);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    PString ReadOptionalSymlink(const PString& path)
    {
        const int linkHandle = open(path.c_str(), O_PATH | O_NOFOLLOW);

        if (linkHandle == -1)
        {
            if (errno != ENOENT)
            {
                Print("ls_usb: failed to open {}: {}\n", path, strerror(errno));
                m_HadError = true;
            }
            return PString();
        }
        PScopeExit closeLinkHandle([linkHandle]()
            {
                close(linkHandle);
            }
        );

        stat_t statBuffer;
        if (fstat(linkHandle, &statBuffer) != 0)
        {
            Print("ls_usb: failed to stat {}: {}\n", path, strerror(errno));
            m_HadError = true;
            return PString();
        }

        size_t targetBufferSize = 256;
        if (statBuffer.st_size > 0) {
            targetBufferSize = std::max<size_t>(targetBufferSize, size_t(statBuffer.st_size));
        }

        PString targetPath;
        targetPath.resize(targetBufferSize);

        const ssize_t bytesRead = readlinkat(linkHandle, "", targetPath.data(), targetPath.size());
        if (bytesRead < 0)
        {
            Print("ls_usb: failed to read {}: {}\n", path, strerror(errno));
            m_HadError = true;
            return PString();
        }

        targetPath.resize(static_cast<size_t>(bytesRead));
        return targetPath;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static std::vector<NameValue> MakeDeviceValues(const PString& devicePath, const PUSBDeviceInfo& deviceInfo, const PString& manufacturer, const PString& product, const PString& serialNumber)
    {
        const USB_DescDevice& deviceDescriptor = deviceInfo.DeviceDescriptor;
        const uint16_t vendorID = PLittleEndianToHost(deviceDescriptor.idVendor);
        const uint16_t productID = PLittleEndianToHost(deviceDescriptor.idProduct);

        return
        {
            { "node", devicePath },
            { "address", PString::format_string("{}", static_cast<uint32_t>(deviceInfo.DeviceAddress)) },
            { "type", deviceInfo.IsHub ? "hub" : "device" },
            { "vendorID", PString::format_string("0x{:04x}", static_cast<uint32_t>(vendorID)) },
            { "productID", PString::format_string("0x{:04x}", static_cast<uint32_t>(productID)) },
            { "name", QuoteString(FormatDeviceName(manufacturer, product)) },
            { "manufacturer", QuoteString(manufacturer) },
            { "product", QuoteString(product) },
            { "serialNumber", serialNumber.empty() ? PString("none") : QuoteString(serialNumber) },
            { "speed", GetSpeedName(deviceInfo.Speed) },
            { "state", FormatDeviceState(deviceInfo) },
            { "parentHub", PString::format_string("{}", static_cast<uint32_t>(deviceInfo.ParentHubAddress)) },
            { "parentPort", PString::format_string("{}", static_cast<uint32_t>(deviceInfo.ParentHubPort)) },
            { "selectedConfiguration", PString::format_string("{}", static_cast<uint32_t>(deviceInfo.SelectedConfiguration)) },
            { "remoteWakeup", FormatBool(deviceInfo.SupportsRemoteWakeup) },
            { "selfPowered", FormatBool(deviceInfo.SelfPowered) },
            { "deviceDescriptorSize", PString::format_string("{}", sizeof(USB_DescDevice)) },
            { "configurationDescriptorSize", PString::format_string("{}", deviceInfo.ConfigurationDescriptorSize) },
            { "usbVersion", FormatBCD(deviceDescriptor.bcdUSB) },
            { "deviceClass", FormatClassCode(deviceDescriptor.bDeviceClass) },
            { "deviceSubclass", PString::format_string("0x{:02x}", static_cast<uint32_t>(deviceDescriptor.bDeviceSubClass)) },
            { "deviceProtocol", PString::format_string("0x{:02x}", static_cast<uint32_t>(deviceDescriptor.bDeviceProtocol)) },
            { "maxPacketSize0", PString::format_string("{}", static_cast<uint32_t>(deviceDescriptor.bMaxPacketSize0)) },
            { "configurationCount", PString::format_string("{}", static_cast<uint32_t>(deviceDescriptor.bNumConfigurations)) },
            { "deviceVersion", FormatBCD(deviceDescriptor.bcdDevice) },
            { "manufacturerIndex", PString::format_string("{}", static_cast<uint32_t>(deviceDescriptor.iManufacturer)) },
            { "productIndex", PString::format_string("{}", static_cast<uint32_t>(deviceDescriptor.iProduct)) },
            { "serialIndex", PString::format_string("{}", static_cast<uint32_t>(deviceDescriptor.iSerialNumber)) }
        };
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static std::vector<NameValue> MakeInterfaceValues(const PUSBDeviceInterfaceInfo& interfaceInfo)
    {
        const USB_DescInterface& interfaceDescriptor = interfaceInfo.InterfaceDescriptor;

        return
        {
            { "class", FormatClassCode(interfaceDescriptor.bInterfaceClass) },
            { "subclass", PString::format_string("0x{:02x}", static_cast<uint32_t>(interfaceDescriptor.bInterfaceSubClass)) },
            { "protocol", PString::format_string("0x{:02x}", static_cast<uint32_t>(interfaceDescriptor.bInterfaceProtocol)) },
            { "alternateSetting", PString::format_string("{}", static_cast<uint32_t>(interfaceDescriptor.bAlternateSetting)) },
            { "alternateSettingCount", PString::format_string("{}", static_cast<uint32_t>(interfaceInfo.AlternateSettingCount)) },
            { "endpointCount", PString::format_string("{}", static_cast<uint32_t>(interfaceInfo.EndpointCount)) },
            { "declaredEndpointCount", PString::format_string("{}", static_cast<uint32_t>(interfaceDescriptor.bNumEndpoints)) },
            { "descriptorOffset", PString::format_string("{}", interfaceInfo.DescriptorOffset) },
            { "descriptorSize", PString::format_string("{}", interfaceInfo.DescriptorSize) },
            { "stringIndex", PString::format_string("{}", static_cast<uint32_t>(interfaceDescriptor.iInterface)) }
        };
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static std::vector<NameValue> MakeDescriptorValues(const USB_DescriptorHeader* descriptor)
    {
        switch (descriptor->bDescriptorType)
        {
            case USB_DescriptorType::INTERFACE:
                if (descriptor->bLength >= sizeof(USB_DescInterface))
                {
                    const USB_DescInterface* interfaceDescriptor = reinterpret_cast<const USB_DescInterface*>(descriptor);
                    return
                    {
                        { "number", PString::format_string("{}", static_cast<uint32_t>(interfaceDescriptor->bInterfaceNumber)) },
                        { "alternateSetting", PString::format_string("{}", static_cast<uint32_t>(interfaceDescriptor->bAlternateSetting)) },
                        { "class", FormatClassCode(interfaceDescriptor->bInterfaceClass) },
                        { "subclass", PString::format_string("0x{:02x}", static_cast<uint32_t>(interfaceDescriptor->bInterfaceSubClass)) },
                        { "protocol", PString::format_string("0x{:02x}", static_cast<uint32_t>(interfaceDescriptor->bInterfaceProtocol)) },
                        { "endpointCount", PString::format_string("{}", static_cast<uint32_t>(interfaceDescriptor->bNumEndpoints)) }
                    };
                }
                break;
            case USB_DescriptorType::ENDPOINT:
                if (descriptor->bLength >= sizeof(USB_DescEndpoint))
                {
                    const USB_DescEndpoint* endpointDescriptor = reinterpret_cast<const USB_DescEndpoint*>(descriptor);
                    return
                    {
                        { "address", PString::format_string("0x{:02x}", static_cast<uint32_t>(endpointDescriptor->bEndpointAddress)) },
                        { "direction", GetEndpointDirectionName(*endpointDescriptor) },
                        { "transfer", GetTransferTypeName(endpointDescriptor->GetTransferType()) },
                        { "maxPacketSize", PString::format_string("{}", static_cast<uint32_t>(endpointDescriptor->GetMaxPacketSize())) },
                        { "interval", PString::format_string("{}", static_cast<uint32_t>(endpointDescriptor->bInterval)) }
                    };
                }
                break;
            case USB_DescriptorType::INTERFACE_ASSOCIATION:
                if (descriptor->bLength >= sizeof(USB_DescInterfaceAssociation))
                {
                    const USB_DescInterfaceAssociation* associationDescriptor = reinterpret_cast<const USB_DescInterfaceAssociation*>(descriptor);
                    return
                    {
                        { "firstInterface", PString::format_string("{}", static_cast<uint32_t>(associationDescriptor->bFirstInterface)) },
                        { "interfaceCount", PString::format_string("{}", static_cast<uint32_t>(associationDescriptor->bInterfaceCount)) },
                        { "class", FormatClassCode(associationDescriptor->bFunctionClass) },
                        { "subclass", PString::format_string("0x{:02x}", static_cast<uint32_t>(associationDescriptor->bFunctionSubClass)) },
                        { "protocol", PString::format_string("0x{:02x}", static_cast<uint32_t>(associationDescriptor->bFunctionProtocol)) }
                    };
                }
                break;
            default:
                break;
        }
        return {};
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintTreeValues(const std::vector<bool>& treeBranches, const std::vector<NameValue>& values)
    {
        for (size_t valueIndex = 0; valueIndex < values.size(); ++valueIndex)
        {
            const NameValue& value = values[valueIndex];
            PrintTreeProperty(treeBranches, valueIndex + 1 == values.size(), value.Name, value.Value);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintTreeProperty(const std::vector<bool>& treeBranches, bool isLastValue, const PString& name, const PString& value)
    {
        PrintTreeLine(treeBranches, isLastValue, name + "=" + value);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void PrintTreeLine(const std::vector<bool>& treeBranches, bool isLastLine, const PString& text)
    {
        Print("{}{}{}\n", MakeTreePrefix(treeBranches), isLastLine ? "└── " : "├── ", text);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString MakeTreePrefix(const std::vector<bool>& treeBranches)
    {
        PString prefix;

        for (bool hasMoreSiblings : treeBranches)
        {
            prefix += hasMoreSiblings ? "│   " : "    ";
        }
        return prefix;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static bool HasPrefixedNumber(const PString& name, const char* prefix)
    {
        const size_t prefixLength = strlen(prefix);

        if (name.size() <= prefixLength) {
            return false;
        }
        if (name.compare(0, prefixLength, prefix) != 0) {
            return false;
        }

        for (size_t characterIndex = prefixLength; characterIndex < name.size(); ++characterIndex)
        {
            const char character = name[characterIndex];
            if (character < '0' || character > '9') {
                return false;
            }
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static size_t ParsePrefixedNumber(const PString& name, const char* prefix)
    {
        size_t value = 0;
        const size_t prefixLength = strlen(prefix);

        for (size_t characterIndex = prefixLength; characterIndex < name.size(); ++characterIndex)
        {
            value *= 10;
            value += size_t(name[characterIndex] - '0');
        }
        return value;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static bool ComparePrefixedNumbers(const PString& lhs, const PString& rhs, const char* prefix)
    {
        const size_t lhsNumber = ParsePrefixedNumber(lhs, prefix);
        const size_t rhsNumber = ParsePrefixedNumber(rhs, prefix);

        if (lhsNumber != rhsNumber) {
            return lhsNumber < rhsNumber;
        }
        return lhs < rhs;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString GetBaseName(const PString& path)
    {
        const size_t separatorPosition = path.rfind('/');
        if (separatorPosition == PString::npos) {
            return path;
        }
        return PString(path.data() + separatorPosition + 1, path.size() - separatorPosition - 1);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString MakeIndent(size_t depth)
    {
        return PString(depth * 2, ' ');
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString QuoteString(const PString& value)
    {
        return PString("\"") + value + "\"";
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString FormatDeviceName(const PString& manufacturer, const PString& product)
    {
        PString deviceName;

        if (!manufacturer.empty()) {
            deviceName += manufacturer;
        }
        if (!product.empty())
        {
            if (!deviceName.empty()) {
                deviceName += " ";
            }
            deviceName += product;
        }
        if (deviceName.empty()) {
            deviceName = "unnamed";
        }
        return deviceName;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString FormatBCD(uint16_t littleEndianBCD)
    {
        const uint16_t value = PLittleEndianToHost(littleEndianBCD);
        return PString::format_string("{:x}.{:02x}", static_cast<uint32_t>((value >> 8) & 0xff), static_cast<uint32_t>(value & 0xff));
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* FormatBool(bool value)
    {
        return value ? "yes" : "no";
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString FormatDeviceState(const PUSBDeviceInfo& deviceInfo)
    {
        PString state;

        AppendFlag(state, deviceInfo.IsConnected, "connected");
        AppendFlag(state, deviceInfo.IsConfigured, "configured");

        if (state.empty()) {
            state = "none";
        }
        return state;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString FormatConfigurationAttributes(uint8_t attributes)
    {
        PString text;
        const bool selfPowered = (attributes & USB_DescConfiguration::ATTRIBUTES_SELF_POWERED) != 0;
        const bool remoteWakeup = (attributes & USB_DescConfiguration::ATTRIBUTES_REMOTE_WAKEUP) != 0;

        AppendFlag(text, true, selfPowered ? "self-powered" : "bus-powered");
        AppendFlag(text, remoteWakeup, "remote-wakeup");
        return text;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static void AppendFlag(PString& text, bool enabled, const char* name)
    {
        if (enabled)
        {
            if (!text.empty()) {
                text += ",";
            }
            text += name;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* GetSpeedName(USB_Speed speed)
    {
        switch (speed)
        {
            case USB_Speed::LOW:  return "low";
            case USB_Speed::FULL: return "full";
            case USB_Speed::HIGH: return "high";
        }
        return "unknown";
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString FormatClassCode(USB_ClassCode classCode)
    {
        return PString::format_string("{}(0x{:02x})", GetClassName(classCode), static_cast<uint32_t>(std::to_underlying(classCode)));
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* GetClassName(USB_ClassCode classCode)
    {
        switch (classCode)
        {
            case USB_ClassCode::UNSPECIFIED:          return "per-interface";
            case USB_ClassCode::AUDIO:                return "audio";
            case USB_ClassCode::CDC:                  return "cdc";
            case USB_ClassCode::HID:                  return "hid";
            case USB_ClassCode::PHYSICAL:             return "physical";
            case USB_ClassCode::IMAGE:                return "image";
            case USB_ClassCode::PRINTER:              return "printer";
            case USB_ClassCode::MSC:                  return "mass-storage";
            case USB_ClassCode::HUB:                  return "hub";
            case USB_ClassCode::CDC_DATA:             return "cdc-data";
            case USB_ClassCode::SMART_CARD:           return "smart-card";
            case USB_ClassCode::CONTENT_SECURITY:     return "content-security";
            case USB_ClassCode::VIDEO:                return "video";
            case USB_ClassCode::PERSONAL_HEALTHCARE:  return "personal-healthcare";
            case USB_ClassCode::AUDIO_VIDEO:          return "audio-video";
            case USB_ClassCode::DIAGNOSTIC:           return "diagnostic";
            case USB_ClassCode::WIRELESS_CONTROLLER:  return "wireless-controller";
            case USB_ClassCode::MISC:                 return "misc";
            case USB_ClassCode::APPLICATION_SPECIFIC: return "application-specific";
            case USB_ClassCode::VENDOR_SPECIFIC:      return "vendor-specific";
            default:                                  return "unknown";
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* GetTransferTypeName(USB_TransferType transferType)
    {
        switch (transferType)
        {
            case USB_TransferType::CONTROL:     return "control";
            case USB_TransferType::ISOCHRONOUS: return "isochronous";
            case USB_TransferType::BULK:        return "bulk";
            case USB_TransferType::INTERRUPT:   return "interrupt";
        }
        return "unknown";
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* GetEndpointDirectionName(const USB_DescEndpoint& endpointDescriptor)
    {
        return ((endpointDescriptor.bEndpointAddress & USB_ADDRESS_DIR_IN) != 0) ? "in" : "out";
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* GetEndpointSyncName(USB_IsoEndpointSyncType syncType)
    {
        switch (syncType)
        {
            case USB_IsoEndpointSyncType::NONE:     return "none";
            case USB_IsoEndpointSyncType::ASYNC:    return "async";
            case USB_IsoEndpointSyncType::ADAPTIVE: return "adaptive";
            case USB_IsoEndpointSyncType::SYNC:     return "sync";
        }
        return "unknown";
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* GetEndpointUsageName(USB_EndpointUsageType usageType)
    {
        switch (usageType)
        {
            case USB_EndpointUsageType::DATA:                   return "data";
            case USB_EndpointUsageType::FEEDBACK:               return "feedback";
            case USB_EndpointUsageType::EXPLICIT_FEEDBACK_DATA: return "explicit-feedback-data";
        }
        return "unknown";
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static const char* GetDescriptorTypeName(USB_DescriptorType descriptorType)
    {
        switch (descriptorType)
        {
            case USB_DescriptorType::DEVICE:                            return "device";
            case USB_DescriptorType::CONFIGURATION:                     return "configuration";
            case USB_DescriptorType::STRING:                            return "string";
            case USB_DescriptorType::INTERFACE:                         return "interface";
            case USB_DescriptorType::ENDPOINT:                          return "endpoint";
            case USB_DescriptorType::DEVICE_QUALIFIER:                  return "device-qualifier";
            case USB_DescriptorType::OTHER_SPEED_CONFIGURATION:         return "other-speed-configuration";
            case USB_DescriptorType::INTERFACE_POWER:                   return "interface-power";
            case USB_DescriptorType::OTG:                               return "otg";
            case USB_DescriptorType::DEBUG_DESC:                        return "debug";
            case USB_DescriptorType::INTERFACE_ASSOCIATION:             return "interface-association";
            case USB_DescriptorType::BOS:                               return "bos";
            case USB_DescriptorType::DEVICE_CAPABILITY:                 return "device-capability";
            case USB_DescriptorType::FUNCTIONAL:                        return "functional";
            case USB_DescriptorType::CS_CONFIGURATION:                  return "class-configuration";
            case USB_DescriptorType::CS_STRING:                         return "class-string";
            case USB_DescriptorType::CS_INTERFACE:                      return "class-interface";
            case USB_DescriptorType::CS_ENDPOINT:                       return "class-endpoint";
            case USB_DescriptorType::HUB:                               return "hub";
            case USB_DescriptorType::SUPERSPEED_ENDPOINT_COMPANION:     return "superspeed-endpoint-companion";
            case USB_DescriptorType::SUPERSPEED_ISO_ENDPOINT_COMPANION: return "superspeed-iso-endpoint-companion";
            default:                                                    return "unknown";
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    static PString FormatDescriptorDetails(const USB_DescriptorHeader* descriptor)
    {
        switch (descriptor->bDescriptorType)
        {
            case USB_DescriptorType::INTERFACE:
                if (descriptor->bLength >= sizeof(USB_DescInterface))
                {
                    const USB_DescInterface* interfaceDescriptor = reinterpret_cast<const USB_DescInterface*>(descriptor);
                    return PString::format_string(" number={} alt={} class={} subclass=0x{:02x} protocol=0x{:02x} endpoints={}",
                        static_cast<uint32_t>(interfaceDescriptor->bInterfaceNumber),
                        static_cast<uint32_t>(interfaceDescriptor->bAlternateSetting),
                        FormatClassCode(interfaceDescriptor->bInterfaceClass),
                        static_cast<uint32_t>(interfaceDescriptor->bInterfaceSubClass),
                        static_cast<uint32_t>(interfaceDescriptor->bInterfaceProtocol),
                        static_cast<uint32_t>(interfaceDescriptor->bNumEndpoints)
                    );
                }
                break;
            case USB_DescriptorType::ENDPOINT:
                if (descriptor->bLength >= sizeof(USB_DescEndpoint))
                {
                    const USB_DescEndpoint* endpointDescriptor = reinterpret_cast<const USB_DescEndpoint*>(descriptor);
                    return PString::format_string(" address=0x{:02x} direction={} transfer={} maxPacket={} interval={}",
                        static_cast<uint32_t>(endpointDescriptor->bEndpointAddress),
                        GetEndpointDirectionName(*endpointDescriptor),
                        GetTransferTypeName(endpointDescriptor->GetTransferType()),
                        static_cast<uint32_t>(endpointDescriptor->GetMaxPacketSize()),
                        static_cast<uint32_t>(endpointDescriptor->bInterval)
                    );
                }
                break;
            case USB_DescriptorType::INTERFACE_ASSOCIATION:
                if (descriptor->bLength >= sizeof(USB_DescInterfaceAssociation))
                {
                    const USB_DescInterfaceAssociation* associationDescriptor = reinterpret_cast<const USB_DescInterfaceAssociation*>(descriptor);
                    return PString::format_string(" firstInterface={} count={} class={} subclass=0x{:02x} protocol=0x{:02x}",
                        static_cast<uint32_t>(associationDescriptor->bFirstInterface),
                        static_cast<uint32_t>(associationDescriptor->bInterfaceCount),
                        FormatClassCode(associationDescriptor->bFunctionClass),
                        static_cast<uint32_t>(associationDescriptor->bFunctionSubClass),
                        static_cast<uint32_t>(associationDescriptor->bFunctionProtocol)
                    );
                }
                break;
            default:
                break;
        }
        return PString();
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template<typename ...ARGS>
    void Print(PFormatString<ARGS...>&& format, ARGS&&... arguments)
    {
        const PString text = PString::format_string(std::forward<PFormatString<ARGS...>>(format), std::forward<ARGS>(arguments)...);
        write(STDOUT_FILENO, text.c_str(), text.size());
    }

    bool m_ShowDescriptorBlocks = true;
    bool m_CompactOutput = false;
    bool m_HadError = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int ls_usb_main(int argc, char* argv[])
{
    CmdLSUSB command;
    return command.Invoke(argc, argv);
}

static PAppDefinition g_LSUSBAppDef("ls_usb", "List USB devices and interfaces.", ls_usb_main);

} // namespace shutil_ls_usb
