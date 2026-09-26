// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
