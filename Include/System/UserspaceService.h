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

#include <atomic>
#include <stddef.h>
#include <sys/types.h>

#include <System/ErrorCodes.h>

#ifdef PADOS_MODULE_USER_SPACE

struct PThreadUserData;

enum class PUserspaceServiceCommand
{
    AllocateMemory,
    FreeMemory
};

struct PUserspaceServiceRequest
{
    PUserspaceServiceCommand Command;
    void* Memory = nullptr;
    size_t Size = 0;
};

// Initialized before main(); the service and its semaphores live for the device uptime.
// The kernel serializes synchronous memory requests. Thread cleanup uses a
// separate intrusive list and shares the request semaphore without a reply.
struct PUserspaceService
{
    std::atomic<bool> Pending = false;
    std::atomic<PThreadUserData*> FirstZombie = nullptr;
    sem_id RequestSemaphore = -1;
    sem_id CompletionSemaphore = -1;
    thread_id ThreadID = -1;
    PUserspaceServiceRequest Request = {};
    PErrorCode Result = PErrorCode::Success;
};

PErrorCode p_userspace_service_start();

#endif // PADOS_MODULE_USER_SPACE
