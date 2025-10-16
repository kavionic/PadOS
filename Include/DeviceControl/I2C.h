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
// Created: 25.02.2018 21:19:21

#pragma once

#include <PadOS/Filesystem.h>
#include <PadOS/DeviceControl.h>


enum I2CIOCTL
{
    I2CIOCTL_SET_SLAVE_ADDRESS,
    I2CIOCTL_GET_SLAVE_ADDRESS,
    I2CIOCTL_SET_INTERNAL_ADDR_LEN,
    I2CIOCTL_GET_INTERNAL_ADDR_LEN,
    I2CIOCTL_SET_BAUDRATE,
    I2CIOCTL_GET_BAUDRATE,
	I2CIOCTL_SET_TIMEOUT,	// Timeout in microseconds (bigtime_t) for IO operations. Defaults to INFINIT_TIMEOUT
	I2CIOCTL_GET_TIMEOUT,
	I2CIOCTL_CLEAR_BUS		// Clock out any ongoing bits from a faulty transaction.
};

enum I2C_ADDR_LEN
{
	I2C_ADDR_LEN_7BIT = 1,
	I2C_ADDR_LEN_10BIT = 2
};

inline PErrorCode I2CIOCTL_SetSlaveAddress(int device, int address) { return device_control(device, I2CIOCTL_SET_SLAVE_ADDRESS, &address, sizeof(address), nullptr, 0); }
inline PErrorCode I2CIOCTL_GetSlaveAddress(int device, int& outAddress)
{
    return device_control(device, I2CIOCTL_GET_SLAVE_ADDRESS, nullptr, 0, &outAddress, sizeof(outAddress));
}

inline PErrorCode I2CIOCTL_SetInternalAddrLen(int device, int length) { return device_control(device, I2CIOCTL_SET_INTERNAL_ADDR_LEN, &length, sizeof(length), nullptr, 0); }
inline PErrorCode I2CIOCTL_GetInternalAddrLen(int device, int& outLength)
{
    return device_control(device, I2CIOCTL_GET_INTERNAL_ADDR_LEN, nullptr, 0, &outLength, sizeof(outLength));
}

inline PErrorCode I2CIOCTL_SetBaudrate(int device, int baudrate) { return device_control(device, I2CIOCTL_SET_BAUDRATE, &baudrate, sizeof(baudrate), nullptr, 0); }
inline PErrorCode I2CIOCTL_GetBaudrate(int device, int& outBaudrate)
{
    return device_control(device, I2CIOCTL_GET_BAUDRATE, nullptr, 0, &outBaudrate, sizeof(outBaudrate));
}

inline PErrorCode I2CIOCTL_SetTimeout(int device, TimeValNanos timeout)
{
    bigtime_t timeoutLL = timeout.AsNanoseconds();
    return device_control(device, I2CIOCTL_SET_TIMEOUT, &timeoutLL, sizeof(timeoutLL), nullptr, 0);
}
inline PErrorCode I2CIOCTL_GetTimeout(int device, TimeValNanos* timeout)
{
    if (timeout == nullptr) {
        return PErrorCode::InvalidArg;
    }
    bigtime_t timeoutLL;
    const PErrorCode result = device_control(device, I2CIOCTL_GET_TIMEOUT, nullptr, 0, &timeoutLL, sizeof(timeoutLL));
    if (result == PErrorCode::Success) {
        *timeout = TimeValNanos::FromNanoseconds(timeoutLL);
    }
    return result;
}

inline PErrorCode I2CIOCTL_ClearBus(int device) { return device_control(device, I2CIOCTL_CLEAR_BUS, nullptr, 0, nullptr, 0); }
