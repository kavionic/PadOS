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
// Created: 11.03.2018 02:48:16

#pragma once

#include <sys/pados_syscalls.h>
#include <PadOS/ThreadLocal.h>
#include <Threads/Threads.h>
#include <Threads/Mutex.h>
#include <Kernel/KThreadCB.h>

template<typename T>
class ThreadLocal
{
public:
    ThreadLocal() {
        thread_local_create_key(m_Slot, TLSDestructor);
    }
    ~ThreadLocal() { thread_local_delete_key(m_Slot); }

    void Set(const T& object )
    {
        T* buffer = static_cast<T*>(thread_local_get(m_Slot));
        if (buffer == nullptr)
        {
            buffer = new T(object);
            thread_local_set(m_Slot, buffer);
        }
        else
        {
            *buffer = object;
        }
    }
    T& Get() {
        T* buffer = static_cast<T*>(thread_local_get(m_Slot));
        if (buffer != nullptr) {
            return *static_cast<T*>(buffer);
        } else {
            return T();
        }
    }

private:
    static void TLSDestructor(void* data)
    {
        delete static_cast<T*>(data);
    }
    int m_Slot;

    ThreadLocal(const ThreadLocal &) = delete;
    ThreadLocal& operator=(const ThreadLocal &) = delete;
};

template<typename T>
class ThreadLocal<T*>
{
public:
    ThreadLocal() {
        thread_local_create_key(&m_Slot, nullptr);
    }
    ~ThreadLocal() { thread_local_delete_key(m_Slot); }

    void Set(T* object ) {
        thread_local_set(m_Slot, object);
    }
    T* Get() {
        return static_cast<T*>(thread_local_get(m_Slot));
    }

private:
    tls_id m_Slot;

    ThreadLocal(const ThreadLocal &) = delete;
    ThreadLocal& operator=(const ThreadLocal &) = delete;
};

class ThreadLocalSlotManager
{
public:
    ThreadLocalSlotManager();

    static ThreadLocalSlotManager& Get();

    PErrorCode  AllocSlot(tls_id& outKey, TLSDestructor_t destructor);
    PErrorCode  FreeSlot(tls_id slot);

    void*       GetSlot(tls_id key);
    PErrorCode  SetSlot(tls_id key, const void* value);

    void ThreadTerminated();

private:
    tls_id FindFreeIndex() const;

    static ThreadLocalSlotManager* s_Instance;

    PMutex          m_Mutex;
    uint32_t        m_AllocationMap[(THREAD_MAX_TLS_SLOTS + 31) / 32];
    std::vector<TLSDestructor_t> m_Destructors;
    std::map<thread_id, std::vector<void*>*> m_ThreadSlotsMap;

    thread_local static std::vector<void*>* s_ThreadSlots;
};
