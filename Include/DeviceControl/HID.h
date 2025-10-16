// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 19.03.2018 22:42:31

#pragma once

#include <PadOS/Filesystem.h>
#include <PadOS/DeviceControl.h>


enum HIDIOCTL
{
	HIDIOCTL_SET_TARGET_PORT,
	HIDIOCTL_GET_TARGET_PORT
};

inline PErrorCode HIDIOCTL_SetTargetPort(int device, port_id port) { return device_control(device, HIDIOCTL_SET_TARGET_PORT, &port, sizeof(port), nullptr, 0); }
inline PErrorCode HIDIOCTL_GetTargetPort(int device, port_id& outPort)
{
    return device_control(device, HIDIOCTL_GET_TARGET_PORT, nullptr, 0, &outPort, sizeof(outPort));
}


