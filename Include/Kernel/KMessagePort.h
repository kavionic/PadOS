// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

    PErrorCode  SendMessage(handler_id targetHandler, int32_t code, const void* data, size_t length);
    PErrorCode  SendMessageTimeout(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos timeout);
    PErrorCode  SendMessageDeadline(handler_id targetHandler, int32_t code, const void* data, size_t length, TimeValNanos deadline);

    ssize_t ReceiveMessage_trw(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize);
    ssize_t ReceiveMessageTimeout_trw(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValNanos timeout);
    ssize_t ReceiveMessageDeadline_trw(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, TimeValNanos deadline);
    
private:
    ssize_t DetachMessage_trw(handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize);
    
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

port_id kmessage_port_create_trw(const char* name, int maxCount);
port_id kmessage_port_duplicate_trw(port_id handle);
void    kmessage_port_delete_trw(port_id handle);
void    kmessage_port_send_trw(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length);
void    kmessage_port_send_timeout_ns_trw(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length, bigtime_t timeout);
void    kmessage_port_send_deadline_ns_trw(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length, bigtime_t deadline);
ssize_t kmessage_port_receive_trw(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize);
ssize_t kmessage_port_receive_timeout_ns_trw(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t timeout);
ssize_t kmessage_port_receive_deadline_ns_trw(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t deadline);


} // namespace
