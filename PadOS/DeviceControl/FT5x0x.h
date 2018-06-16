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

#include "Kernel/VFS/FileIO.h"


enum FT5x0xIOCTL
{
    FT5x0xIOCTL_SET_TARGET_PORT,
    FT5x0xIOCTL_GET_TARGET_PORT
};

inline int FT5x0xIOCTL_SetTargetPort(int device, port_id port) { return os::FileIO::DeviceControl(device, FT5x0xIOCTL_SET_TARGET_PORT, &port, sizeof(port), nullptr, 0); }
inline int FT5x0xIOCTL_GetTargetPort(int device)
{
    port_id port;
    if (os::FileIO::DeviceControl(device, FT5x0xIOCTL_GET_TARGET_PORT, nullptr, 0, &port, sizeof(port)) < 0) return -1;
    return port;
}


