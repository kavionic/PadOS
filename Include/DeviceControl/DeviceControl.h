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
// Created: 02.05.2018 21:24:23

#pragma once

struct device_geometry
{
    off64_t sector_count;
    size_t  bytes_per_sector;
    bool    read_only;
    bool    removable;
};

enum DeviceControlID
{
    DEVCTL_GET_DEVICE_GEOMETRY = 1000000000,
    DEVCTL_REREAD_PARTITION_TABLE    
};