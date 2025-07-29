// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
