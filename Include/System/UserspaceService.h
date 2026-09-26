// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
