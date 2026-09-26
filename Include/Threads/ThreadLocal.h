// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.03.2018 02:48:16

#pragma once

#include <map>

#include <sys/pados_syscalls.h>
#include <PadOS/ThreadLocal.h>
#include <Threads/Threads.h>
#include <Threads/Mutex.h>
#include <Kernel/KThreadCB.h>

template<typename T>
class PThreadLocal
{
public:
    PThreadLocal() {
        thread_local_create_key(m_Slot, TLSDestructor);
    }
    ~PThreadLocal() { thread_local_delete_key(m_Slot); }

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

    PThreadLocal(const PThreadLocal &) = delete;
    PThreadLocal& operator=(const PThreadLocal &) = delete;
};

template<typename T>
class PThreadLocal<T*>
{
public:
    PThreadLocal() {
        thread_local_create_key(&m_Slot, nullptr);
    }
    ~PThreadLocal() { thread_local_delete_key(m_Slot); }

    void Set(T* object ) {
        thread_local_set(m_Slot, object);
    }
    T* Get() {
        return static_cast<T*>(thread_local_get(m_Slot));
    }

private:
    tls_id m_Slot;

    PThreadLocal(const PThreadLocal &) = delete;
    PThreadLocal& operator=(const PThreadLocal &) = delete;
};

class PThreadLocalSlotManager
{
public:
    PThreadLocalSlotManager();

    static PThreadLocalSlotManager& Get();

    PErrorCode  AllocSlot(tls_id& outKey, TLSDestructor_t destructor);
    PErrorCode  FreeSlot(tls_id slot);

    void*       GetSlot(tls_id key);
    PErrorCode  SetSlot(tls_id key, const void* value);

    void ThreadTerminated();

private:
    tls_id FindFreeIndex() const;

    static PThreadLocalSlotManager* s_Instance;

    PMutex          m_Mutex;
    uint32_t        m_AllocationMap[(THREAD_MAX_TLS_SLOTS + 31) / 32];
    std::vector<TLSDestructor_t> m_Destructors;
    std::map<thread_id, std::vector<void*>*> m_ThreadSlotsMap;

    thread_local static std::vector<void*>* s_ThreadSlots;
};
