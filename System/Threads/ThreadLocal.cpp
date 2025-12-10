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
// Created: 11.03.2018 02:48:15

#include <Threads/ThreadLocal.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>

ThreadLocalSlotManager*          ThreadLocalSlotManager::s_Instance;
thread_local std::vector<void*>* ThreadLocalSlotManager::s_ThreadSlots;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ThreadLocalSlotManager::ThreadLocalSlotManager() : m_Mutex("tls_mgr", PEMutexRecursionMode::PEMutexRecursionMode_RaiseError)
{
    assert(s_Instance == nullptr);
    s_Instance = this;

    memset(m_AllocationMap, 0, sizeof(m_AllocationMap));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ThreadLocalSlotManager& ThreadLocalSlotManager::Get()
{
    static ThreadLocalSlotManager instance;
    return instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ThreadLocalSlotManager::AllocSlot(tls_id& outKey, TLSDestructor_t destructor)
{
    assert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    tls_id index = FindFreeIndex();

    if (index == INVALID_INDEX) {
        return PErrorCode::TryAgain;
    }

    try
    {
        if (m_Destructors.size() <= index) {
            m_Destructors.resize(index + 1);
        }
        m_Destructors[index] = destructor;
    }
    PERROR_CATCH_RET_CODE;

    m_AllocationMap[index / 32] |= 1 << (index % 32);


    outKey = index;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ThreadLocalSlotManager::FreeSlot(tls_id slot)
{
    assert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    size_t index = size_t(slot);

    if (slot >= THREAD_MAX_TLS_SLOTS)
    {
        return PErrorCode::InvalidArg;
    }
    for (const auto& threadSlots : m_ThreadSlotsMap)
    {
        if (index < threadSlots.second->size()) {
            (*threadSlots.second)[index] = nullptr;
        }
    }
    if (index < m_Destructors.size()) {
        m_Destructors[index] = nullptr;
    }
    m_AllocationMap[index / 32] &= ~(1 << (index % 32));
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* ThreadLocalSlotManager::GetSlot(tls_id key)
{
    if (s_ThreadSlots == nullptr || size_t(key) >= s_ThreadSlots->size()) {
        return nullptr;
    }
    return s_ThreadSlots->at(key);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ThreadLocalSlotManager::SetSlot(tls_id key, const void* value)
{
    if (size_t(key) >= THREAD_MAX_TLS_SLOTS) {
        return PErrorCode::InvalidArg;
    }
    if (s_ThreadSlots == nullptr || s_ThreadSlots->size() <= key)
    {
        if (value == nullptr) {
            return PErrorCode::Success;
        }
        std::vector<void*>* slots = nullptr;
        try
        {
            if (s_ThreadSlots == nullptr)
            {
                slots = new std::vector<void*>(key + 1);

                m_ThreadSlotsMap[get_thread_id()] = slots;
                s_ThreadSlots = slots;
            }
            else
            {
                s_ThreadSlots->resize(key + 1);
            }
        }
        PERROR_CATCH_RET([&slots](const std::exception& exc, PErrorCode error) { delete slots; return error; });
    }
    s_ThreadSlots->at(key) = (void*)value;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ThreadLocalSlotManager::ThreadTerminated()
{
    std::vector<void*>* slots = s_ThreadSlots;

    if (slots != nullptr)
    {
        bool slotsDestructed;
        do
        {
            slotsDestructed = false;

            for (size_t i = 0; i < slots->size(); ++i)
            {
                void* slotValue = slots->at(i);
                if (slotValue != nullptr)
                {
                    slots->at(i) = nullptr;

                    TLSDestructor_t destructor = nullptr;

                    CRITICAL_BEGIN(m_Mutex)
                    {
                        if (i < m_Destructors.size()) {
                            destructor = m_Destructors[i];
                        }
                    } CRITICAL_END;

                    if (destructor != nullptr)
                    {
                        destructor(slotValue);
                        slotsDestructed = true;
                    }
                }
            }
        } while (slotsDestructed);
    }

    delete slots;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

tls_id ThreadLocalSlotManager::FindFreeIndex() const
{
    for (uint32_t i = 0; i < ARRAY_COUNT(m_AllocationMap); ++i)
    {
        if (m_AllocationMap[i] != ~uint32_t(0))
        {
            for (uint32_t j = 0, mask = 1; j < 32; ++j, mask <<= 1)
            {
                if ((m_AllocationMap[i] & mask) == 0)
                {
                    const size_t index = i * 32 + j;
                    if (index < THREAD_MAX_TLS_SLOTS)
                    {
                        return index;
                    }
                }
            }
        }
    }
    return INVALID_INDEX;
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode thread_local_create_key(tls_id* outKey, TLSDestructor_t destructor)
{
    return ThreadLocalSlotManager::Get().AllocSlot(*outKey, destructor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode thread_local_delete_key(tls_id key)
{
    return ThreadLocalSlotManager::Get().FreeSlot(key);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode thread_local_set(tls_id key, const void* value)
{
    return ThreadLocalSlotManager::Get().SetSlot(key, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* thread_local_get(tls_id key)
{
    return ThreadLocalSlotManager::Get().GetSlot(key);
}
