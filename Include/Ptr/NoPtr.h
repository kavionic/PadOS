// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
/// Wrapper used for explicitly disabling reference counting on PtrTarget
/// objects. This should be used very carefully, but can in rare cases be
/// useful to enable stack allocation and by-value members of classes inheriting
/// from PtrTarget. You can safely point WeakPtr and SigWeakPtr pointers
/// to a NoPtr object, but when assigning it to a Ptr great care must be
/// taken to assure that the lifetime of the Ptr object is shorter than that of
/// the object it points to.
///
/// The usage of this class is very similar to that of Ptr and the other pointer
/// classes. You declare an instance with
/// "NoPtr<Class> instance( constructor args );", and then access
/// members on 'instance' as if it was an instance of Class. All arguments
/// passed to the NoPtr<Class> constructor will be forwarded to the Class
/// constructor.
///
/// \author Kurt Skauen
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class NoPtr : public T
{
public:
    template<typename ...ARGS>
    NoPtr(ARGS&&... args) : T(args...) { static_cast<T*>(this)->DisableReferenceCounting(); }

    T* operator->() const noexcept { return const_cast<NoPtr<T>*>(this); }
    T& operator*() const noexcept { return *const_cast<NoPtr<T>*>(this); }

};
