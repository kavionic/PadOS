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


class MessagePort
{
public:
    MessagePort(const char* name, int maxCount) : m_Port(create_message_port(name, maxCount)) {}
    ~MessagePort() {
        if (m_Port != -1) delete_message_port(m_Port);
    }
    port_id GetPortID() const { return m_Port; }
    
    bool    SendMessage(int32_t code, const void* data, size_t length, bigtime_t timeout = INFINIT_TIMEOUT) {
        return send_message(m_Port, code, data, length, timeout) >= 0;
    }
    ssize_t ReceiveMessage(int32_t* code, void* buffer, size_t bufferSize) {
        return receive_message(m_Port, code, buffer, bufferSize);
    }
    ssize_t ReceiveMessageTimeout(int32_t* code, void* buffer, size_t bufferSize, bigtime_t timeout) {
        return receive_message_timeout(m_Port, code, buffer, bufferSize, timeout);
    }
    ssize_t ReceiveMessageDeadline(int32_t* code, void* buffer, size_t bufferSize, bigtime_t deadline) {
        return receive_message_deadline(m_Port, code, buffer, bufferSize, deadline);
    }

    MessagePort(const MessagePort& other) { m_Port = duplicate_message_port(other.m_Port); }
    MessagePort& operator=(const MessagePort& other)
    {
        if (m_Port != -1) delete_message_port(m_Port);        
        m_Port = duplicate_message_port(other.m_Port);
        return *this;
    }
    
private:
    port_id m_Port;
};
