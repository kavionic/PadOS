// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 09.03.2018 16:02:32

#include "System/Platform.h"

#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include <new>

#include "Kernel/KMessagePort.h"
#include "Kernel/Kernel.h"
#include "Threads/Threads.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

namespace kernel
{
    struct KMessagePortMessage
    {
        handler_id           m_TargetHandler;
        int32_t              m_Code;
        size_t               m_Length;
        KMessagePortMessage* m_Next;
    };
}

static const size_t MAX_CACHED_MESSAGE_SIZE  = 64;
static const int    MAX_CACHED_MESSAGE_COUNT = 100;

static KMessagePortMessage* gk_FirstCachedMessage = nullptr;
static int                  gk_CachedMessageCount = 0;

static KMessagePortMessage* alloc_message(size_t size)
{
    if (size <= MAX_CACHED_MESSAGE_SIZE)
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        if (gk_CachedMessageCount > 0)
        {
            KMessagePortMessage* message = gk_FirstCachedMessage;
            gk_FirstCachedMessage = message->m_Next;
            gk_CachedMessageCount--;
            message->m_Length = size;
            return message;
        }
    }    
    KMessagePortMessage* message = static_cast<KMessagePortMessage*>(malloc(sizeof(KMessagePortMessage) + std::max(MAX_CACHED_MESSAGE_SIZE, size)));
    if (message != nullptr)
    {
        message->m_Length = size;
    }
    return message;
}

