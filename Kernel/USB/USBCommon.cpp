// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 28.05.2022 15:00

#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBProtocol.h>

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const char* USB_GetSpeedName(USB_Speed speed)
{
    switch (speed)
    {
        case USB_Speed::FULL:   return "FULL";
        case USB_Speed::LOW:    return "LOW";
        case USB_Speed::HIGH:   return "HIGH";
        default:                return "INVALID";
    }
}


} // namespace kernel
