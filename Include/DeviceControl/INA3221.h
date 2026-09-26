// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 25.02.2018 00:13:11

#pragma once

#include "Kernel/Kernel.h"
#include "Kernel/VFS/FileIO.h"

#define INA3221_SENSOR_COUNT 3

struct INA3221ShuntConfig
{
    double ShuntValues[INA3221_SENSOR_COUNT];
};

struct INA3221Values
{
    double Currents[INA3221_SENSOR_COUNT];
    double Voltages[INA3221_SENSOR_COUNT];
};

#define INA3221_CMD_GET_MEASUREMENTS 0
#define INA3221_CMD_SET_SHUNT_CFG    1
#define INA3221_CMD_GET_SHUNT_CFG    2


inline int INA3221_GetMeasurements(int device, INA3221Values* values)           { return os::FileIO::DeviceControl(device, INA3221_CMD_GET_MEASUREMENTS, nullptr, 0, values, sizeof(*values)); }
inline int INA3221_SetShuntConfig(int device, const INA3221ShuntConfig& config) { return os::FileIO::DeviceControl(device, INA3221_CMD_SET_SHUNT_CFG, &config, sizeof(INA3221ShuntConfig), nullptr, 0); }
inline int INA3221_GetShuntConfig(int device, INA3221ShuntConfig* config)       { return os::FileIO::DeviceControl(device, INA3221_CMD_GET_SHUNT_CFG, nullptr, 0, config, sizeof(*config)); }

