// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 08.03.2018 17:55:02

#pragma once

#include <Kernel/Kernel.h>
#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include <System/System.h>
#include <System/ErrorCodes.h>
#include <Kernel/KWaitableObject.h>
#ifdef PADOS_MODULE_POSIX_SIGNALS
#include <Kernel/KPosixSignals.h>
#endif // PADOS_MODULE_POSIX_SIGNALS


namespace kernel
{
class KThreadCB;
class KObjectWaitGroup;

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



class KNamedObject : public PtrTarget, public KWaitableObject
{
public:
    static void InitializeStatics();
    KNamedObject(const char* name, KNamedObjectType type);
    virtual ~KNamedObject();

    bool DebugValidate() const;

    KNamedObjectType GetType() const noexcept { return m_Type; }
    void             SetName(const char* name);
    const char*      GetName() const noexcept { return m_Name; }

    virtual void     SetHandle(int32_t handle) noexcept { m_Handle = handle; }
    int32_t          GetHandle() const noexcept         { return m_Handle; }

    static PErrorCode        RegisterObject(handle_id& outHandle, Ptr<KNamedObject> object);
    static handle_id         RegisterObject_trw(Ptr<KNamedObject> object);
    static void              FreeHandle_trw(int32_t handle);
    static void              FreeHandle_trw(int32_t handle, KNamedObjectType type);

    static Ptr<KNamedObject> GetObject(int32_t handle, KNamedObjectType type);
    static Ptr<KNamedObject> GetObject_trw(int32_t handle, KNamedObjectType type);
    
    template<typename T>
    static Ptr<T>            GetObject(int32_t handle) { return ptr_static_cast<T>(GetObject(handle, T::ObjectType)); }
    template<typename T>
    static Ptr<T>            GetObject_trw(int32_t handle) { return ptr_static_cast<T>(GetObject_trw(handle, T::ObjectType)); }
    
    static Ptr<KNamedObject> GetAnyObject(int32_t handle);
    static Ptr<KNamedObject> GetAnyObject_trw(int32_t handle);

    template<typename TObjectType, typename CALLBACK, typename... ARGS>
    static void ForwardToHandle_trw(int handle, CALLBACK callback, ARGS&&... args)
    {
        Ptr<TObjectType> object = GetObject_trw<TObjectType>(handle);
        (ptr_raw_pointer_cast(object)->*callback)(std::forward<ARGS>(args)...);
    }

    template<typename TObjectType, typename TReturnType, typename CALLBACK, typename... ARGS>
    static TReturnType ForwardToHandle(int handle, const TReturnType& invalidHandleReturnValue, CALLBACK callback, ARGS&&... args)
    {
        Ptr<TObjectType> object = GetObject<TObjectType>(handle);
        if (object != nullptr) {
            return (ptr_raw_pointer_cast(object)->*callback)(std::forward<ARGS>(args)...);
        } else {
            return invalidHandleReturnValue;
        }
    }

    template<typename TObjectType, typename CALLBACK, typename... ARGS>
    static PErrorCode ForwardToHandleRestartable(int handle, const PErrorCode& invalidHandleReturnValue, CALLBACK callback, ARGS&&... args)
    {
#ifdef PADOS_MODULE_POSIX_SIGNALS
        for (;;)
        {
            const PErrorCode result = ForwardToHandle<TObjectType>(handle, invalidHandleReturnValue, callback, std::forward<ARGS>(args)...);
            if (result != PErrorCode::RestartSyscall) [[likely]] {
                return result;
            }
            kforce_process_signals();
        }
#else
        return ForwardToHandle<TObjectType>(handle, invalidHandleReturnValue, callback, std::forward<ARGS>(args)...);
#endif // PADOS_MODULE_POSIX_SIGNALS
    }

    template<typename T, typename CALLBACK, typename... ARGS>
    static status_t ForwardToHandleVoid(int handle, CALLBACK callback, ARGS&&... args)
    {
        Ptr<T> object = GetObject<T>(handle);
        if (object != nullptr)
        {
            (ptr_raw_pointer_cast(object)->*callback)(std::forward<ARGS>(args)...);
            return 0;
        }
        else
        {
            set_last_error(EINVAL);
            return -1;
        }
    }

    template<typename T, typename RETURN_TYPE, typename CALLBACK, typename... ARGS>
    static RETURN_TYPE ForwardToHandleBool(int handle, RETURN_TYPE falseValue, RETURN_TYPE trueValue, CALLBACK callback, ARGS&&... args)
    {
        return ForwardToHandle<T>(handle, false, callback, std::forward<ARGS>(args)...) ? trueValue : falseValue;
    }

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

handle_id kduplicate_handle_trw(handle_id handle);
void kdelete_handle_trw(handle_id handle);

} // namespace
