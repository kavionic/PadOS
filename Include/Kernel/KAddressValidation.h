// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.10.2025 18:00

#pragma once

#include <System/ExceptionHandling.h>

template<typename T>
void validate_user_read_pointer_trw(const T* address)
{
    if (address == nullptr) {
        PERROR_THROW_CODE(PErrorCode::FAULT);
    }
}

template<typename T>
void validate_user_write_pointer_trw(const T* address)
{
    if (address == nullptr) {
        PERROR_THROW_CODE(PErrorCode::FAULT);
    }
}

inline void validate_user_read_pointer_trw(const void* address, size_t length)
{
    if (address == nullptr && length > 0) {
        PERROR_THROW_CODE(PErrorCode::FAULT);
    }
}

inline void validate_user_write_pointer_trw(const void* address, size_t length)
{
    if (address == nullptr && length > 0) {
        PERROR_THROW_CODE(PErrorCode::FAULT);
    }
}

inline void validate_user_read_string_trw(const char* address, size_t maxLength)
{
    if (address == nullptr && maxLength > 0) {
        PERROR_THROW_CODE(PErrorCode::FAULT);
    }
}