static void free_message(KMessagePortMessage* message)
{
    if (message->m_Length <= MAX_CACHED_MESSAGE_SIZE)
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        if (gk_CachedMessageCount < MAX_CACHED_MESSAGE_COUNT)
        {
            message->m_Next = gk_FirstCachedMessage;
            gk_FirstCachedMessage = message;
            gk_CachedMessageCount++;
            return;
        }
    }
    free(message);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KMessagePort::KMessagePort(const char* name, size_t maxCount) : KNamedObject(name, ObjectType)
                                                              , m_Mutex("message_port_mutex", EMutexRecursionMode::RaiseError)
                                                              , m_SendCondition("message_port_send")
                                                              , m_ReceiveCondition("message_port_receive")
                                                              , m_MaxCount(maxCount)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KMessagePort::~KMessagePort()
{
    while(m_FirstMsg != nullptr)
    {
        KMessagePortMessage* message = m_FirstMsg;
        m_FirstMsg = message->m_Next;
        free_message(message);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMessagePort::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    CRITICAL_SCOPE(m_Mutex);

    switch (mode)
    {
        case ObjectWaitMode::Read:
            if (m_MessageCount == 0) {
                return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read);
            } else {
                return false; // Will not block.
            }
        case ObjectWaitMode::Write:
            if (m_MessageCount >= m_MaxCount) {
                return m_SendCondition.AddListener(waitNode, ObjectWaitMode::Read);
            } else {
                return false; // Will not block.
            }
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMessagePort::SendMessage(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValMicros timeout)
{
    TimeValMicros deadline = (!timeout.IsInfinit()) ? (get_system_time() + timeout) : TimeValMicros::infinit;
    CRITICAL_SCOPE(m_Mutex);

    while (m_MessageCount >= m_MaxCount)
    {
        if (!m_SendCondition.WaitDeadline(m_Mutex, deadline)) {
            if (get_last_error() != EINTR)
            {
                return false;
            }
        }
    }

    KMessagePortMessage* message = alloc_message(length);
    if (message == nullptr) {
        return false;
    }
    message->m_TargetHandler = targetHandler;
    message->m_Code = code;
    message->m_Length = length;

    memcpy(message + 1, data, length);

    message->m_Next = nullptr;
    if (m_LastMsg != nullptr) {
        m_LastMsg->m_Next = message;
    } else {
        m_FirstMsg = message;
    }
    m_LastMsg = message;
    m_MessageCount++;
    m_ReceiveCondition.WakeupAll();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t KMessagePort::ReceiveMessage(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize)
{
	CRITICAL_SCOPE(m_Mutex);

    while (m_MessageCount == 0)
    {
        if (!m_ReceiveCondition.Wait(m_Mutex))
        {
            if (get_last_error() != EINTR) {
                return -1;
            }
        }
    }
    return DetachMessage(targetHandler, code, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t KMessagePort::ReceiveMessageTimeout(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValMicros timeout)
{
    return ReceiveMessageDeadline(targetHandler, code, buffer, bufferSize, (!timeout.IsInfinit()) ? (get_system_time() + timeout) : TimeValMicros::infinit);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t KMessagePort::ReceiveMessageDeadline(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValMicros deadline)
{
	CRITICAL_SCOPE(m_Mutex);

	while (m_MessageCount == 0)
	{
		if (!m_ReceiveCondition.WaitDeadline(m_Mutex, deadline))
		{
			if (get_last_error() != EINTR)
			{
				return -1;
			}
		}
	}
    return DetachMessage(targetHandler, code, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t KMessagePort::DetachMessage(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize)
{
    KMessagePortMessage* message = m_FirstMsg;
    kassure(m_MessageCount > 0 && message != nullptr, "ERROR: KMessagePort::ReceiveMessage() acquired receive semaphore with no message available.: %s\n", GetName());
    if (message == nullptr) {
        set_last_error(EINVAL); // DetachMessage() should never be called unless there is a message available.
        return -1;
    }
    m_FirstMsg = message->m_Next;
    if (m_FirstMsg == nullptr) {
        m_LastMsg = nullptr;
    }
    m_MessageCount--;
    m_SendCondition.WakeupAll();

    ssize_t bytesReceived = 0;
    
    if (targetHandler != nullptr) *targetHandler = message->m_TargetHandler;
    if (code != nullptr)          *code          = message->m_Code;
    
    if (buffer != nullptr) {
        bytesReceived = std::min(bufferSize, message->m_Length);
        memcpy(buffer, message + 1, bytesReceived);
    }
    free_message(message);
    return bytesReceived;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

port_id create_message_port(const char* name, int maxCount)
{
    try {
        return KNamedObject::RegisterObject(ptr_new<KMessagePort>(name, maxCount));
    } catch(const std::bad_alloc& error) {
        set_last_error(ENOMEM);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

port_id  duplicate_message_port(port_id handle)
{
    Ptr<KMessagePort> port = KNamedObject::GetObject<KMessagePort>(handle);
    if (port != nullptr) {
        return KNamedObject::RegisterObject(port);
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t delete_message_port(port_id handle)
{
    if (KNamedObject::FreeHandle(handle, KNamedObjectType::MessagePort)) {
        return 0;
    }
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t send_message(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length, bigtime_t timeout)
{
    Ptr<KMessagePort> port = KNamedObject::GetObject<KMessagePort>(handle);
    if (port != nullptr) {
        return port->SendMessage(targetHandler, code, data, length, TimeValMicros::FromMicroseconds(timeout)) ? 0 : -1;
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t receive_message(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize)
{
    Ptr<KMessagePort> port = KNamedObject::GetObject<KMessagePort>(handle);
    if (port != nullptr) {
        return port->ReceiveMessage(targetHandler, code, buffer, bufferSize);
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t receive_message_timeout(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t timeout)
{
    Ptr<KMessagePort> port = KNamedObject::GetObject<KMessagePort>(handle);
    if (port != nullptr) {
        return port->ReceiveMessageTimeout(targetHandler, code, buffer, bufferSize, TimeValMicros::FromMicroseconds(timeout));
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t receive_message_deadline(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t deadline)
{
    Ptr<KMessagePort> port = KNamedObject::GetObject<KMessagePort>(handle);
    if (port != nullptr) {
        return port->ReceiveMessageDeadline(targetHandler, code, buffer, bufferSize, TimeValMicros::FromMicroseconds(deadline));
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}
