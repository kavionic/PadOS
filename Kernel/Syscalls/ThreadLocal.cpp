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


using namespace os;
using namespace kernel;

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

tls_id sys_thread_local_create_key(TLSDestructor_t destructor)
{
    return gk_CurrentProcess->AllocTLSSlot(destructor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_thread_local_delete_key(tls_id slot)
{
    return gk_CurrentProcess->FreeTLSSlot(slot) ? 0 : -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_thread_local_set(tls_id slot, const void* value)
{
    if (slot >= 0 && slot < THREAD_MAX_TLS_SLOTS)
    {
        KThreadCB* thread = gk_CurrentThread;
        thread->m_ThreadLocalSlots[slot] = const_cast<void*>(value);
        return 0;
    }
    else
    {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

volatile void* p0;
volatile void* p1;
volatile void* p2;
volatile int i0;
volatile int i1;

void* sys_thread_local_get(tls_id slot)
{
    if (slot >= 0 && slot < THREAD_MAX_TLS_SLOTS)
    {
        KThreadCB* thread = gk_CurrentThread;
        return thread->m_ThreadLocalSlots[slot];
    }
    else
    {
        set_last_error(EINVAL);
        return nullptr;
    }
}


} // extern "C"
