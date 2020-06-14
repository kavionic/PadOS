// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 01.06.2020 21:40:07

#pragma once

#include "Kernel/VFS/FileIO.h"


enum WS2812BIOCTL
{
	WS2812BIOCTL_SET_LED_COUNT,
	WS2812BIOCTL_GET_LED_COUNT,
	WS2812BIOCTL_SET_EXPONENTIAL,
	WS2812BIOCTL_GET_EXPONENTIAL,
};


inline int WS2812BIOCTL_SetLEDCount(int device, int count)
{
    return os::FileIO::DeviceControl(device, WS2812BIOCTL_SET_LED_COUNT, &count, sizeof(count), nullptr, 0);
}

inline int WS2812BIOCTL_GetLEDCount(int device)
{
    int count;
    if (os::FileIO::DeviceControl(device, WS2812BIOCTL_GET_LED_COUNT, nullptr, 0, &count, sizeof(count)) < 0) return -1;
    return count;
}

inline int WS2812BIOCTL_SetExponential(int device, bool exponential)
{
    int value = int(exponential);
    return os::FileIO::DeviceControl(device, WS2812BIOCTL_SET_EXPONENTIAL, &value, sizeof(value), nullptr, 0);
}

inline int WS2812BIOCTL_GetExponential(int device)
{
    int value;
    if (os::FileIO::DeviceControl(device, WS2812BIOCTL_GET_EXPONENTIAL, nullptr, 0, &value, sizeof(value)) < 0) return -1;
    return value;
}
