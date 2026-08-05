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
