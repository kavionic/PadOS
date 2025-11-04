// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 28.10.2025 23:00

#include <sys/pados_syscalls.h>
#include <System/ExceptionHandling.h>
#include <Kernel/Misc.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/KAddressValidation.h>

namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_duplicate_handle(handle_id handle, handle_id* outNewHandle)
{
    try
    {
        *outNewHandle = kduplicate_handle_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_delete_handle(handle_id handle)
{
    try
    {
        kdelete_handle_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool sys_is_debugger_attached()
{
    return kis_debugger_attached();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_digital_pin_set_direction(DigitalPinID pinID, DigitalPinDirection_e dir)
{
    return kdigital_pin_set_direction(pinID, dir);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_digital_pin_set_drive_strength(DigitalPinID pinID, DigitalPinDriveStrength_e strength)
{
    return kdigital_pin_set_drive_strength(pinID, strength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_digital_pin_set_pull_mode(DigitalPinID pinID, PinPullMode_e mode)
{
    return kdigital_pin_set_pull_mode(pinID, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_digital_pin_set_peripheral_mux(DigitalPinID pinID, DigitalPinPeripheralID peripheral)
{
    return kdigital_pin_set_peripheral_mux(pinID, peripheral);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_digital_pin_read(DigitalPinID pinID, bool* outValue)
{
    return kdigital_pin_read(pinID, *outValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_digital_pin_write(DigitalPinID pinID, bool value)
{
    return kdigital_pin_write(pinID, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_write_backup_register(size_t registerID, uint32_t value)
{
    try
    {
        kwrite_backup_register_trw(registerID, value);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_read_backup_register(size_t registerID, uint32_t* outValue)
{
    try
    {
        validate_user_write_pointer_trw(outValue);
        *outValue = kread_backup_register_trw(registerID);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_beep_seconds(float duration)
{
    kbeep_seconds(duration);
    return PErrorCode::Success;
}

} // extern "C"

} // namespace kernel
