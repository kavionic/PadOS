// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.09.2026 00:00

#include <stdlib.h>

#include <System/AppDefinition.h>
#include <System/ExceptionHandling.h>
#include <System/UserspaceService.h>
#include <Threads/ThreadUserspaceState.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void PUserspaceServiceExecute(PUserspaceService& service)
{
    PUserspaceServiceRequest& request = service.Request;
    service.Result = PErrorCode::Success;
    switch (request.Command)
    {
        case PUserspaceServiceCommand::AllocateMemory:
            request.Memory = malloc(request.Size);
            if (request.Memory == nullptr) {
                service.Result = PErrorCode::NOMEM;
            }
            break;
        case PUserspaceServiceCommand::FreeMemory:
            free(request.Memory);
            request.Memory = nullptr;
            break;
        default:
            service.Result = PErrorCode::INVAL;
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void PUserspaceServiceReapThreads(PUserspaceService& service)
{
    PThreadUserData* currentThread = service.FirstZombie.exchange(nullptr, std::memory_order_acquire);
    while (currentThread != nullptr)
    {
        PThreadUserData* nextThread = currentThread->NextZombie;
        delete_thread_user_data(currentThread);
        currentThread = nextThread;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void* PUserspaceServiceRun(void* argument)
{
    PUserspaceService& service = *static_cast<PUserspaceService*>(argument);
    for (;;)
    {
        PERROR_ERRORCODE_THROW_ON_FAIL(semaphore_acquire(service.RequestSemaphore));
        if (service.Pending.load(std::memory_order_acquire))
        {
            PUserspaceServiceExecute(service);
            service.Pending.store(false, std::memory_order_release);
            PERROR_ERRORCODE_THROW_ON_FAIL(semaphore_release(service.CompletionSemaphore));
        }
        PUserspaceServiceReapThreads(service);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode p_userspace_service_start()
{
    PUserspaceService& service = *__app_definition.UserspaceService;
    PErrorCode result = semaphore_create(&service.RequestSemaphore, "userspace_request", CLOCK_MONOTONIC_COARSE, 0);
    if (result != PErrorCode::Success) {
        return result;
    }
    result = semaphore_create(&service.CompletionSemaphore, "userspace_complete", CLOCK_MONOTONIC_COARSE, 0);
    if (result != PErrorCode::Success)
    {
        semaphore_delete(service.RequestSemaphore);
        return result;
    }

    PThreadAttribs attributes("userspace_service", 20, PThreadDetachState_Detached);
    result = thread_spawn(&service.ThreadID, &attributes, PUserspaceServiceRun, &service);
    if (result != PErrorCode::Success)
    {
        semaphore_delete(service.CompletionSemaphore);
        semaphore_delete(service.RequestSemaphore);
    }
    return result;
}
