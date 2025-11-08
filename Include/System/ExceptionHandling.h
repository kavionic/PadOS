// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
    catch (const std::system_error& error) { HANDLER(PErrorCode(error.code().value())); } \
    catch (const std::bad_alloc& error)    { HANDLER(PErrorCode::NoMemory); } \
    catch (const std::exception& error)    { HANDLER(PErrorCode::InvalidArg); } \
    ((void)0)

#define PERROR_CATCH_RET(HANDLER) \
    catch (const std::system_error& error) { return HANDLER(error, PErrorCode(error.code().value())); } \
    catch (const std::bad_alloc& error)    { return HANDLER(error, PErrorCode::NoMemory); } \
    catch (const std::exception& error)    { return HANDLER(error, PErrorCode::InvalidArg); } \
    ((void)0)

#define PERROR_CATCH_RET_CODE \
    catch (const std::system_error& error) { return PErrorCode(error.code().value()); } \
    catch (const std::bad_alloc& error)    { return PErrorCode::NoMemory; } \
    catch (const std::exception& error)    { return PErrorCode::InvalidArg; } \
    ((void)0)

#define PERROR_CATCH_RET_SYSRET \
    catch (const std::system_error& error) { return PMakeSysRetFail(PErrorCode(error.code().value())); } \
    catch (const std::bad_alloc& error)    { return PMakeSysRetFail(PErrorCode::NoMemory); } \
    catch (const std::exception& error)    { return PMakeSysRetFail(PErrorCode::InvalidArg); } \
    ((void)0)

#define PERROR_CATCH_SET_ERRNO(RET_VAL) \
    catch (const std::system_error& error) { set_last_error(error.code().value()); return RET_VAL; } \
    catch (const std::bad_alloc& error)    { set_last_error(ENOMEM); return RET_VAL; } \
    catch (const std::exception& error)    { set_last_error(EINVAL); return RET_VAL; } \
    ((void)0)

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
