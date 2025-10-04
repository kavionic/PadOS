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

#include <sys/pados_syscalls.h>

#include <Ptr/PtrTarget.h>
#include <Threads/Threads.h>

#include <assert.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PtrTargetNotifier::Lock()
{
    thread_id thread = __get_thread_id();
    
    if ( thread != m_LockerThread )
    {
        int expected = 0;
        while (!m_SpinLock.compare_exchange_weak(expected, 1))
        {
            __yield();
            expected = 0;
        }
        m_LockerThread = thread;
    }
    else
    {
        m_SpinlockNestCount++;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PtrTargetNotifier::Unlock()
{
    if ( m_SpinlockNestCount == 0 ) {
        m_LockerThread = -1;
        m_SpinLock = 0;
    } else {
        m_SpinlockNestCount--;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
void PtrTargetNotifier::ValidateRefCount( const char* source ) const
{
    assert(m_ReferenceCount.load() > 0);
}
#endif // NDEBUG

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WeakPtrTarget::WeakPtrTarget()
{
    m_Notifier.store(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WeakPtrTarget::~WeakPtrTarget()
{
    if (m_Notifier.load() != nullptr) {
        m_Notifier.load()->ClearObject();
        m_Notifier.load()->Release();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PtrTargetNotifier* WeakPtrTarget::GetNotifier() const
{
    if ( m_Notifier.load() != nullptr )
    {
        m_Notifier.load()->AddRef();
    }
    else
    {
        PtrTargetNotifier* notifier = new PtrTargetNotifier(this);

        PtrTargetNotifier* expected = nullptr;
        if ( !m_Notifier.compare_exchange_strong(expected, notifier) ) {
            notifier->Release();
        }
        m_Notifier.load()->AddRef();
    }
    return m_Notifier.load();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PtrTarget::PtrTarget()
{
    m_FirstMonitor = nullptr;
    m_ReferenceCount = e_PreInitRefCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PtrTarget::~PtrTarget()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PtrTarget::NotifyMonitors()
{
    while ( m_FirstMonitor != nullptr ) {
        m_FirstMonitor->ObjectDied();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \par Description:
///   Increment the reference count if it is larger than 0.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PtrTarget::AddPtrRefIfNotZero() const
{
    int refCount = m_ReferenceCount;
    for (;;)
    {
        if ( refCount > 0 )
        {
            if ( m_ReferenceCount.compare_exchange_weak(refCount, refCount + 1) ) {
                return true; // Ref-count was not 0 to begin with, and we managed to bring it to >1 before another thread changed it
            }
        }
        else
        {
            return false; // Ref-count is 0. The object is about to die.
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PtrTarget::HandleLastRefGone() const
{
    assert(GetPtrCount() >= 0);
    if (m_FirstMonitor == nullptr)
    {
        PtrTargetNotifier* notifier = m_Notifier.load();
        if (notifier != nullptr)
        {
            notifier->Lock();
            assert(m_ReferenceCount == 0);
            notifier->AddRef();
        }
        m_ReferenceCount++; // In case the destructor convert "this" to a smart pointer.
        const_cast<PtrTarget*>(this)->LastReferenceGone();
        if ( notifier != nullptr ) {
            notifier->Unlock();
            notifier->Release();
        }
        return true;
    }
    else
    {
        // We have monitors (SigWeakPtr) watching us.
        // We increase the reference count to make this a proper valid object
        // again before sending notifications. Then we send the notifications and
        // start the release process over again. This way we will not get into
        // problems if some of the monitors caused the object to be referenced with
        // a strong pointer again.

        m_ReferenceCount++;
        const_cast<PtrTarget*>(this)->NotifyMonitors();

        return false; // Check again in case one of the object monitors created a new reference to the object
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
void PtrTarget::ValidateRefCount(int minCount) const
{
    assert(( GetPtrCount() > minCount ));
}
#endif // NDEBUG

