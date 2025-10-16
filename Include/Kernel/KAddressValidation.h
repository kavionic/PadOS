// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 11.10.2025 18:00

#pragma once

#include <System/ExceptionHandling.h>

template<typename T>
void validate_user_read_pointer_trw(const T* address)
{
    if (address == nullptr) {
        PERROR_THROW_CODE(PErrorCode::Fault);
    }
}

template<typename T>
void validate_user_write_pointer_trw(const T* address)
{
    if (address == nullptr) {
        PERROR_THROW_CODE(PErrorCode::Fault);
    }
}

inline void validate_user_read_pointer_trw(const void* address, size_t length)
{
    if (address == nullptr && length > 0) {
        PERROR_THROW_CODE(PErrorCode::Fault);
    }
}

inline void validate_user_write_pointer_trw(const void* address, size_t length)
{
    if (address == nullptr && length > 0) {
        PERROR_THROW_CODE(PErrorCode::Fault);
    }
}

inline void validate_user_read_string_trw(const char* address, size_t maxLength)
{
    if (address == nullptr && maxLength > 0) {
        PERROR_THROW_CODE(PErrorCode::Fault);
    }
}
