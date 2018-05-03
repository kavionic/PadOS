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
// Created: 08.03.2018 17:55:02

#pragma once

#include <sys/errno.h>

#include "Kernel.h"
#include "System/Ptr/PtrTarget.h"
#include "System/System.h"
#include "System/Utils/IntrusiveList.h"

namespace kernel
{
class KThreadCB;

enum class KNamedObjectType
{
    Thread,
    Semaphore,
    Mutex,
    ConditionVariable,
    MessagePort,
};

struct KThreadWaitNode
{
    bool Detatch()
    {
        if (m_List != nullptr) {
            m_List->Remove(this);
            return true;
        } else {
            return false;
        }
    }

    bigtime_t                       m_ResumeTime = 0;
    KThreadCB*                      m_Thread = nullptr;
    int                             m_ReturnCode = 0;
    bool                            m_TargetDeleted = false;
    KThreadWaitNode*                m_Next = nullptr;
    KThreadWaitNode*                m_Prev = nullptr;
    IntrusiveList<KThreadWaitNode>* m_List = nullptr;
};

typedef IntrusiveList<KThreadWaitNode> KThreadWaitList;

class KNamedObject : public PtrTarget
{
public:
    KNamedObject(const char* name, KNamedObjectType type);
    virtual ~KNamedObject();

    bool DebugValidate() const { if (m_Magic != MAGIC) { panic("KnamedObject has been overwritten!\n"); return false; } return true; }

    KNamedObjectType GetType() const           { return m_Type; }
    const char*      GetName() const           { return m_Name; }

    void             SetHandle(int32_t handle) { m_Handle = handle; }
    int32_t          GetHandle() const         { return m_Handle; }

    static int32_t           RegisterObject(Ptr<KNamedObject> object);
    static bool              FreeHandle(int32_t handle, KNamedObjectType type);

    static Ptr<KNamedObject> GetObject(int32_t handle, KNamedObjectType type);
    template<typename T>
    static Ptr<T>            GetObject(int32_t handle) { return ptr_static_cast<T>(GetObject(handle, T::ObjectType)); }

    KThreadWaitList& GetWaitQueue() { return m_WaitQueue; }


    template<typename T, typename CALLBACK, typename... ARGS>
    static status_t ForwardToHandle(int handle, CALLBACK callback, ARGS&&... args)
    {
        Ptr<T> object = ptr_static_cast<T>(GetObject(handle, T::ObjectType));
        if (object != nullptr) {
            return (ptr_raw_pointer_cast(object)->*callback)(args...);
        } else {
            set_last_error(EINVAL);
            return -1;
        }
    }

    template<typename T, typename CALLBACK, typename... ARGS>
    static status_t ForwardToHandleVoid(int handle, CALLBACK callback, ARGS&&... args)
    {
        Ptr<T> object = ptr_static_cast<T>(GetObject(handle, T::ObjectType));
        if (object != nullptr) {
            (ptr_raw_pointer_cast(object)->*callback)(args...);
            return 0;
        } else {
            set_last_error(EINVAL);
            return -1;
        }
    }

    template<typename T, typename CALLBACK, typename... ARGS>
    static status_t ForwardToHandleBoolToInt(int handle, CALLBACK callback, ARGS&&... args)
    {
        return ForwardToHandle<T>(handle, callback, args...) ? 0 : -1;
    }

protected:
    KThreadWaitList m_WaitQueue;

private:
    static const uint32_t MAGIC = 0x1ee3babe;
    uint32_t         m_Magic = MAGIC;
    char             m_Name[OS_NAME_LENGTH];
    KNamedObjectType m_Type;
    int32_t          m_Handle = -1;

    KNamedObject(const KNamedObject&);
    KNamedObject& operator=(const KNamedObject&);
};


} // namespace
