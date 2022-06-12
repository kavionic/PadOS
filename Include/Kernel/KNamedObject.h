// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "System/System.h"
#include "Utils/IntrusiveList.h"

enum class ObjectWaitMode : uint8_t
{
    Read,
    Write,
    ReadWrite
};

namespace kernel
{
class KThreadCB;

enum class KNamedObjectType
{
    Generic,
    Thread,
    Semaphore,
    Mutex,
    ConditionVariable,
	ObjectWaitGroup,
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

    TimeValMicros                   m_ResumeTime;
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
    static void InitializeStatics();
    KNamedObject(const char* name, KNamedObjectType type);
    virtual ~KNamedObject();

    bool DebugValidate() const;

    KNamedObjectType GetType() const           { return m_Type; }
    const char*      GetName() const           { return m_Name; }

    void             SetHandle(int32_t handle) { m_Handle = handle; }
    int32_t          GetHandle() const         { return m_Handle; }

    static int32_t           RegisterObject(Ptr<KNamedObject> object);
    static bool              FreeHandle(int32_t handle);
	static bool              FreeHandle(int32_t handle, KNamedObjectType type);

    static Ptr<KNamedObject> GetObject(int32_t handle, KNamedObjectType type);
    template<typename T>
    static Ptr<T>            GetObject(int32_t handle) { return ptr_static_cast<T>(GetObject(handle, T::ObjectType)); }
	static Ptr<KNamedObject> GetAnyObject(int32_t handle);

    KThreadWaitList& GetWaitQueue() { return m_WaitQueue; }

    // If access would block, add to wait-list and return true. If not, don't add to any list and return false.
    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode);

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

#if DEBUG_HANDLE_OBJECT_LIST
    static size_t s_ObjectCount;
    static KNamedObject* s_FirstObject;
    KNamedObject* m_PrevObject = nullptr;
    KNamedObject* m_NextObject = nullptr;
#endif // DEBUG_HANDLE_OBJECT_LIST
    uint32_t         m_Magic = MAGIC;
    char             m_Name[OS_NAME_LENGTH];
    KNamedObjectType m_Type;
    int32_t          m_Handle = -1;

    KNamedObject(const KNamedObject&);
    KNamedObject& operator=(const KNamedObject&);
};


} // namespace
