// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.02.2018 01:41:28

#pragma once
#include <sys/pados_error_codes.h>
#include <Kernel/HAL/DigitalPort_STM32.h>

namespace kernel
{

PErrorCode kdigital_pin_set_direction(DigitalPinID pinID, DigitalPinDirection_e dir);
PErrorCode kdigital_pin_set_drive_strength(DigitalPinID pinID, DigitalPinDriveStrength_e strength);
PErrorCode kdigital_pin_set_pull_mode(DigitalPinID pinID, PinPullMode_e mode);
PErrorCode kdigital_pin_set_peripheral_mux(DigitalPinID pinID, DigitalPinPeripheralID peripheral);
PErrorCode kdigital_pin_read(DigitalPinID pinID, bool& outValue);
PErrorCode kdigital_pin_write(DigitalPinID pinID, bool value);

} //namespace kernel
