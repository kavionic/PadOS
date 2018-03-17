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
// Created: 25.02.2018 00:13:11

#pragma once

#include "Kernel/Kernel.h"

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


inline int INA3221_GetMeasurements(int device, INA3221Values* values)           { return kernel::Kernel::DeviceControl(device, INA3221_CMD_GET_MEASUREMENTS, nullptr, 0, values, sizeof(*values)); }
inline int INA3221_SetShuntConfig(int device, const INA3221ShuntConfig& config) { return kernel::Kernel::DeviceControl(device, INA3221_CMD_SET_SHUNT_CFG, &config, sizeof(INA3221ShuntConfig), nullptr, 0); }
inline int INA3221_GetShuntConfig(int device, INA3221ShuntConfig* config)       { return kernel::Kernel::DeviceControl(device, INA3221_CMD_GET_SHUNT_CFG, nullptr, 0, config, sizeof(*config)); }

