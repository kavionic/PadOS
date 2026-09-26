// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 02.05.2018 21:22:43

#pragma once

#include <PadOS/Filesystem.h>
#include <PadOS/DeviceControl.h>

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

inline PErrorCode SDCDEVCTL_SDIOReadDirect(int device, uint8_t functionNumber, uint32_t addr, uint8_t *dest)
{
    SDCDEVCTL_SDIOReadDirectArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    return device_control(device, SDCDEVCTL_SDIO_READ_DIRECT, &args, sizeof(args), dest, sizeof(*dest));
}

inline PErrorCode SDCDEVCTL_SDIOWriteDirect(int device, uint8_t functionNumber, uint32_t addr, uint8_t data)
{
    SDCDEVCTL_SDIOWriteDirectArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    args.Data = data;
    return device_control(device, SDCDEVCTL_SDIO_WRITE_DIRECT, &args, sizeof(args), nullptr, 0);
}

inline PErrorCode  SDCDEVCTL_SDIOReadExtended(int device, uint8_t functionNumber, uint32_t addr, bool incrementAddr, void* buffer, size_t size)
{
    SDCDEVCTL_SDIOReadExtendedArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    args.IncrementAddr = incrementAddr;
    return device_control(device, SDCDEVCTL_SDIO_READ_EXTENDED, &args, sizeof(args), buffer, size);
}

inline PErrorCode  SDCDEVCTL_SDIOWriteExtended(int device, uint8_t functionNumber, uint32_t addr, bool incrementAddr, const void* buffer, size_t size)
{
    SDCDEVCTL_SDIOWriteExtendedArgs args;
    args.Address = addr;
    args.FunctionNumber = functionNumber;
    args.IncrementAddr = incrementAddr;
    args.Buffer        = buffer;
    args.Size          = size;
    return device_control(device, SDCDEVCTL_SDIO_WRITE_EXTENDED, &args, sizeof(args), nullptr, 0);
}

