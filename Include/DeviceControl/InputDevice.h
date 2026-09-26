// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 05.08.2026 17:00

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <DeviceControl/DeviceControlInvoker.h>
#include <GUI/GUIEvent.h>


static constexpr int PInputDeviceControlRequest_GetRegisteredDevices = 0;

struct PInputDeviceInfo
{
    PInputClass ClassID;
    int32_t     SourceID;
};

class PInputDeviceControl : public PDeviceControlInterface
{
public:
    PInputDeviceControl()
        : GetRegisteredDevices(*this)
    {
    }

    explicit PInputDeviceControl(int fileHandle)
        : PInputDeviceControl()
    {
        SetDeviceFD(fileHandle);
    }

    PDeviceControlInvoker<
        PInputDeviceControlRequest_GetRegisteredDevices,
        size_t(PInputDeviceInfo* devices, size_t maxDeviceCount) const
    > GetRegisteredDevices;
};
