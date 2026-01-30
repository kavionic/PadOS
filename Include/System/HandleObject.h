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

#pragma once

#include <sys/pados_error_codes.h>
#include <System/Types.h>
#include <System/System.h>


class PHandleObject
{
public:
    PHandleObject() : m_Handle(INVALID_HANDLE) {}
    PHandleObject(handler_id handle) : m_Handle(handle) {}
    virtual ~PHandleObject();

    void SetHandle(handle_id handle) { m_Handle = handle; }
    handle_id GetHandle() const { return m_Handle; }
    
    PHandleObject(PHandleObject&& other) : m_Handle(other.m_Handle) { other.m_Handle = INVALID_HANDLE; }

    PHandleObject(const PHandleObject& other);
    PHandleObject& operator=(const PHandleObject& other);

protected:
    bool ParseResult(PErrorCode result) const
    {
        if (result == PErrorCode::Success)
        {
            return true;
        }
        else
        {
            set_last_error(result);
            return false;
        }
    }

    handle_id m_Handle;
};
