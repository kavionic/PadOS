// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 28.05.2022 15:00

#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBProtocol.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_Initialize()
{
    REGISTER_KERNEL_LOG_CATEGORY(kernel::LogCategoryUSB, kernel::KLogSeverity::INFO_LOW_VOL);
    REGISTER_KERNEL_LOG_CATEGORY(kernel::LogCategoryUSBDevice, kernel::KLogSeverity::WARNING);
    REGISTER_KERNEL_LOG_CATEGORY(kernel::LogCategoryUSBHost, kernel::KLogSeverity::INFO_LOW_VOL);
}


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
