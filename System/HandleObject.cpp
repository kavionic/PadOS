// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.07.2020 13:00

#include <PadOS/HandleObject.h>
#include <System/HandleObject.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PHandleObject::PHandleObject(const PHandleObject& other)
{
    if (duplicate_handle(other.m_Handle, &m_Handle) != PErrorCode::Success) {
        m_Handle = INVALID_HANDLE;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PHandleObject::~PHandleObject()
{
    if (m_Handle != INVALID_HANDLE) {
        delete_handle(m_Handle);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PHandleObject& PHandleObject::operator=(const PHandleObject& other)
{
    if (m_Handle != INVALID_HANDLE) {
        delete_handle(m_Handle);
    }
    if (duplicate_handle(other.m_Handle, &m_Handle) != PErrorCode::Success) {
        m_Handle = INVALID_HANDLE;
    }
    return *this;
}

