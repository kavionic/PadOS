// This file is part of PadOS.
//
// Copyright (C) 2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 20.03.2021 19:00

#pragma once

#include <PadOS/Filesystem.h>
#include <PadOS/DeviceControl.h>
#include <System/TimeValue.h>

enum USARTIOCTL
{
    USARTIOCTL_SET_BAUDRATE,
    USARTIOCTL_GET_BAUDRATE,
    USARTIOCTL_SET_READ_TIMEOUT,
    USARTIOCTL_GET_READ_TIMEOUT,
    USARTIOCTL_SET_WRITE_TIMEOUT,
    USARTIOCTL_GET_WRITE_TIMEOUT,
    USARTIOCTL_SET_IOCTRL,
    USARTIOCTL_GET_IOCTRL,
    USARTIOCTL_SET_PINMODE,
    USARTIOCTL_GET_PINMODE,
    USARTIOCTL_SET_SWAPRXTX,
    USARTIOCTL_GET_SWAPRXTX,
};

enum class USARTPin : int
{
    RX,
    TX
};

enum class USARTPinMode : int
{
    Normal, // Normal USART function.
    Off,    // Low power, possibly floating.
    Low,    // Forced driven low.
    High    // Forced driven high.
};

static constexpr uint32_t USART_DISABLE_RX      = 0x01;
static constexpr uint32_t USART_DISABLE_TX      = 0x02;

inline PErrorCode USARTIOCTL_SetBaudrate(int device, int baudrate)
{
    return device_control(device, USARTIOCTL_SET_BAUDRATE, &baudrate, sizeof(baudrate), nullptr, 0);
}

inline PErrorCode USARTIOCTL_GetBaudrate(int device, int& outBaudrate)
{
    return device_control(device, USARTIOCTL_GET_BAUDRATE, nullptr, 0, &outBaudrate, sizeof(outBaudrate));
}

inline PErrorCode USARTIOCTL_SetReadTimeout(int device, TimeValNanos timeout)
{
    bigtime_t nanos = timeout.AsNanoseconds();
    return device_control(device, USARTIOCTL_SET_READ_TIMEOUT, &nanos, sizeof(nanos), nullptr, 0);
}

inline PErrorCode USARTIOCTL_GetReadTimeout(int device, TimeValNanos& outTimeout)
{
    bigtime_t nanos;
    const PErrorCode result = device_control(device, USARTIOCTL_GET_READ_TIMEOUT, nullptr, 0, &nanos, sizeof(nanos));
    if (result != PErrorCode::Success) {
        return result;
    }
    outTimeout = TimeValNanos::FromNanoseconds(nanos);
    return PErrorCode::Success;
}

inline PErrorCode USARTIOCTL_SetWriteTimeout(int device, TimeValNanos timeout)
{
    bigtime_t nanos = timeout.AsNanoseconds();
    return device_control(device, USARTIOCTL_SET_WRITE_TIMEOUT, &nanos, sizeof(nanos), nullptr, 0);
}

inline PErrorCode USARTIOCTL_GetWriteTimeout(int device, TimeValNanos& outTimeout)
{
    bigtime_t nanos;
    const PErrorCode result = device_control(device, USARTIOCTL_GET_WRITE_TIMEOUT, nullptr, 0, &nanos, sizeof(nanos));
    if (result != PErrorCode::Success) {
        return result;
    }
    outTimeout = TimeValNanos::FromNanoseconds(nanos);
    return PErrorCode::Success;
}

inline PErrorCode USARTIOCTL_SetIOCtrl(int device, uint32_t flags)
{
    return device_control(device, USARTIOCTL_SET_IOCTRL, &flags, sizeof(flags), nullptr, 0);
}

inline PErrorCode USARTIOCTL_GetIOCtrl(int device, int& outFlags)
{
    return device_control(device, USARTIOCTL_GET_IOCTRL, nullptr, 0, &outFlags, sizeof(outFlags));
}

inline PErrorCode USARTIOCTL_SetPinMode(int device, USARTPin pin, USARTPinMode mode)
{
    int arg = (int(pin) << 16) | int(mode);
    return device_control(device, USARTIOCTL_SET_PINMODE, &arg, sizeof(arg), nullptr, 0);
}

inline PErrorCode USARTIOCTL_GetPinMode(int device, USARTPin pin, USARTPinMode& outMode)
{
    int arg = int(pin);
    int mode = 0;
    const PErrorCode result = device_control(device, USARTIOCTL_GET_PINMODE, &arg, sizeof(arg), &mode, sizeof(mode));
    if (result != PErrorCode::Success) {
        return result;
    }
    outMode = USARTPinMode(mode);
    return PErrorCode::Success;
}

inline PErrorCode USARTIOCTL_SetSwapRXTX(int device, bool doSwap)
{
    int arg = doSwap;
    return device_control(device, USARTIOCTL_SET_SWAPRXTX, &arg, sizeof(arg), nullptr, 0);
}

inline PErrorCode USARTIOCTL_GetSwapRXTX(int device, bool& outIsSwapped)
{
    int value = 0;
    const PErrorCode result = device_control(device, USARTIOCTL_GET_SWAPRXTX, nullptr, 0, &value, sizeof(value));
    if (result != PErrorCode::Success) {
        return result;
    }
    outIsSwapped = value != 0;
    return PErrorCode::Success;
}
