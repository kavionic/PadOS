// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.09.2026 00:00

#pragma once

#include <System/UserspaceService.h>

#ifdef PADOS_MODULE_USER_SPACE
namespace kernel
{

// Synchronous, thread-context-only request. Callers must not hold locks needed
// by the user-space allocator or thread cleanup while waiting for completion.
PErrorCode kuserspace_service_request(PUserspaceServiceRequest& request);

// Transfers ownership on success and returns without waiting for cleanup.
PErrorCode kuserspace_service_schedule_thread_cleanup(PThreadUserData* threadData);

} // namespace kernel
#endif // PADOS_MODULE_USER_SPACE
