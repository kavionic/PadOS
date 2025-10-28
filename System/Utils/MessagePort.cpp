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
// Created: 13.03.2018 21:00:23

#include "Utils/MessagePort.h"

namespace os
{

bool MessagePort::SendMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) const
{
    return ParseResult(message_port_send(m_Handle, targetHandler, code, data, length));
}

bool MessagePort::SendMessageTimeout(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos timeout) const
{
    return ParseResult(message_port_send_timeout_ns(m_Handle, targetHandler, code, data, length, timeout.AsNanoseconds()));
}

bool MessagePort::SendMessageDeadline(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos deadline) const
{
    return ParseResult(message_port_send_deadline_ns(m_Handle, targetHandler, code, data, length, deadline.AsNanoseconds()));
}

} // namespace os
