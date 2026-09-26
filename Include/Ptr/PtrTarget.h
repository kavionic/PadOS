// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <atomic>
#include <System/Types.h>

class PtrTargetNotifier;

template <class T> class Ptr;
template <class T> class WeakPtr;
template <class T> class SigWeakPtr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PtrTargetMonitor
{
public:
    PtrTargetMonitor() : m_Prev(nullptr), m_Next(nullptr) {}
    virtual ~PtrTargetMonitor() {}

    virtual void ObjectDied() = 0;

private:
    friend class PtrTarget;

    PtrTargetMonitor* m_Prev;
    PtrTargetMonitor* m_Next;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class WeakPtrTarget
{
public:
    WeakPtrTarget();
    virtual ~WeakPtrTarget();

private:
    friend class PtrTarget;
    friend class PtrTargetNotifier;

    template<class Y> friend class WeakPtr;
    template<class Y> friend class SigWeakPtr;

    PtrTargetNotifier* GetNotifier() const;

    mutable std::atomic<PtrTargetNotifier*> m_Notifier;

    // Disabled operators:
    WeakPtrTarget(const WeakPtrTarget&) = delete;
    WeakPtrTarget& operator=(const WeakPtrTarget&) = delete;
};

////////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
////////////////////////////////////////////////////////////////////////////////

class PtrTargetNotifier
{
public:
    explicit PtrTargetNotifier(const WeakPtrTarget* obj)
    {
        m_Object = const_cast<WeakPtrTarget*>(obj);
        m_ReferenceCount = 1;
        m_SpinLock = 0;
    }

    bool IsValid() const { return m_Object != nullptr; }

    void ClearObject() { m_Object = nullptr; }

    void AddRef()
    {
#if !defined(NDEBUG)
        ValidateRefCount("PtrTargetNotified_c::AddRef()");
#endif // !defined(NDEBUG)
        m_ReferenceCount++;
    }

    void Release()
    {
#if !defined(NDEBUG)
        ValidateRefCount("PtrTargetNotified_c::Release()");
#endif // !defined(NDEBUG)
        if( --m_ReferenceCount == 0 ) { delete this; }
    }

    void Lock();
    void Unlock();

private:
    template<class Y> friend class Ptr;

    ~PtrTargetNotifier() {}

#if !defined(NDEBUG)
    void ValidateRefCount(const char* source) const;
#endif // !defined(NDEBUG)

    std::atomic_int    m_ReferenceCount;
    std::atomic_int    m_SpinLock;
    volatile int       m_SpinlockNestCount = 0;
    volatile thread_id m_LockerThread = -1;
    WeakPtrTarget*     m_Object;
};


///////////////////////////////////////////////////////////////////////////////
/// \par Description:
/// This is the base class you will need to inherit from in order to use
/// the Ptr and SigWeakPtr smartpointers. It handle the reference
/// counting.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PtrTarget : public WeakPtrTarget
{
public:
    PtrTarget();
    virtual ~PtrTarget();

    int GetPtrCount() const
    {
        return m_ReferenceCount;
    }

    // Return true if the object was deleted, or false to retain it.
    virtual bool LastReferenceGone() {
        delete this;
        return true;
    }
    void NotifyMonitors();

protected:
    bool HandleLastRefGone() const;

private:
    template<class Y> friend class Ptr;
    template<class Y> friend class WeakPtr;
    template<class Y> friend class SigWeakPtr;
    template<class Y> friend class NoPtr;
    template<class Y> friend Ptr<Y> ptr_new_cast(Y*);
    template<class Y> friend Ptr<Y> ptr_tmp_cast(Y*);
    template<class Y> friend Ptr<Y> ptr_lock_cast(Y*);

    enum { e_PreInitRefCount = 1000000000, e_ManagedRefCount = e_PreInitRefCount + 1000000, e_UnmanagedRefCount = e_ManagedRefCount + 1000000 };  

    void AddPtrRef() const
    {
        m_ReferenceCount++;
    }
    void AddPtrRef(int value)
    {
        m_ReferenceCount += value;
    }

    bool AddPtrRefIfNotZero() const;

    void ReleasePtrRef() const
    {
        for (;;)
        { // If we have any SigWeakPtr pointing at us we will have to retry the release after sending notifications.
            if (--m_ReferenceCount > 0) { return; }
            if (HandleLastRefGone()) { break; }
        }
    }

    void DisableReferenceCounting()
    {
        m_ReferenceCount.fetch_sub(e_PreInitRefCount - e_UnmanagedRefCount);
    }

#if !defined(NDEBUG)
     void ValidateRefCount(int minCount) const;
#endif // !defined(NDEBUG)

    void AddMonitor(PtrTargetMonitor* monitor) const
    {
        monitor->m_Prev = nullptr;
        monitor->m_Next = m_FirstMonitor;
        if (m_FirstMonitor != nullptr)
        {
            m_FirstMonitor->m_Prev = monitor;
        }
        m_FirstMonitor = monitor;
    }

    void RemoveMonitor(PtrTargetMonitor* monitor) const
    {
        if (monitor->m_Next != nullptr) {
            monitor->m_Next->m_Prev = monitor->m_Prev;
        }
        if (monitor->m_Prev != nullptr) {
            monitor->m_Prev->m_Next = monitor->m_Next;
        } else {
            m_FirstMonitor = monitor->m_Next;
        }
        monitor->m_Prev = nullptr;
        monitor->m_Next = nullptr;
    }

    mutable std::atomic_int   m_ReferenceCount;
    mutable PtrTargetMonitor* m_FirstMonitor;
    
    PtrTarget(const PtrTarget&) = delete;
    PtrTarget& operator=(const PtrTarget&) = delete;
};
