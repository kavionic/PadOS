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
#include <SerialConsole/SerialCommandHandler.h>
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_beep_seconds(float duration)
{
    kbeep_seconds(duration);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_add_serial_command_handler(uint32_t command, port_id messagePortID)
{
    SerialCommandHandler::Get().SetHandlerMessagePort(messagePortID);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_serial_command_send_data(void* header, size_t headerSize, const void* data, size_t dataSize)
{
    return SerialCommandHandler::Get().SendSerialData(static_cast<SerialProtocol::PacketHeader*>(header), headerSize, data, dataSize) ? PErrorCode::Success : PErrorCode::IOError;
}

} // extern "C"

} // namespace kernel
