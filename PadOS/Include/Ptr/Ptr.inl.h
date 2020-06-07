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

#include <assert.h>
#include "WeakPtr.h"

#define PTR_DELETED_MAGIC 0xdeaddead

///////////////////////////////////////////////////////////////////////////////
/// Default constructor.
/// \par Description:
///      Initialize the pointer with the nullptr value.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
Ptr<T>::Ptr()
{
    Initialize(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// nullptr constructor
/// \par Description:
///      This constructor exists to make it possible to use nullptr as a null
///      pointer rather than always having to give a full declaration
///      (like Ptr<ClassName>()).
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
  Ptr<T>::Ptr(std::nullptr_t)
{
    Initialize(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// Copy constructor.
/// \par Description:
///      Makes a copy of ptr and increase the reference count of the target
///      (unless ptr is a null pointer).
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
Ptr<T>::Ptr(const Ptr& ptr)
{
    Initialize(ptr.m_Object);
}

///////////////////////////////////////////////////////////////////////////////
/// Move constructor.
/// \par Description:
///      Move constructor
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
Ptr<T>::Ptr(Ptr&& ptr)
{
    m_Object = ptr.m_Object;
    ptr.m_Object = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// Casting constructor.
/// \par Description:
///      This constructor allow one pointer to be constructed from another
///      pointer as long as an implicit cast between the two is possible.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
Ptr<T>::Ptr(const Ptr<Y>& ptr)
{
    Initialize(ptr.m_Object);
}

///////////////////////////////////////////////////////////////////////////////
/// Casting constructor.
/// \par Description:
///      This constructor converts a WeakPtr to a Ptr. The type of the target
///      class does not need to be identical as long as an implicit cast is
///      possible.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
Ptr<T>::Ptr(const WeakPtr<Y>& ptr)
{
    Initialize(nullptr);

    if (ptr.m_Notifier != nullptr)
    {
        ptr.m_Notifier->Lock();

        if (ptr.m_Notifier->IsValid() && ptr.m_Object->AddPtrRefIfNotZero()) {
            m_Object = ptr.m_Object;
        }
        ptr.m_Notifier->Unlock();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Casting constructor.
/// \par Description:
///      This constructor converts a SigWeakPtr to a Ptr. The type of the target
///      class does not need to be identical as long as an implicit cast is
///      possible.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
Ptr<T>::Ptr(const SigWeakPtr<Y>& ptr)
{
    Initialize(ptr.m_Object);
}

///////////////////////////////////////////////////////////////////////////////
/// Casting constructor.
/// \par Description:
///      This constructor converts a NoPtr to a Ptr. The type of the target
///      class does not need to be identical as long as an implicit cast is
///      possible.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
Ptr<T>::Ptr(NoPtr<Y>& ptr)
{
    Initialize(&ptr);
}

///////////////////////////////////////////////////////////////////////////////
/// Destructor.
/// \par Description:
///      The destructor will decrease the reference count of the target.
///      If the reference count reached 0, the target will be deleted.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> inline
Ptr<T>::~Ptr()
{
    Set(nullptr);
#ifndef NDEBUG
    m_Object = (T*)uintptr_t(PTR_DELETED_MAGIC);
#endif // NDEBUG
}

///////////////////////////////////////////////////////////////////////////////
/// Reset the pointer to nullptr.
/// \par Description:
///      Calling Reset() will reset the pointer to nullptr, and decrease the
///      reference count of the target. If the reference count reached 0,
///      the target will be deleted.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
void Ptr<T>::Reset()
{
    Set(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// Dereferencing operator.
/// \par Description:
///      The dereferencing operator simply return the internal raw-pointer.
///      In optimized builds this should have the same cost as dereferencing
///      a raw-pointer.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
T* Ptr<T>::operator->() const
{
    return m_Object;
}

///////////////////////////////////////////////////////////////////////////////
/// Dereferencing operator.
/// \par Description:
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> inline
T& Ptr<T>::operator*() const
{
    return *m_Object;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
Ptr<T>& Ptr<T>::operator=(const Ptr& ptr)
{
    if (this != &ptr) {
        Set(ptr.m_Object);
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
Ptr<T>& Ptr<T>::operator=(Ptr&& ptr)
{
    if (this != &ptr) {
        Set( nullptr );
        std::swap(m_Object, ptr.m_Object);
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
Ptr<T>& Ptr<T>::operator=(const Ptr<Y>& ptr)
{
    Set(ptr.m_Object);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
Ptr<T>& Ptr<T>::operator=(const WeakPtr<Y>& ptr)
{
    *this = ptr.Lock();
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
Ptr<T>& Ptr<T>::operator=(const SigWeakPtr<Y>& ptr)
{
    Set(ptr.Get());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
bool Ptr<T>::operator==(const Ptr<Y>& ptr) const
{
    return m_Object == ptr.m_Object;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
bool Ptr<T>::operator!=(const Ptr<Y>& ptr) const
{
    return m_Object != ptr.m_Object;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
bool Ptr<T>::operator< (const Ptr<Y>& ptr) const
{
    return m_Object < ptr.m_Object;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool Ptr<T>::operator==(const WeakPtr<Y>& ptr) const
{
    return Get() == ptr.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool Ptr<T>::operator==(const SigWeakPtr<Y>& ptr) const
{
    return Get() == ptr.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool Ptr<T>::operator!=(const WeakPtr<Y>& ptr) const
{
    return Get() != ptr.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool Ptr<T>::operator!=(const SigWeakPtr<Y>& ptr) const
{
    return Get() != ptr.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool Ptr<T>::operator<(const WeakPtr<Y>& ptr) const
{
    return Get() < ptr.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool Ptr<T>::operator<(const SigWeakPtr<Y>& ptr) const
{
    return Get() < ptr.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
bool Ptr<T>::operator==(const T* obj) const
{
    return m_Object == obj;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
bool Ptr<T>::operator!=(const T* obj) const
{
    return m_Object != obj;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
bool Ptr<T>::operator< (const T* obj) const
{
    return m_Object < obj;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> inline
void Ptr<T>::Initialize(T* obj)
{
    m_Object = nullptr;
    if ( obj != nullptr ) {
        Set(obj);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
void Ptr<T>::Set(T* obj)
{
    if (m_Object != obj)
    {
#ifndef NDEBUG
        assert(m_Object != (T*)PTR_DELETED_MAGIC);
#endif // NDEBUG

        if (m_Object != nullptr) {
            T* tmp = m_Object;
            m_Object = nullptr;
            tmp->ReleasePtrRef();
        }
        if (obj != nullptr) {
            m_Object = obj;
            m_Object->AddPtrRef();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
T* Ptr<T>::Get() const
{
    return m_Object;
}
