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
// Created: 11.03.2018 02:48:16

#pragma once


#include "Threads/Threads.h"

template<typename T>
class ThreadLocal
{
public:
    ThreadLocal() {
        m_Slot = alloc_thread_local_storage(TLSDestructor);
    }
    ~ThreadLocal() { delete_thread_local_storage(m_Slot); }

    void Set(const T& object )
    {
        T* buffer = static_cast<T*>(get_thread_local(m_Slot));
        if (buffer != nullptr) {
            buffer = new T(object);
            set_thread_local(m_Slot, buffer);
        } else {
            *buffer = object;
        }
    }
    T& Get() {
        T* buffer = static_cast<T*>(get_thread_local(m_Slot));
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
        m_Slot = alloc_thread_local_storage(nullptr);
    }
    ~ThreadLocal() { delete_thread_local_storage(m_Slot); }

    void Set(T* object ) {
        set_thread_local(m_Slot, object);
    }
    T* Get() {
        return static_cast<T*>(get_thread_local(m_Slot));
    }

private:
    int m_Slot;

    ThreadLocal(const ThreadLocal &) = delete;
    ThreadLocal& operator=(const ThreadLocal &) = delete;
};
