// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
