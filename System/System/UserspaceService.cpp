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
