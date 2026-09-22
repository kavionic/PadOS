// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
