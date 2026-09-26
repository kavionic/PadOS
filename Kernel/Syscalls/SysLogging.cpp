// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 08.12.2025 22:30

#include <sys/pados_syscalls.h>
#include <System/ExceptionHandling.h>
#include <Kernel/KAddressValidation.h>
#include <Kernel/KLogging.h>
#include <Kernel/Logging/LogManager.h>

namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_register_category(uint32_t categoryHash, PLogChannel channel, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel)
{
    return ksystem_log_register_category(categoryHash, channel, categoryName, displayName, initialLogLevel);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_set_category_minimum_severity(uint32_t categoryHash, PLogSeverity logLevel)
{
    return ksystem_log_set_category_minimum_severity(categoryHash, logLevel);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_is_category_active(uint32_t categoryHash, PLogSeverity logLevel, bool* outIsActive)
{
    try
    {
        validate_user_write_pointer_trw(outIsActive);
        *outIsActive = ksystem_log_is_category_active(categoryHash, logLevel);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_get_category_channel(uint32_t categoryHash, PLogChannel* outChannel)
{
    try
    {
        validate_user_write_pointer_trw(outChannel);
        *outChannel = ksystem_log_get_category_channel(categoryHash);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode system_log_get_name(char* buffer, size_t bufferLen, const char* name, size_t nameLen)
{
    if (bufferLen == 0) {
        return PErrorCode::INVAL;
    }
    try
    {
        validate_user_write_pointer_trw(buffer, bufferLen);

        if (nameLen >= bufferLen)
        {
            memcpy(buffer, name, bufferLen - 1);
            buffer[bufferLen - 1] = 0;
            return PErrorCode::OVERFLOW;
        }
        memcpy(buffer, name, nameLen);
        buffer[nameLen] = 0;
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_get_severity_name(PLogSeverity logLevel, char* buffer, size_t bufferLen)
{
    try
    {
        validate_user_write_pointer_trw(buffer, bufferLen);
        const char* name = ksystem_log_get_severity_name(logLevel);
        const size_t nameLen = strlen(name);
        return system_log_get_name(buffer, bufferLen, name, nameLen);
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_get_category_name(uint32_t categoryHash, char* buffer, size_t bufferLen)
{
    try
    {
        validate_user_write_pointer_trw(buffer, bufferLen);
        const PString& name = ksystem_log_get_category_name(categoryHash);
        return system_log_get_name(buffer, bufferLen, name.c_str(), name.size());
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_get_category_display_name(uint32_t categoryHash, char* buffer, size_t bufferLen)
{
    try
    {
        validate_user_write_pointer_trw(buffer, bufferLen);
        const PString& name = ksystem_log_get_category_display_name(categoryHash);
        return system_log_get_name(buffer, bufferLen, name.c_str(), name.size());
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_system_log_add_message(uint32_t category, PLogSeverity severity, const char* message)
{
    try
    {
        validate_user_read_string_trw(message, 65536);
        ksystem_log_add_message(category, severity, message);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}


} // extern "C"

} // namespace kernel
