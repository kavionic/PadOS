// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 23.10.2025 20:00

#include <sys/pados_syscalls.h>
#include <Kernel/KMessagePort.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

extern "C"
{

PErrorCode sys_message_port_create(port_id* outHandle, const char* name, int maxCount)
{
    try
    {
        *outHandle = kmessage_port_create_trw(name, maxCount);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_message_port_duplicate(port_id* outNewHandle, port_id handle)
{
    try
    {
        *outNewHandle = kmessage_port_duplicate_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_message_port_delete(port_id handle)
{
    try
    {
        kmessage_port_delete_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_message_port_send(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    try
    {
        kmessage_port_send_trw(handle, targetHandler, code, data, length);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode  sys_message_port_send_timeout_ns(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length, bigtime_t timeout)
{
    try
    {
        kmessage_port_send_timeout_ns_trw(handle, targetHandler, code, data, length, timeout);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode  sys_message_port_send_deadline_ns(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length, bigtime_t deadline)
{
    try
    {
        kmessage_port_send_deadline_ns_trw(handle, targetHandler, code, data, length, deadline);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PSysRetPair sys_message_port_receive(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize)
{
    try
    {
        return PMakeSysRetSuccess(kmessage_port_receive_trw(handle, targetHandler, code, buffer, bufferSize));
    }
    PERROR_CATCH_RET_SYSRET;
}

PSysRetPair sys_message_port_receive_timeout_ns(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t timeout)
{
    try
    {
        return PMakeSysRetSuccess(kmessage_port_receive_timeout_ns_trw(handle, targetHandler, code, buffer, bufferSize, timeout));
    }
    PERROR_CATCH_RET_SYSRET;
}

PSysRetPair sys_message_port_receive_deadline_ns(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t deadline)
{
    try
    {
        return PMakeSysRetSuccess(kmessage_port_receive_deadline_ns_trw(handle, targetHandler, code, buffer, bufferSize, deadline));
    }
    PERROR_CATCH_RET_SYSRET;
}

} // extern "C"

} // namespace kernel
