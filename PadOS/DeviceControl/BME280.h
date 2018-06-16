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
// Created: 25.02.2018 21:18:49

#pragma once

#include "Kernel/VFS/FileIO.h"


struct BME280Values
{
    double m_Temperature = 0.0;
    double m_Pressure    = 0.0;
    double m_Humidity    = 0.0;
};

enum BME280IOCTL
{
    BME280IOCTL_GET_VALUES
};

inline int BME280IOCTL_GetValues(int device, BME280Values* values) { return os::FileIO::DeviceControl(device, BME280IOCTL_GET_VALUES, nullptr, 0, values, sizeof(*values)); }
