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

#pragma once

#include "Ptr.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> class WeakPtr
{
public:
    typedef T element_type; 

    WeakPtr();
    WeakPtr(std::nullptr_t null);
    WeakPtr(const WeakPtr& ptr);
    WeakPtr(WeakPtr&& ptr);
    template <class Y> WeakPtr(const WeakPtr<Y>& ptr);
    template <class Y> WeakPtr(const Ptr<Y>& ptr);
    template <class Y> WeakPtr(const SigWeakPtr<Y>& ptr);

    ~WeakPtr();

    void Reset();

    Ptr<T> Lock() const { return Ptr<T>(*this); }

    WeakPtr& operator=(const WeakPtr& ptr); 
    template <class Y> WeakPtr& operator=(const WeakPtr<Y>& ptr);
    template <class Y> WeakPtr& operator=(const Ptr<Y>& ptr);
    template <class Y> WeakPtr& operator=(const SigWeakPtr<Y>& ptr);

    template <class Y> bool operator==(const WeakPtr<Y>& ptr) const;
    template <class Y> bool operator!=(const WeakPtr<Y>& ptr) const;
    template <class Y> bool operator<(const WeakPtr<Y>& ptr) const;

    template <class Y> bool operator==(const Ptr<Y>& ptr) const;
    template <class Y> bool operator!=(const Ptr<Y>& ptr) const;
    template <class Y> bool operator<(const Ptr<Y>& ptr) const;

    template <class Y> bool operator==(const SigWeakPtr<Y>& ptr) const;
    template <class Y> bool operator!=(const SigWeakPtr<Y>& ptr) const;
    template <class Y> bool operator<(const SigWeakPtr<Y>& ptr) const;
  
private:
    template<class Y> friend class Ptr;
    template<class Y> friend class WeakPtr;
    template<class Y> friend class SigWeakPtr;

    template<class Y> friend WeakPtr<Y> ptr_weak_cast(Y*);
    template<class Y> friend Y* ptr_raw_pointer_cast (const WeakPtr<Y>&);

    template <class Y> void Set(Y* obj);
    T* Get() const;
  
    PtrTargetNotifier* m_Notifier;
    T*                 m_Object;

};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class T> inline
WeakPtr<T> ptr_weak_cast(T* obj)
{
    WeakPtr<T> ptr;
    ptr.Set(obj);
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class T> inline
T* ptr_raw_pointer_cast(const WeakPtr<T>& src)
{
    return src.Get();
}


#include "WeakPtr.inl.h"
