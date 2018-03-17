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

#include "PtrTarget.h"
template <class T> class WeakPtr;
template <class T> class SigWeakPtr;
template <class T> class NoPtr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> class Ptr
{
public:
    typedef T element_type; 

    inline Ptr();
    inline Ptr(std::nullptr_t value);
    inline Ptr(const Ptr& value);
    inline Ptr(Ptr&& value);

    template <class Y> inline Ptr(const Ptr<Y>& ptr);
    template <class Y> inline explicit Ptr(const SigWeakPtr<Y>& ptr);
    template <class Y> inline Ptr(NoPtr<Y>& value);

    ~Ptr();
    
    void Reset();
    
    T* operator->() const;
    T& operator*() const;
    
    Ptr& operator=(const Ptr& ptr);
    Ptr& operator=(Ptr&& ptr);
    template <class Y> Ptr& operator=(const Ptr<Y>& ptr);
    template <class Y> inline Ptr<T>& operator=(const WeakPtr<Y>& ptr);
    template <class Y> inline Ptr<T>& operator=(const SigWeakPtr<Y>& ptr);

    template <class Y> bool operator==(const Ptr<Y>& ptr) const;
    template <class Y> bool operator!=(const Ptr<Y>& ptr) const;
    template <class Y> bool operator< (const Ptr<Y>& ptr) const;

    template <class Y> inline bool operator==(const WeakPtr<Y>& ptr) const;
    template <class Y> inline bool operator!=(const WeakPtr<Y>& ptr) const;
    template <class Y> inline bool operator<(const WeakPtr<Y>& ptr) const;

    template <class Y> inline bool operator==(const SigWeakPtr<Y>& ptr) const;
    template <class Y> inline bool operator!=(const SigWeakPtr<Y>& ptr) const;
    template <class Y> inline bool operator<(const SigWeakPtr<Y>& ptr) const;

    bool operator==(const T* obj) const;
    bool operator!=(const T* obj) const;
    bool operator< (const T* obj) const;

private:
    template <class Y> inline Ptr(const WeakPtr<Y>& ptr);

    template<class Y> friend class Ptr;
    template<class Y> friend class WeakPtr;
    template<class Y> friend class SigWeakPtr;
    friend class PtrTarget;
  
    template<class Y> friend Ptr<Y> ptr_new_cast(Y*);
    template<class Y> friend Ptr<Y> ptr_tmp_cast(Y*);

    template<class Y> friend Y* ptr_raw_pointer_cast (const Ptr<Y>&);
    template<class Y,class X> friend Y* ptr_raw_pointer_static_cast(const Ptr<X>& src);
    template<class Y,class X> friend Y* ptr_raw_pointer_dynamic_cast(const Ptr<X>& src);

    template<class Y,class X> friend Ptr<Y> ptr_static_cast(const Ptr<X>& src);
    template<class Y,class X> friend Ptr<Y> ptr_const_cast(const Ptr<X>& src);
    template<class Y,class X> friend Ptr<Y> ptr_dynamic_cast(const Ptr<X>& src);

    void Initialize(T* obj);

    void Set(T* obj);
    T*   Get() const;
    T*   m_Object;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class T> inline
Ptr<T> ptr_tmp_cast(T* obj)
{
    Ptr<T> ptr;
    if ( obj != nullptr )
    {
#ifndef NDEBUG
        obj->ValidateRefCount(0);
#endif
        ptr.Set(obj);
    }
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
/// Casting operator.
///
/// \par Description:
///      ptr_new_cast() is used to convert the initial raw-pointer returned
///      by the "new" operator to a smartpointer. It is crucial that all PtrTarget
///      derived objects are passed through ptr_new_cast() exactly once.
///      Note that you normally don't call ptr_new_cast() explicitly though.
///      A better solution is to allocate PtrTarget derived objects using
///      the ptr_new() function.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class T> inline
Ptr<T> ptr_new_cast(T* obj)
{
    Ptr<T> ptr;
    if (obj != nullptr)
    {
        ptr.Set(obj);
#ifndef NDEBUG
        obj->ValidateRefCount(PtrTarget::e_PreInitRefCount);
#endif
        obj->m_ReferenceCount -= PtrTarget::e_PreInitRefCount;
    }
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
/// Casting operator
///
/// \par Description:
///      Converts a smartpointer to a raw-pointer. Use with caution!
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class T> inline
T* ptr_raw_pointer_cast(const Ptr<T>& src)
{
    return src.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// Casting operator
///
/// \par Description:
///      Converts a smartpointer to a raw-pointer, then perform a static_cast
///      on the raw-pointer before returning it. Use with caution!
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class Y,class T> inline
Y* ptr_raw_pointer_static_cast(const Ptr<T>& src)
{
    return static_cast<Y*>(src.Get());
}

///////////////////////////////////////////////////////////////////////////////
/// Casting operator
///
/// \par Description:
///      Converts a smartpointer to a raw-pointer, then perform a dynamic_cast
///      on the raw-pointer before returning it. Use with caution!
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class Y,class T> inline
Y* ptr_raw_pointer_dynamic_cast(const Ptr<T>& src)
{
    return dynamic_cast<Y*>(src.Get());
}

///////////////////////////////////////////////////////////////////////////////
/// Casting operator
///
/// \par Description:
///      This is used to static_cast a smartpointer from one type to another.
///      The syntax and semantics are almost the same as static_cast. The
///      only difference is that it works with smartpointers, and that you
///      don't add a "*" after the target class name.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class Y,class T> inline
Ptr<Y> ptr_static_cast(const Ptr<T>& src)
{
    Ptr<Y> ptr;
    ptr.Set(static_cast<Y*>(src.Get()));
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
/// Casting operator
///
/// \par Description:
///      This is used to const_cast a smartpointer from one type to another.
///      The syntax and semantics are almost the same as static_cast. The
///      only difference is that it works with smartpointers, and that you
///      don't add a "*" after the target class name.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class Y,class T> inline
Ptr<Y> ptr_const_cast(const Ptr<T>& src)
{
    Ptr<Y> ptr;
    ptr.Set(const_cast<Y*>(src.Get()));
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
/// Casting operator
///
/// \par Description:
///      This is used to dynamic_cast a smartpointer from one type to another.
///      The syntax and semantics are almost the same as dynamic_cast. The
///      only difference is that it works with smartpointers, and that you
///      don't add a "*" after the target class name.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<class Y,class T> inline
Ptr<Y> ptr_dynamic_cast(const Ptr<T>& src)
{
    Ptr<Y> ptr;
    ptr.Set(dynamic_cast<Y*>(src.Get()));
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
/// Alternate new operator for smartpointer controlled objects.
///
/// \par Description:
///      This function allocates an instance of type T using the "new" operator
///      and passing all it's arguments to the class constructor. Then it
///      converts the raw-pointer returned by "new" to a smartpointer using
///      ptr_new_cast() and return that smartpointer.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T, typename ...ARGS>
Ptr<T> ptr_new(ARGS&&... args)
{
    return ptr_new_cast(new T( args... ));
}

#include "Ptr.inl.h"
