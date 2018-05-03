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
// Created: 02.05.2018 21:22:43

#pragma once

enum SDCardDeviceControl
{
    SDCDEVCTL_SDIO_READ_DIRECT,
    SDCDEVCTL_SDIO_WRITE_DIRECT,
    SDCDEVCTL_SDIO_READ_EXTENDED,
    SDCDEVCTL_SDIO_WRITE_EXTENDED,
};

struct SDCDEVCTL_SDIOReadDirectArgs
{
    uint8_t  FunctionNumber;
    uint32_t Address;
};

struct SDCDEVCTL_SDIOWriteDirectArgs
{
    uint32_t Address;
    uint8_t  FunctionNumber;
    uint8_t  Data;
};

struct SDCDEVCTL_SDIOReadExtendedArgs
{
    uint32_t Address;
    uint8_t  FunctionNumber;
    bool     IncrementAddr;
};

struct SDCDEVCTL_SDIOWriteExtendedArgs
{
    uint32_t    Address;
    uint8_t     FunctionNumber;
    bool        IncrementAddr;
    const void* Buffer;
    size_t      Size;
};

inline status_t SDCDEVCTL_SDIOReadDirect(int device, uint8_t functionNumber, uint32_t addr, uint8_t *dest)
{
    SDCDEVCTL_SDIOReadDirectArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    return kernel::Kernel::DeviceControl(device, SDCDEVCTL_SDIO_READ_DIRECT, &args, sizeof(args), dest, sizeof(*dest));
}

inline status_t SDCDEVCTL_SDIOWriteDirect(int device, uint8_t functionNumber, uint32_t addr, uint8_t data)
{
    SDCDEVCTL_SDIOWriteDirectArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    args.Data = data;
    return kernel::Kernel::DeviceControl(device, SDCDEVCTL_SDIO_WRITE_DIRECT, &args, sizeof(args), nullptr, 0);
}

inline ssize_t  SDCDEVCTL_SDIOReadExtended(int device, uint8_t functionNumber, uint32_t addr, bool incrementAddr, void* buffer, size_t size)
{
    SDCDEVCTL_SDIOReadExtendedArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    args.IncrementAddr = incrementAddr;
    return kernel::Kernel::DeviceControl(device, SDCDEVCTL_SDIO_READ_EXTENDED, &args, sizeof(args), buffer, size);
}

inline ssize_t  SDCDEVCTL_SDIOWriteExtended(int device, uint8_t functionNumber, uint32_t addr, bool incrementAddr, const void* buffer, size_t size)
{
    SDCDEVCTL_SDIOWriteExtendedArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    args.IncrementAddr = incrementAddr;
    args.Buffer        = buffer;
    args.Size          = size;
    return kernel::Kernel::DeviceControl(device, SDCDEVCTL_SDIO_WRITE_EXTENDED, &args, sizeof(args), nullptr, 0);
}

