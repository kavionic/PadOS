// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 02.05.2026 21:00

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <unwind.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static _Unwind_Reason_Code exit_unwind_stop(
    int version,
    _Unwind_Action actions,
    _Unwind_Exception_Class exc_class,
    struct _Unwind_Exception* exc_obj,
    struct _Unwind_Context* context,
    void* stop_parameter)
{
    if (actions & _UA_END_OF_STACK)
    {
        _Unwind_DeleteException(exc_obj);
        _exit(static_cast<int>(reinterpret_cast<intptr_t>(stop_parameter)));
    }
    return _URC_NO_REASON;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void exit_unwind_cleanup(_Unwind_Reason_Code reason, struct _Unwind_Exception* exc)
{
    // No cleanup needed. The exception object lives in a static thread_local variable.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" void exit(int exitCode)
{
    static const char className[] = "GNUCFOR";
    static thread_local _Unwind_Exception exc;

    static_assert(sizeof(className) == sizeof(exc.exception_class));
    memcpy(exc.exception_class, className, sizeof(exc.exception_class));

    exc.exception_cleanup = exit_unwind_cleanup;

    _Unwind_ForcedUnwind(&exc, exit_unwind_stop, reinterpret_cast<void*>(static_cast<intptr_t>(exitCode)));

    _exit(exitCode);
}
