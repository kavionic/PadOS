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

#include <Kernel/VFS/FileIO.h>
#include <System/SysTime.h>

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

inline int USARTIOCTL_SetBaudrate(int device, int baudrate)
{
    return os::FileIO::DeviceControl(device, USARTIOCTL_SET_BAUDRATE, &baudrate, sizeof(baudrate), nullptr, 0);
}

inline int USARTIOCTL_GetBaudrate(int device)
{
    int baudrate = -1;
    if (os::FileIO::DeviceControl(device, USARTIOCTL_GET_BAUDRATE, nullptr, 0, &baudrate, sizeof(baudrate)) < 0) return -1;
    return baudrate;
}

inline int USARTIOCTL_SetReadTimeout(int device, TimeValNanos timeout)
{
    bigtime_t nanos = timeout.AsNanoseconds();
    return os::FileIO::DeviceControl(device, USARTIOCTL_SET_READ_TIMEOUT, &nanos, sizeof(nanos), nullptr, 0);
}

inline int USARTIOCTL_GetReadTimeout(int device, TimeValNanos& outTimeout)
{
    bigtime_t nanos;
    if (os::FileIO::DeviceControl(device, USARTIOCTL_GET_READ_TIMEOUT, nullptr, 0, &nanos, sizeof(nanos)) < 0) return -1;
    outTimeout = TimeValNanos::FromNanoseconds(nanos);
    return 0;
}

inline int USARTIOCTL_SetWriteTimeout(int device, TimeValNanos timeout)
{
    bigtime_t nanos = timeout.AsNanoseconds();
    return os::FileIO::DeviceControl(device, USARTIOCTL_SET_WRITE_TIMEOUT, &nanos, sizeof(nanos), nullptr, 0);
}

inline int USARTIOCTL_GetWriteTimeout(int device, TimeValNanos& outTimeout)
{
    bigtime_t nanos;
    if (os::FileIO::DeviceControl(device, USARTIOCTL_GET_WRITE_TIMEOUT, nullptr, 0, &nanos, sizeof(nanos)) < 0) return -1;
    outTimeout = TimeValNanos::FromNanoseconds(nanos);
    return 0;
}

inline int USARTIOCTL_SetIOCtrl(int device, uint32_t flags)
{
    return os::FileIO::DeviceControl(device, USARTIOCTL_SET_IOCTRL, &flags, sizeof(flags), nullptr, 0);
}

inline int USARTIOCTL_GetIOCtrl(int device)
{
    int flags = -1;
    if (os::FileIO::DeviceControl(device, USARTIOCTL_GET_IOCTRL, nullptr, 0, &flags, sizeof(flags)) < 0) return -1;
    return flags;
}

inline int USARTIOCTL_SetPinMode(int device, USARTPin pin, USARTPinMode mode)
{
    int arg = (int(pin) << 16) | int(mode);
    return os::FileIO::DeviceControl(device, USARTIOCTL_SET_PINMODE, &arg, sizeof(arg), nullptr, 0);
}

inline int USARTIOCTL_GetPinMode(int device, USARTPin pin, USARTPinMode& outMode)
{
    int arg = int(pin);
    int result = 0;
    if (os::FileIO::DeviceControl(device, USARTIOCTL_GET_PINMODE, &arg, sizeof(arg), &result, sizeof(result)) < 0) {
        return -1;
    } else {
        outMode = USARTPinMode(result);
        return 0;
    }
}

inline int USARTIOCTL_SetSwapRXTX(int device, bool doSwap)
{
    int arg = doSwap;
    return os::FileIO::DeviceControl(device, USARTIOCTL_SET_SWAPRXTX, &arg, sizeof(arg), nullptr, 0);
}

inline int USARTIOCTL_GetSwapRXTX(int device, bool& outIsSwapped)
{
    int result = 0;
    if (os::FileIO::DeviceControl(device, USARTIOCTL_GET_SWAPRXTX, nullptr, 0, &result, sizeof(result)) < 0) {
        return -1;
    } else {
        outIsSwapped = result != 0;
        return 0;
    }
}
