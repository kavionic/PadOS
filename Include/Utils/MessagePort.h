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
// Created: 13.03.2018 21:00:24

#pragma once


#include "System/Types.h"
#include "System/System.h"
#include "System/HandleObject.h"

namespace os
{

class MessagePort : public HandleObject
{
public:
    MessagePort(const char* name, int maxCount) : HandleObject(create_message_port(name, maxCount)) {}
    MessagePort(port_id port, bool doClone = false) : HandleObject((doClone) ? duplicate_message_port(port) : port) {
        m_DontDeletePort = !doClone;
    }
    ~MessagePort() {
        if (m_DontDeletePort) m_Handle = INVALID_HANDLE;
    }
    
    bool    SendMessage(handler_id targetHandler, int32_t code, const void* data, size_t length, bigtime_t timeout = INFINIT_TIMEOUT) const {
        return send_message(m_Handle, targetHandler, code, data, length, timeout) >= 0;
    }
    ssize_t ReceiveMessage(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize) const {
        return receive_message(m_Handle, targetHandler, code, buffer, bufferSize);
    }
    ssize_t ReceiveMessageTimeout(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t timeout) const {
        return receive_message_timeout(m_Handle, targetHandler, code, buffer, bufferSize, timeout);
    }
    ssize_t ReceiveMessageDeadline(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t deadline) const {
        return receive_message_deadline(m_Handle, targetHandler, code, buffer, bufferSize, deadline);
    }

    MessagePort(MessagePort&& other) = default;
    MessagePort(const MessagePort& other) = default;
    MessagePort& operator=(const MessagePort&) = default;

    
private:
    bool    m_DontDeletePort = false;
};

} // namespace
