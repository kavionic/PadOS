// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 11.07.2020 13:00

#include "System/HandleObject.h"
#include "Threads/Threads.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HandleObject::HandleObject(const HandleObject& other)
{
    if (duplicate_handle(m_Handle, other.m_Handle) != PErrorCode::Success) {
        m_Handle = INVALID_HANDLE;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HandleObject::~HandleObject()
{
    if (m_Handle != INVALID_HANDLE) {
        delete_handle(m_Handle);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HandleObject& HandleObject::operator=(const HandleObject& other)
{
    if (m_Handle != INVALID_HANDLE) {
        delete_handle(m_Handle);
    }
    if (duplicate_handle(m_Handle, other.m_Handle) != PErrorCode::Success) {
        m_Handle = INVALID_HANDLE;
    }
    return *this;
}

