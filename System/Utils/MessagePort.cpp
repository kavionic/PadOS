// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 13.03.2018 21:00:23

#include "Utils/MessagePort.h"

bool PMessagePort::SendMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) const
{
    return ParseResult(message_port_send(m_Handle, targetHandler, code, data, length));
}

bool PMessagePort::SendMessageTimeout(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos timeout) const
{
    return ParseResult(message_port_send_timeout_ns(m_Handle, targetHandler, code, data, length, timeout.AsNanoseconds()));
}

bool PMessagePort::SendMessageDeadline(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos deadline) const
{
    return ParseResult(message_port_send_deadline_ns(m_Handle, targetHandler, code, data, length, deadline.AsNanoseconds()));
}
