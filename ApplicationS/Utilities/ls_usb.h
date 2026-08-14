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

#pragma once

#include <utility>
#include <vector>
#include <unistd.h>

#include <DeviceControl/USB.h>
#include <Utils/String.h>


namespace shutil_ls_usb
{

struct NameValue
{
    PString Name;
    PString Value;
};

class CmdLSUSB
{
public:
    int Invoke(int argc, char* argv[]);

private:
    void PrintTopology();
    void PrintTopologyNodeTree(const PString& topologyPath, const std::vector<bool>& treeBranches, bool isLastNode);
    void PrintDeviceTree(const PString& devicePath, const std::vector<bool>& treeBranches, bool isLastDevice);
    void PrintHubInfoTree(const PUSBDeviceInfo& deviceInfo, const std::vector<bool>& treeBranches, bool isLastHubInfo);
    void PrintConfigurationInfoTree(const PUSBDeviceInfo& deviceInfo, const std::vector<bool>& treeBranches, bool isLastConfiguration);
    void PrintInterfaceTree(const PString& interfacePath, const std::vector<bool>& treeBranches, bool isLastInterface);
    void PrintEndpointInfoTree(const PUSBDeviceInterface& deviceInterface, const USB_DescEndpoint& endpointDescriptor, const PUSBHostPipeInfo& hostPipeInfo, const std::vector<bool>& treeBranches, bool isLastEndpoint);
    void PrintHostPipeInfoTree(const PUSBDeviceInterface& deviceInterface, uint8_t endpointAddr, const PUSBHostPipeInfo& hostPipeInfo, const std::vector<bool>& treeBranches, bool isLastHostPipe);
    void PrintInterfaceDescriptorBlockTree(const PUSBDeviceInterface& deviceInterface, size_t descriptorSize, const std::vector<bool>& treeBranches, bool isLastDescriptorBlock);
    void PrintDescriptorTree(const USB_DescriptorHeader* descriptor, const std::vector<bool>& treeBranches, bool isLastDescriptor);
    void PrintTopologyNodeCompact(const PString& topologyPath, size_t depth);
    void PrintDeviceCompact(const PString& nodeName, const PString& devicePath, size_t depth);
    void PrintDeviceInfoCompact(const PString& nodeName, const PString& devicePath, const PUSBDeviceInfo& deviceInfo, const PString& manufacturer, const PString& product, const PString& serialNumber, size_t depth);
    void PrintConfigurationInfoCompact(const PUSBDeviceInfo& deviceInfo, size_t depth);
    void PrintInterfaceListCompact(const PString& devicePath, size_t depth);
    void PrintInterfaceCompact(const PString& interfacePath, size_t depth);
    void PrintInterfaceInfoCompact(const PUSBDeviceInterface& deviceInterface, const PUSBDeviceInterfaceInfo& interfaceInfo, size_t depth);
    void PrintEndpointInfoCompact(const PUSBDeviceInterface& deviceInterface, const USB_DescEndpoint& endpointDescriptor, const PUSBHostPipeInfo& hostPipeInfo, size_t depth);
    void PrintInterfaceDescriptorBlockCompact(const PUSBDeviceInterface& deviceInterface, size_t descriptorSize, size_t depth);
    PString ReadDeviceString(const PUSBDeviceControl& deviceControl, PUSBDeviceStringID stringID) const;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    PString ReadHostPipeDebugEntryString(const PUSBDeviceInterface& deviceInterface, uint8_t endpointAddr, size_t entryIndex, bool readValue) const;
    std::vector<NameValue> ReadHostPipeDebugEntries(const PUSBDeviceInterface& deviceInterface, uint8_t endpointAddr) const;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    std::vector<PString> GetSortedPrefixedNames(const PString& directoryPath, const char* prefix, bool directoriesOnly);
    bool IsDirectoryEntry(int directoryHandle, const PString& directoryPath, const PString& entryName);
    PString ReadOptionalSymlink(const PString& path);
    static std::vector<NameValue> MakeDeviceValues(const PString& devicePath, const PUSBDeviceInfo& deviceInfo, const PString& manufacturer, const PString& product, const PString& serialNumber);
    static std::vector<NameValue> MakeInterfaceValues(const PUSBDeviceInterfaceInfo& interfaceInfo);
    static std::vector<NameValue> MakeHostPipeValues(const PUSBHostPipeInfo& hostPipeInfo);
    static std::vector<NameValue> MakeDescriptorValues(const USB_DescriptorHeader* descriptor);
    void PrintTreeValues(const std::vector<bool>& treeBranches, const std::vector<NameValue>& values);
    void PrintTreeProperty(const std::vector<bool>& treeBranches, bool isLastValue, const PString& name, const PString& value);
    void PrintTreeLine(const std::vector<bool>& treeBranches, bool isLastLine, const PString& text);
    static PString MakeTreePrefix(const std::vector<bool>& treeBranches);
    static bool HasPrefixedNumber(const PString& name, const char* prefix);
    static size_t ParsePrefixedNumber(const PString& name, const char* prefix);
    static bool ComparePrefixedNumbers(const PString& lhs, const PString& rhs, const char* prefix);
    static PString GetBaseName(const PString& path);
    static PString MakeIndent(size_t depth);
    static PString QuoteString(const PString& value);
    static PString FormatDeviceName(const PString& manufacturer, const PString& product);
    static PString FormatBCD(uint16_t littleEndianBCD);
    static const char* FormatBool(bool value);
    static PString FormatDeviceState(const PUSBDeviceInfo& deviceInfo);
    static PString FormatConfigurationAttributes(uint8_t attributes);
    static PString FormatHostPipeCompact(const PUSBHostPipeInfo& hostPipeInfo);
    static void AppendFlag(PString& text, bool enabled, const char* name);
    static const char* GetSpeedName(USB_Speed speed);
    static PString FormatClassCode(USB_ClassCode classCode);
    static const char* GetClassName(USB_ClassCode classCode);
    static const char* GetRequestDirectionName(USB_RequestDirection direction);
    static const char* GetURBStateName(USB_URBState state);
    static const char* GetTransferTypeName(USB_TransferType transferType);
    static const char* GetEndpointDirectionName(const USB_DescEndpoint& endpointDescriptor);
    static const char* GetEndpointSyncName(USB_IsoEndpointSyncType syncType);
    static const char* GetEndpointUsageName(USB_EndpointUsageType usageType);
    static const char* GetDescriptorTypeName(USB_DescriptorType descriptorType);
    static PString FormatDescriptorDetails(const USB_DescriptorHeader* descriptor);

    template<typename ...ARGS>
    void Print(PFormatString<ARGS...>&& format, ARGS&&... arguments)
    {
        const PString text = PString::format_string(
            std::forward<PFormatString<ARGS...>>(format),
            std::forward<ARGS>(arguments)...);
        write(STDOUT_FILENO, text.c_str(), text.size());
    }

    bool m_ShowDescriptorBlocks = true;
#if PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    bool m_ShowDebugInfo = false;
#endif // PADOS_OPT_DEBUG_USB_DIAGNOSTICS
    bool m_CompactOutput = false;
    bool m_HadError = false;
};

int ls_usb_main(int argc, char* argv[]);

} // namespace shutil_ls_usb
