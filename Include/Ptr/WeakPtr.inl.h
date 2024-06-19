// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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

#include <algorithm>
#include "Ptr.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
WeakPtr<T>::WeakPtr()
{
    m_Notifier = nullptr;
    m_Object   = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
WeakPtr<T>::WeakPtr(std::nullptr_t)
{
    m_Notifier = nullptr;
    m_Object   = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
WeakPtr<T>::WeakPtr(const WeakPtr& ptr)
{
    if ( ptr.m_Notifier != nullptr && ptr.m_Notifier->IsValid() ) {
        m_Notifier = ptr.m_Notifier;
        m_Notifier->AddRef();
    } else {
        m_Notifier = nullptr;
    }
    m_Object = ptr.m_Object;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> inline
WeakPtr<T>::WeakPtr(WeakPtr&& ptr) : m_Notifier(ptr.m_Notifier), m_Object(ptr.m_Object)
{
    ptr.m_Object   = nullptr;
    ptr.m_Notifier = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
WeakPtr<T>::WeakPtr(const WeakPtr<Y>& ptr)
{
    if (ptr.m_Notifier != nullptr && ptr.m_Notifier->IsValid()) {
        m_Notifier = ptr.m_Notifier;
        m_Notifier->AddRef();
    } else {
        m_Notifier = nullptr;
    }
    m_Object = ptr.m_Object;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
WeakPtr<T>::WeakPtr(const Ptr<Y>& ptr)
{
    if (ptr.m_Object != nullptr) {
        m_Notifier = ptr.m_Object->GetNotifier();
    } else {
        m_Notifier = nullptr;
    }
    m_Object = ptr.m_Object;  
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> inline
WeakPtr<T>::~WeakPtr()
{
    if (m_Notifier != nullptr) {
        m_Notifier->Release();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
void WeakPtr<T>::Reset()
{
    if (m_Notifier != nullptr)
    {
        m_Notifier->Release();
        m_Notifier = nullptr;
        m_Object   = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr& ptr)
{
    if (this != &ptr)
    {
        if ( m_Notifier != nullptr ) {
            m_Notifier->Release();
        }
        if (ptr.m_Notifier != nullptr && ptr.m_Notifier->IsValid())
        {
            m_Notifier = ptr.m_Notifier;
            m_Notifier->AddRef();
            m_Object = ptr.m_Object;
        }
        else
        {
            m_Notifier = nullptr;
            m_Object   = nullptr;
        }
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<Y>& ptr)
{
    if ( m_Notifier != nullptr ) {
        m_Notifier->Release();
    }
    if (ptr.m_Notifier != nullptr && ptr.m_Notifier->IsValid())
    {
        m_Notifier = ptr.m_Notifier;
        m_Notifier->AddRef();
        m_Object = ptr.m_Object;
    }
    else
    {
        m_Notifier = nullptr;
        m_Object   = nullptr;
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
WeakPtr<T>& WeakPtr<T>::operator=(const Ptr<Y>& ptr)
{
    Set(ptr.m_Object);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool WeakPtr<T>::operator==( const WeakPtr<Y>& p ) const
{
    return m_Notifier == p.m_Notifier;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool WeakPtr<T>::operator!=( const WeakPtr<Y>& p ) const
{
    return m_Notifier != p.m_Notifier;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool WeakPtr<T>::operator<( const WeakPtr<Y>& p ) const
{
    return m_Notifier < p.m_Notifier;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool WeakPtr<T>::operator==( const Ptr<Y>& p ) const
{
    return Get() == p.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool WeakPtr<T>::operator!=( const Ptr<Y>& p ) const
{
    return Get() != p.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Y> inline
bool WeakPtr<T>::operator<( const Ptr<Y>& p ) const
{
    return Get() < p.Get();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T>
template <class Y> inline
void WeakPtr<T>::Set(Y* obj)
{
    if (obj != m_Object)
    {
        if (m_Notifier != nullptr) {
            m_Notifier->Release();
        }
        if (obj != nullptr) {
            m_Notifier = obj->GetNotifier();
        } else {
            m_Notifier = nullptr;
        }
        m_Object = obj;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
  
template <class T> inline
T* WeakPtr<T>::Get() const
{
    if ( m_Notifier != nullptr && m_Notifier->IsValid() ) {
        return m_Object;
    } else {
        return nullptr;
    }
}
