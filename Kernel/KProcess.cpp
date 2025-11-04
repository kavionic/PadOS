// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include <System/Platform.h>

#include <string.h>
#include <sys/errno.h>

#include <Kernel/KHandleArray.h>
#include <Kernel/KProcess.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Threads/Threads.h>
#include <Utils/Utils.h>

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::KProcess() : m_TLSMutex("process_tls_mutex", PEMutexRecursionMode_RaiseError)
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
    std::vector<std::pair<TLSDestructor_t, void*>> destructors;

    CRITICAL_BEGIN(m_TLSMutex)
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
                        if (index < thread->m_ControlBlock->TLSSlotCount)
                        {
                            if (m_TLSDestructors[index] != nullptr) {
                                destructors.emplace_back(m_TLSDestructors[index], thread->m_ControlBlock->TLSSlots[index]);
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }
    } CRITICAL_END;

    for (const auto& i : destructors)
    {
        i.first(i.second);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KProcess::AllocTLSSlot(tls_id& outKey, TLSDestructor_t destructor)
{
    kassert(!m_TLSMutex.IsLocked());
    CRITICAL_SCOPE(m_TLSMutex);

    for (uint32_t i = 0; i < ARRAY_COUNT(m_TLSAllocationMap); ++i)
    {
        if (m_TLSAllocationMap[i] != ~uint32_t(0))
        {
            for (uint32_t j = 0, mask = 1; j < 32; ++j, mask <<= 1)
            {
                if ((m_TLSAllocationMap[i] & mask) == 0)
                {
                    const size_t index = i * 32 + j;
                    if (index < THREAD_MAX_TLS_SLOTS)
                    {
                        m_TLSAllocationMap[i] |= mask;
                        m_TLSDestructors[index] = destructor;

                        for (Ptr<const KThreadCB> thread = get_thread_table().GetNext(INVALID_HANDLE, [](Ptr<const KThreadCB> thread) { return thread->m_State != ThreadState_Deleted; });
                            thread != nullptr;
                            thread = get_thread_table().GetNext(thread->GetHandle(), [](Ptr<const KThreadCB> thread) { return thread->m_State != ThreadState_Deleted; }))
                        {
                            thread->m_ControlBlock->TLSSlots[index] = nullptr;
                        }
                        outKey = index;
                        return PErrorCode::Success;
                    }
                    else
                    {
                        return PErrorCode::TryAgain;
                    }
                }
            }
        }
    }
    return PErrorCode::TryAgain;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KProcess::FreeTLSSlot(tls_id slot)
{
    kassert(!m_TLSMutex.IsLocked());
    CRITICAL_SCOPE(m_TLSMutex);

    if (slot >= 0 && slot < THREAD_MAX_TLS_SLOTS)
    {
        m_TLSAllocationMap[slot/32] &= ~(1<<(slot % 32));
        return PErrorCode::Success;
    }
    else
    {
        return PErrorCode::InvalidArg;
    }
}

} // namespace kernel
