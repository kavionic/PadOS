// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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


