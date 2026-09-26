// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.09.2026 00:00

#include <Kernel/KMutex.h>
#include <Kernel/KSemaphore.h>
#include <Kernel/KThread.h>
#include <Kernel/KUserspaceService.h>
#include <System/AppDefinition.h>
#include <Threads/ThreadUserspaceState.h>

namespace kernel
{

static KMutex gk_UserspaceServiceMutex("userspace_service", PEMutexRecursionMode_RaiseError);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kuserspace_service_request(PUserspaceServiceRequest& request)
{
    PUserspaceService& service = *__app_definition.UserspaceService;
    if (service.ThreadID == kget_thread_id()) {
        return PErrorCode::DEADLK;
    }
    KScopedLock lock(gk_UserspaceServiceMutex);
    const Ptr<KSemaphore> requestSemaphore = KNamedObject::GetObject<KSemaphore>(service.RequestSemaphore);
    const Ptr<KSemaphore> completionSemaphore = KNamedObject::GetObject<KSemaphore>(service.CompletionSemaphore);
    if (requestSemaphore == nullptr || completionSemaphore == nullptr) {
        return PErrorCode::INVAL;
    }

    service.Request = request;
    service.Pending.store(true, std::memory_order_release);
    requestSemaphore->Release();

    // Holding a reference keeps the completion semaphore alive. Acquire waits
    // through signals, so the shared slot cannot be reused before completion.
    const PErrorCode result = completionSemaphore->Acquire();
    if (result != PErrorCode::Success) {
        return result;
    }
    if (service.Pending.load(std::memory_order_acquire)) {
        return PErrorCode::IO;
    }
    request = service.Request;
    return service.Result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kuserspace_service_schedule_thread_cleanup(PThreadUserData* threadData)
{
    if (threadData == nullptr) {
        return PErrorCode::Success;
    }
    PUserspaceService& service = *__app_definition.UserspaceService;
    const Ptr<KSemaphore> requestSemaphore = KNamedObject::GetObject<KSemaphore>(service.RequestSemaphore);
    if (requestSemaphore == nullptr) {
        return PErrorCode::INVAL;
    }

    PThreadUserData* oldHead = service.FirstZombie.load(std::memory_order_relaxed);
    do {
        threadData->NextZombie = oldHead;
    } while (!service.FirstZombie.compare_exchange_weak(
        oldHead,
        threadData,
        std::memory_order_release,
        std::memory_order_relaxed
    ));

    if (oldHead == nullptr) {
        requestSemaphore->Release();
    }
    return PErrorCode::Success;
}

} // namespace kernel
