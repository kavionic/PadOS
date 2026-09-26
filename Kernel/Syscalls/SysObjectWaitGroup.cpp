// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 23.10.2025 19:00

#include <sys/pados_error_codes.h>
#include <Kernel/KObjectWaitGroup.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

extern "C"
{

PErrorCode sys_object_wait_group_create(handle_id* outHandle, const char* name)
{
    try
    {
        *outHandle = kobject_wait_group_create_trw(name);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_delete(handle_id handle)
{
    try
    {
        kobject_wait_group_delete_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_add_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    try
    {
        kobject_wait_group_add_object_trw(handle, objectHandle, waitMode);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_remove_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    try
    {
        kobject_wait_group_remove_object_trw(handle, objectHandle, waitMode);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_add_file(handle_id handle, int fileHandle, ObjectWaitMode waitMode)
{
    try
    {
        kobject_wait_group_add_file_trw(handle, fileHandle, waitMode);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_remove_file(handle_id handle, int fileHandle, ObjectWaitMode waitMode)
{
    try
    {
        kobject_wait_group_remove_file_trw(handle, fileHandle, waitMode);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_clear(handle_id handle)
{
    try
    {
        kobject_wait_group_clear_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_wait(handle_id handle, handle_id mutexHandle, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    try
    {
        kobject_wait_group_wait_trw(handle, mutexHandle, readyFlagsBuffer, readyFlagsSize);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_wait_timeout_ns(handle_id handle, handle_id mutexHandle, bigtime_t timeout, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    try
    {
        kobject_wait_group_wait_timeout_ns_trw(handle, mutexHandle, timeout, readyFlagsBuffer, readyFlagsSize);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_object_wait_group_wait_deadline_ns(handle_id handle, handle_id mutexHandle, bigtime_t deadline, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    try
    {
        kobject_wait_group_wait_deadline_ns_trw(handle, mutexHandle, deadline, readyFlagsBuffer, readyFlagsSize);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

} // extern "C"

} // namespace kernel
