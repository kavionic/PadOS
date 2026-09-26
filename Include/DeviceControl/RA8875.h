// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.04.2026 22:30

#pragma once

#include <DeviceControl/DeviceControlInvoker.h>


class PRA8875 : public PDeviceControlInterface
{
public:
    PRA8875() : WaitBlitter(*this) {}
    explicit PRA8875(int fileHandle) : WaitBlitter(*this) { SetDeviceFD(fileHandle); }

    PDeviceControlInvoker<0, void()> WaitBlitter;
};
