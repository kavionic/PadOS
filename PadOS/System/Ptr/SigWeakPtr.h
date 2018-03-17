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

#include "SignalSystem.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> class WeakPtrSignal_c : public Signal<void,Ptr<T> >
{
public:
    WeakPtrSignal_c() {}
    void AddRef() { m_nRefCount++; }
    void Release() {
        if ( --m_nRefCount == 0 ) {
            delete this;
        }
    }
private:
    virtual ~WeakPtrSignal_c() {
        assert(( m_nRefCount == 0 ));
    }
    int m_nRefCount = 1;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> class WeakPtrSignalWrapper_c
{
public:
    typedef void R;
    typedef Ptr<T> A0;

    WeakPtrSignalWrapper_c() {
        m_Signal = new WeakPtrSignal_c<T>();
    }
  
    WeakPtrSignalWrapper_c(const WeakPtrSignalWrapper_c& other) {
        m_Signal = other.m_Signal;
        m_Signal->AddRef();
    }

    ~WeakPtrSignalWrapper_c() {
        m_Signal->Release();
    }
  
    WeakPtrSignalWrapper_c& operator=(const WeakPtrSignalWrapper_c& other)
    {
        if (this == &other) {
            return( *this );
        }
        m_Signal->Release();
    
        m_Signal = other.m_Signal;
        m_Signal->AddRef();
        return *this;
    }

    template <class fC> void Connect( const SignalTarget* pcObj, R (fC::*pfCallback)(  ) ) const {
        m_Signal->Connect( pcObj, pfCallback );
    }
    template <class fC> void Connect( const SignalTarget* pcObj, R (fC::*pfCallback)(  ) const ) const {
        m_Signal->Connect( pcObj, pfCallback );
    }
    void Connect( R (*pfCallback)(  ) ) {
        m_Signal->Connect( pfCallback );
    }
    template <class fC> void Disconnect( const SignalTarget* pcObj, R (fC::*pfCallback)(  ) ) const {
        m_Signal->Disconnect( pcObj, pfCallback );
    }
    template <class fC> void Disconnect( const SignalTarget* pcObj, R (fC::*pfCallback)(  ) const ) const {
        m_Signal->Disconnect( pcObj, pfCallback );
    }
    void Disconnect( R (*pfCallback)(  ) ) const {
        m_Signal->Disconnect( pfCallback );
    }
    template <class fC> bool IsSlotConnected( const SignalTarget* pcObj, R (fC::*pfCallback)(  ) ) const {
        return m_Signal->IsSlotConnected( pcObj, pfCallback );
    }
    template <class fC> bool IsSlotConnected( const SignalTarget* pcObj, R (fC::*pfCallback)(  ) const ) const {
        return m_Signal->IsSlotConnected( pcObj, pfCallback );
    }
    bool IsSlotConnected( R (*pfCallback)(  ) ) const {
        return m_Signal->IsSlotConnected( pfCallback );
    }
    template <class fC> void Connect( const SignalTarget* pcObj, R (fC::*pfCallback)( A0 ) ) const {
        m_Signal->Connect( pcObj, pfCallback );
    }
    template <class fC> void Connect( const SignalTarget* pcObj, R (fC::*pfCallback)( A0 ) const ) const {
        m_Signal->Connect( pcObj, pfCallback );
    }
    void Connect( R (*pfCallback)( A0 ) ) {
        m_Signal->Connect( pfCallback );
    }
    template <class fC> void Disconnect( const SignalTarget* pcObj, R (fC::*pfCallback)( A0 ) ) const {
        m_Signal->Disconnect( pcObj, pfCallback );
    }
    template <class fC> void Disconnect( const SignalTarget* pcObj, R (fC::*pfCallback)( A0 ) const ) const {
        m_Signal->Disconnect( pcObj, pfCallback );
    }
    void Disconnect( R (*pfCallback)( A0 ) ) const {
        m_Signal->Disconnect( pfCallback );
    }
    template <class fC> bool IsSlotConnected( const SignalTarget* pcObj, R (fC::*pfCallback)( A0 ) ) const {
        return m_Signal->IsSlotConnected( pcObj, pfCallback );
    }
    template <class fC> bool IsSlotConnected( const SignalTarget* pcObj, R (fC::*pfCallback)( A0 ) const ) const {
        return m_Signal->IsSlotConnected( pcObj, pfCallback );
    }
    bool IsSlotConnected( R (*pfCallback)( A0 ) ) const {
        return m_Signal->IsSlotConnected( pfCallback );
    }

    bool operator() ( A0 a0 ) const {
        return (*m_Signal)( a0 );
    }

private:
    WeakPtrSignal_c<T>* m_Signal;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template <class T> class SigWeakPtr : public PtrTargetMonitor
{
public:
    typedef T element_type; 

    explicit SigWeakPtr( T* obj = nullptr );
    SigWeakPtr(const SigWeakPtr& ptr);

    template <class Y> SigWeakPtr(const WeakPtr<Y>& ptr);
    template <class Y> SigWeakPtr(const Ptr<Y>& ptr);

    ~SigWeakPtr();

    void Reset();
  
    SigWeakPtr& operator=(const SigWeakPtr& ptr);
    template <class Y> SigWeakPtr& operator=(const Ptr<Y>& ptr);
  
    T* operator->() const;

    template <class Y> bool operator==(const SigWeakPtr<Y>& ptr) const;
    template <class Y> bool operator!=(const SigWeakPtr<Y>& ptr) const;
    template <class Y> bool operator<(const SigWeakPtr<Y>& ptr) const;

    template <class Y> bool operator==(const WeakPtr<Y>& ptr) const;
    template <class Y> bool operator!=(const WeakPtr<Y>& ptr) const;
    template <class Y> bool operator<(const WeakPtr<Y>& ptr) const;
  
    template <class Y> bool operator==(const Ptr<Y>& ptr) const;
    template <class Y> bool operator!=(const Ptr<Y>& ptr) const;
    template <class Y> bool operator<(const Ptr<Y>& ptr) const;
  
    bool operator==(const T* ptr) const;
    bool operator!=(const T* ptr) const;
    bool operator<(const T* ptr) const;

    // From PointerTargetMonitor:
    virtual void ObjectDied();
  
public:
    WeakPtrSignalWrapper_c<T> SignalTargetDying;

private:
    template<class Y> friend class Ptr;
    template<class Y> friend class WeakPtr;
    template<class Y> friend class SigWeakPtr;

    template<class Y> friend Y* ptr_raw_pointer_cast (const SigWeakPtr<Y>&);

    template <class Y> void Set(Y* obj);
    T* Get() const;

    T*    m_Object;
    bool* m_DeleteGuard;
};

#include "SigWeakPtr.inl.h"
