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
// Created: 11.03.2018 01:23:12

#include "Platform.h"

#include <string.h>
#include <sys/errno.h>

#include "KProcess.h"
#include "KThreadCB.h"
#include "Kernel.h"
#include "Scheduler.h"
#include "System/Utils/Utils.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::KProcess()
{
    memset(m_TLSDestructors, 0, sizeof(m_TLSDestructors));
    memset(m_TLSAllocationMap, 0, sizeof(m_TLSAllocationMap));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::~KProcess()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::ThreadQuit(KThreadCB* thread)
{
    for (uint32_t i = 0; i < ARRAY_COUNT(m_TLSAllocationMap); ++i)
    {
        if (m_TLSAllocationMap[i] != 0)
        {
            for (uint32_t j = 0, mask = 1; j < 32; ++j, mask <<= 1)
            {
                if (m_TLSAllocationMap[i] & mask)
                {
                    int index = i * 32 + j;
                    if (index < THREAD_MAX_TLS_SLOTS && m_TLSDestructors[index] != nullptr) {
                        m_TLSDestructors[index](((void**)thread->m_StackBuffer)[index]);
                    } else {
                        break;
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KProcess::AllocTLSSlot(TLSDestructor_t destructor)
{
    for (uint32_t i = 0; i < ARRAY_COUNT(m_TLSAllocationMap); ++i)
    {
        if (m_TLSAllocationMap[i] != ~uint32_t(0))
        {
            for (uint32_t j = 0, mask = 1; j < 32; ++j, mask <<= 1)
            {
                if ((m_TLSAllocationMap[i] & mask) == 0)
                {
                    int index = i * 32 + j;
                    if (index < THREAD_MAX_TLS_SLOTS) {
                        m_TLSAllocationMap[i] |= mask;
                        m_TLSDestructors[index] = destructor;
                        return index;
                    } else {
                        set_last_error(EAGAIN);
                        return -1;
                    }
                }
            }
        }
    }
    set_last_error(EAGAIN);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KProcess::FreeTLSSlot(int slot)
{
    if (slot >= 0 && slot < THREAD_MAX_TLS_SLOTS) {
        m_TLSAllocationMap[slot/32] &= ~(1<<(slot % 32));
        return true;
    } else {
        set_last_error(EINVAL);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int alloc_thread_local_storage(TLSDestructor_t destructor)
{
    return gk_CurrentProcess->AllocTLSSlot(destructor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int delete_thread_local_storage(int slot)
{
    return gk_CurrentProcess->FreeTLSSlot(slot) ? 0 : -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int set_thread_local(int slot, void* value)
{
    if (slot >= 0 && slot < THREAD_MAX_TLS_SLOTS) {
        KThreadCB* thread = gk_CurrentThread;
        ((void**)thread->m_StackBuffer)[slot] = value;
        return 0;
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* get_thread_local(int slot)
{
    if (slot >= 0 && slot < THREAD_MAX_TLS_SLOTS) {
        KThreadCB* thread = gk_CurrentThread;
        return ((void**)thread->m_StackBuffer)[slot];
    } else {
        set_last_error(EINVAL);
        return nullptr;
    }
}
