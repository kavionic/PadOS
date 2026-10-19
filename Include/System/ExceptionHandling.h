// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 04.10.2025 17:00

#pragma once

#include <utility>
#include <functional>
#include <experimental/scope>
#include <system_error>
#include <sys/pados_error_codes.h>
#include <PadOS/SyscallReturns.h>

#include <System/System.h>

#define PERROR_THROW_CODE(error) throw std::system_error(std::to_underlying(error), std::generic_category())
#define PERROR_ERRORCODE_THROW_ON_FAIL(result)  p_error_code_throw_on_fail(result)
#define PERROR_SYSRET_THROW_ON_FAIL(result)     p_sysret_throw_on_fail(result)

#define PERROR_CATCH(HANDLER) \
    catch (const std::system_error& PERROR_CATCH_error) { HANDLER(PErrorCode(PERROR_CATCH_error.code().value())); } \
    catch (const std::bad_alloc& PERROR_CATCH_error)    { HANDLER(PErrorCode::NOMEM); } \
    catch (const std::exception& PERROR_CATCH_error)    { HANDLER(PErrorCode::INVAL); } \
    ((void)0)

#define PERROR_CATCH_RET(HANDLER) \
    catch (const std::system_error& PERROR_CATCH_error) \
    { \
        return HANDLER(PERROR_CATCH_error, PErrorCode(PERROR_CATCH_error.code().value())); \
    } \
    catch (const std::bad_alloc& PERROR_CATCH_error)    { return HANDLER(PERROR_CATCH_error, PErrorCode::NOMEM); } \
    catch (const std::exception& PERROR_CATCH_error)    { return HANDLER(PERROR_CATCH_error, PErrorCode::INVAL); } \
    ((void)0)

#define PERROR_CATCH_RET_CODE \
    catch (const std::system_error& PERROR_CATCH_error) { return PErrorCode(PERROR_CATCH_error.code().value()); } \
    catch (const std::bad_alloc& PERROR_CATCH_error)    { return PErrorCode::NOMEM; } \
    catch (const std::exception& PERROR_CATCH_error)    { return PErrorCode::INVAL; } \
    ((void)0)

#define PERROR_CATCH_RET_SYSRET \
    catch (const std::system_error& PERROR_CATCH_error) \
    { \
        return PMakeSysRetFail(PErrorCode(PERROR_CATCH_error.code().value())); \
    } \
    catch (const std::bad_alloc& PERROR_CATCH_error)    { return PMakeSysRetFail(PErrorCode::NOMEM); } \
    catch (const std::exception& PERROR_CATCH_error)    { return PMakeSysRetFail(PErrorCode::INVAL); } \
    ((void)0)

#define PERROR_CATCH_SET_ERRNO(RET_VAL) \
    catch (const std::system_error& PERROR_CATCH_error) { set_last_error(PERROR_CATCH_error.code().value()); return RET_VAL; } \
    catch (const std::bad_alloc& PERROR_CATCH_error)    { set_last_error(ENOMEM); return RET_VAL; } \
    catch (const std::exception& PERROR_CATCH_error)    { set_last_error(EINVAL); return RET_VAL; } \
    ((void)0)

#define PRETHROW_CANCELLATION catch (const __cxxabiv1::__forced_unwind&) { throw; }

template<typename EF> using PScopeSuccess = std::experimental::scope_success<EF>;
template<typename EF> using PScopeFail    = std::experimental::scope_fail<EF>;
template<typename EF> using PScopeExit    = std::experimental::scope_exit<EF>;

inline PErrorCode p_error_code_throw_on_fail(PErrorCode result)
{
    if (result != PErrorCode::Success) {
        PERROR_THROW_CODE(result);
    }
    return result;
}

inline int32_t p_sysret_throw_on_fail(PSysRetPair result)
{
    PERROR_ERRORCODE_THROW_ON_FAIL(PSysRetResult(result));
    return PSysRetValue(result);
}
