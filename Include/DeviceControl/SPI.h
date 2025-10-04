// This file is part of PadOS.
//
// Copyright (C) 2023 Kurt Skauen <http://kavionic.com/>
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
// Created: 22.06.2023 19:30

#pragma once

#include <Kernel/VFS/FileIO.h>
#include <System/SysTime.h>

enum SPIIOCTL
{
    SPIIOCTL_SET_BAUDRATE_DIVIDER,
    SPIIOCTL_GET_BAUDRATE_DIVIDER,
    SPIIOCTL_SET_READ_TIMEOUT,
    SPIIOCTL_GET_READ_TIMEOUT,
    SPIIOCTL_SET_WRITE_TIMEOUT,
    SPIIOCTL_GET_WRITE_TIMEOUT,
    SPIIOCTL_SET_PINMODE,
    SPIIOCTL_GET_PINMODE,
    SPIIOCTL_SET_SWAP_MOSI_MISO,
    SPIIOCTL_GET_SWAP_MOSI_MISO,
    SPIIOCTL_START_TRANSACTION,
    SPIIOCTL_GET_LAST_ERROR
};

enum class SPIError : int
{
    None,
    CRCError,
    TIFrameError,
    ModeFault,
    Unknown
};

enum class SPIRole : int
{
    Master,
    Slave
};

enum class SPIProtocol : int
{
    Motorola,
    TI
};

enum class SPIComMode
{
    FullDuplex,
    SimplexTransmitter,
    SimplexReceiver,
    HalfDuplex
};

enum SPIEndian
{
    MSB,
    LSB
};

enum class SPIBaudRateDivider : uint32_t
{
    DIV2 = 0,
    DIV4 = 1,
    DIV8 = 2,
    DIV16 = 3,
    DIV32 = 4,
    DIV64 = 5,
    DIV128 = 6,
    DIV256 = 7
};

enum class SPICLkPhase : int
{
    FirstEdge,
    SecondEdge
};

enum class SPICLkPolarity : int
{
    Low,
    High
};

enum class SPISlaveUnderrunMode
{
    ConstantPattern,
    RepeatLastReceived,
    RepeatLastTransmitted
};

enum class SPIPin : int
{
    CLK,
    MOSI,
    MISO
};

enum class SPIPinMode : int
{
    Normal, // Normal SPI function.
    Off,    // Low power, possibly floating.
    Low,    // Forced driven low.
    High    // Forced driven high.
};

static constexpr uint32_t SPI_DISABLE_MOSI      = 0x01;
static constexpr uint32_t SPI_DISABLE_MISO      = 0x02;


struct SPITransaction
{
    size_t      Length = 0;
    void*       ReceiveBuffer = nullptr;
    const void* TransmitBuffer = nullptr;
};

inline int SPIIOCTL_SetBaudrateDivider(int device, SPIBaudRateDivider divider)
{
    return os::FileIO::DeviceControl(device, SPIIOCTL_SET_BAUDRATE_DIVIDER, &divider, sizeof(divider), nullptr, 0);
}

inline int SPIIOCTL_GetBaudrateDivider(int device, SPIBaudRateDivider& divider)
{
    return os::FileIO::DeviceControl(device, SPIIOCTL_GET_BAUDRATE_DIVIDER, nullptr, 0, &divider, sizeof(divider));
}

inline int SPIIOCTL_SetReadTimeout(int device, TimeValNanos timeout)
{
    bigtime_t nanos = timeout.AsNanoseconds();
    return os::FileIO::DeviceControl(device, SPIIOCTL_SET_READ_TIMEOUT, &nanos, sizeof(nanos), nullptr, 0);
}

inline int SPIIOCTL_GetReadTimeout(int device, TimeValNanos& outTimeout)
{
    bigtime_t nanos;
    if (os::FileIO::DeviceControl(device, SPIIOCTL_GET_READ_TIMEOUT, nullptr, 0, &nanos, sizeof(nanos)) < 0) return -1;
    outTimeout = TimeValNanos::FromNanoseconds(nanos);
    return 0;
}

inline int SPIIOCTL_SetWriteTimeout(int device, TimeValNanos timeout)
{
    bigtime_t nanos = timeout.AsNanoseconds();
    return os::FileIO::DeviceControl(device, SPIIOCTL_SET_WRITE_TIMEOUT, &nanos, sizeof(nanos), nullptr, 0);
}

inline int SPIIOCTL_GetWriteTimeout(int device, TimeValNanos& outTimeout)
{
    bigtime_t nanos;
    if (os::FileIO::DeviceControl(device, SPIIOCTL_GET_WRITE_TIMEOUT, nullptr, 0, &nanos, sizeof(nanos)) < 0) return -1;
    outTimeout = TimeValNanos::FromNanoseconds(nanos);
    return 0;
}

inline int SPIIOCTL_SetPinMode(int device, SPIPin pin, SPIPinMode mode)
{
    int arg = (int(pin) << 16) | int(mode);
    return os::FileIO::DeviceControl(device, SPIIOCTL_SET_PINMODE, &arg, sizeof(arg), nullptr, 0);
}

inline int SPIIOCTL_GetPinMode(int device, SPIPin pin, SPIPinMode& outMode)
{
    int arg = int(pin);
    int result = 0;
    if (os::FileIO::DeviceControl(device, SPIIOCTL_GET_PINMODE, &arg, sizeof(arg), &result, sizeof(result)) < 0) {
        return -1;
    } else {
        outMode = SPIPinMode(result);
        return 0;
    }
}

inline int SPIIOCTL_SetSwap_MOSI_MISO(int device, bool doSwap)
{
    int arg = doSwap;
    return os::FileIO::DeviceControl(device, SPIIOCTL_SET_SWAP_MOSI_MISO, &arg, sizeof(arg), nullptr, 0);
}

inline int SPIIOCTL_GetSwap_MOSI_MISO(int device, bool& outIsSwapped)
{
    int result = 0;
    if (os::FileIO::DeviceControl(device, SPIIOCTL_GET_SWAP_MOSI_MISO, nullptr, 0, &result, sizeof(result)) < 0) {
        return -1;
    } else {
        outIsSwapped = result != 0;
        return 0;
    }
}

inline ssize_t SPIIOCTL_StartTransaction(int device, const SPITransaction& transaction)
{
    ssize_t result = -1;
    if (os::FileIO::DeviceControl(device, SPIIOCTL_START_TRANSACTION, &transaction, sizeof(transaction), &result, sizeof(result)) >= 0) {
        return result;
    } else {
        return -1;
    }
}

inline int SPIIOCTL_GetLastError(int device, SPIError& outError)
{
    return os::FileIO::DeviceControl(device, SPIIOCTL_GET_LAST_ERROR, nullptr, 0, &outError, sizeof(outError));
}
