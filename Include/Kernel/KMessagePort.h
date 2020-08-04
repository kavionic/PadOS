// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include "KNamedObject.h"
#include "KConditionVariable.h"
#include "KMutex.h"
#include "System/Types.h"
#include "System/System.h"

namespace kernel
{

struct KMessagePortMessage;

class KMessagePort : public KNamedObject
{
public:
    static const KNamedObjectType ObjectType = KNamedObjectType::MessagePort;

    KMessagePort(const char* name, size_t maxCount);
    ~KMessagePort();

    // From KNamedObject:
    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

    bool    SetReplyPort(port_id port);

    bool    SendMessage(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValMicros timeout = TimeValMicros::infinit);
    
    ssize_t ReceiveMessage(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize);
    ssize_t ReceiveMessageTimeout(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValMicros timeout);
    ssize_t ReceiveMessageDeadline(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValMicros deadline);
    
private:
    ssize_t DetachMessage(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize);
    
    KMutex     m_Mutex;
    KConditionVariable m_SendCondition;
    KConditionVariable m_ReceiveCondition;

    size_t  m_MaxCount;
    size_t  m_MessageCount = 0;

    KMessagePortMessage* m_FirstMsg = nullptr;
    KMessagePortMessage* m_LastMsg = nullptr;

    KMessagePort(const KMessagePort &) = delete;
    KMessagePort& operator=(const KMessagePort &) = delete;
};

} // namespace
