// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 13.03.2018 21:00:24

#pragma once


#include <PadOS/MessagePort.h>
#include "System/Types.h"
#include "System/System.h"
#include "System/HandleObject.h"
#include "System/TimeValue.h"

class PMessagePort : public PHandleObject
{
public:
    PMessagePort(const char* name, int maxCount)
    {
        port_id handle;
        if (message_port_create(&handle, name, maxCount) == PErrorCode::Success) {
            SetHandle(handle);
        }
    }
    PMessagePort(port_id port, bool doClone = false) : PHandleObject()
    {
        if (doClone)
        {
            port_id newHandle;

            if (message_port_duplicate(&newHandle, port) == PErrorCode::Success) {
                SetHandle(newHandle);
            }
        }
        else
        {
            SetHandle(port);
        }
        m_DontDeletePort = !doClone;
    }
    ~PMessagePort() {
        if (m_DontDeletePort) m_Handle = INVALID_HANDLE;
    }
    
    bool    SendMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) const;
    bool    SendMessageTimeout(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos timeout) const;
    bool    SendMessageDeadline(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos deadline) const;
    ssize_t ReceiveMessage(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize) const {
        return message_port_receive(m_Handle, targetHandler, code, buffer, bufferSize);
    }
    ssize_t ReceiveMessageTimeout(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValNanos timeout) const {
        return message_port_receive_timeout_ns(m_Handle, targetHandler, code, buffer, bufferSize, timeout.AsNanoseconds());
    }
    ssize_t ReceiveMessageDeadline(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValNanos deadline) const {
        return message_port_receive_deadline_ns(m_Handle, targetHandler, code, buffer, bufferSize, deadline.AsNanoseconds());
    }

    PMessagePort(PMessagePort&& other) = default;
    PMessagePort(const PMessagePort& other) = default;
    PMessagePort& operator=(const PMessagePort&) = default;

    
private:
    bool    m_DontDeletePort = false;
};
