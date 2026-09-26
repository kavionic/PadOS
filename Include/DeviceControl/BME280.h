// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
