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


constexpr float TLV493D_LSB_FLUX = 0.098e-3f;	// LSB = 0.098mT

struct tlv493d_config
{
	int32_t frame_rate = 10;
	int32_t oversampling = 1;

	float temparature_scale = 1.0f;
	float temperature_offset = 0.0f;
};


struct tlv493d_data
{
	int32_t frame_number;
	float x;
	float y;
	float z;
	float temperature;
};

enum TLV493DIOCTL
{
	TLV493DIOCTL_SET_CONFIG,
	TLV493DIOCTL_GET_VALUES,
	TLV493DIOCTL_GET_VALUES_SYNC
};

inline int TLV493DIOCTL_SetConfig(int device, const tlv493d_config* values) { return os::FileIO::DeviceControl(device, TLV493DIOCTL_SET_CONFIG, values, sizeof(*values), nullptr, 0); }
inline int TLV493DIOCTL_GetValues(int device, tlv493d_data* values) { return os::FileIO::DeviceControl(device, TLV493DIOCTL_GET_VALUES, nullptr, 0, values, sizeof(*values)); }
inline int TLV493DIOCTL_GetValuesSync(int device, tlv493d_data* values) { return os::FileIO::DeviceControl(device, TLV493DIOCTL_GET_VALUES_SYNC, nullptr, 0, values, sizeof(*values)); }
