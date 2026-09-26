// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 30.07.2022 23:00

#pragma once

#include <stdint.h>
#include <System/Sections.h>
#include <Kernel/USB/USBHostControl.h>

namespace kernel
{
class USBHost;
class USBDeviceNode;

class USBHostEnumerator
{
public:
    void Setup(USBHost* host) { m_HostHandler = host; }

    void Reset();
    bool Enumerate(USBHostControlRequestCallback&& callback);

private:
    void SendResult(bool result, uint8_t deviceAddr);

    void HandleConfigurationHeaderResult(bool result, uint8_t deviceAddr);
    void HandleConfigurationFullResult(bool result, uint8_t deviceAddr);
    void HandleSetAddressResult(bool result, uint8_t deviceAddr);
    void StartConfigurationDescriptorRead(uint8_t deviceAddr);
    void GetStringDescriptors(uint8_t deviceAddr);
    void HandleGetManufacturerStringResult(bool result, uint8_t deviceAddr);
    void HandleGetProductStringResult(bool result, uint8_t deviceAddr);
    void HandleGetSerialNumberResult(bool result, uint8_t deviceAddr);


    USBHost* m_HostHandler = nullptr;
    USBHostControlRequestCallback m_ResultCallback;
};

} // namespace kernel
