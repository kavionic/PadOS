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
// Created: 31.08.2025 17:00

#include <sys/pados_syscalls.h>

#include <Kernel/Scheduler.h>
#include <Kernel/KProcess.h>
#include <Kernel/Syscalls.h>

using namespace os;
using namespace kernel;


extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_local_create_key(tls_id* outKey, TLSDestructor_t destructor)
{
    return gk_CurrentProcess->AllocTLSSlot(*outKey, destructor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_local_delete_key(tls_id key)
{
    return gk_CurrentProcess->FreeTLSSlot(key);
}


} // extern "C"
