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
// Created: 22.03.2026 16:00

#include <System/AppDefinition.h>
#include <Threads/ThreadUserspaceState.h>
#include <Threads/ThreadLocal.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __thread_terminated(void* returnValue, PThreadUserData* threadData)
{
    _reclaim_reent(nullptr);

    PThreadLocalSlotManager::Get().ThreadTerminated();

    if (threadData->TLSData != nullptr)
    {
        delete_thread_tls_block(threadData->TLSData);
        threadData->TLSData = nullptr;
    }
    thread_terminate(returnValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThreadUserData* create_thread_user_data(const PFirmwareImageDefinition& imageDefinition, PThreadAttribs& attribs)
{
    PThreadUserData* threadData;

    try
    {
        threadData = new PThreadUserData;
    }
    catch (const std::bad_alloc& exc)
    {
        return nullptr;
    }
    memset(threadData, 0, sizeof(*threadData));

    threadData->IsStackUserProvided = attribs.StackAddress != nullptr;
    if (!threadData->IsStackUserProvided)
    {
        attribs.StackSize = (attribs.StackSize != 0) ? align_up(attribs.StackSize, KSTACK_ALIGNMENT) : THREAD_DEFAULT_STACK_SIZE;
        attribs.StackAddress = aligned_alloc(KSTACK_ALIGNMENT, attribs.StackSize);
    }

    if (attribs.StackAddress != nullptr)
    {
        threadData->StackSize = attribs.StackSize;
        threadData->StackBuffer = attribs.StackAddress;

        threadData->TLSData = create_thread_tls_block(__app_definition);

        if (threadData->TLSData != nullptr) {
            return threadData;
        }
        if (!threadData->IsStackUserProvided) {
            free(threadData->StackBuffer);
        }
    }
    delete threadData;
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void delete_thread_user_data(PThreadUserData* threadData)
{
    if (threadData->IsStackUserProvided && threadData->StackBuffer != nullptr) {
        free(threadData->StackBuffer);
    }
    if (threadData->TLSData != nullptr) {
        delete_thread_tls_block(threadData->TLSData);
    }
    delete threadData;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThreadControlBlock* create_thread_tls_block(const PFirmwareImageDefinition& imageDefinition, void* buffer)
{
    static_assert(sizeof(PThreadControlBlock) == 8);

    const size_t controlBlockSize = sizeof(PThreadControlBlock) + imageDefinition.TLSDefinition.TLSDataSize + imageDefinition.TLSDefinition.TLSBSSSize;
    assert(imageDefinition.TLSDefinition.TLSAlign <= sizeof(PThreadControlBlock));

    PThreadControlBlock* controlBlock =
        (buffer == nullptr)
        ? reinterpret_cast<PThreadControlBlock*>(aligned_alloc(imageDefinition.TLSDefinition.TLSAlign, align_up(controlBlockSize, imageDefinition.TLSDefinition.TLSAlign)))
        : reinterpret_cast<PThreadControlBlock*>(buffer);

    if (controlBlock == nullptr) {
        return nullptr;
    }
    memset(controlBlock, 0, sizeof(*controlBlock));
    memcpy(controlBlock + 1, imageDefinition.TLSDefinition.TLSData, imageDefinition.TLSDefinition.TLSDataSize);
    memset(reinterpret_cast<uint8_t*>(controlBlock + 1) + imageDefinition.TLSDefinition.TLSDataSize, 0, imageDefinition.TLSDefinition.TLSBSSSize);
    return controlBlock;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void delete_thread_tls_block(PThreadControlBlock* tlsBlock)
{
    if (tlsBlock != nullptr) {
        free(tlsBlock);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void p_thread_reaper_run()
{
    sem_id semaphore;
    semaphore_create(&semaphore, "zombie_thread_usr", CLOCK_MONOTONIC_COARSE, 0);

    __app_definition.ThreadReaperQueue->Semaphore = semaphore;

    for (;;)
    {
        semaphore_acquire(semaphore);

        PThreadUserData* currentThread = __app_definition.ThreadReaperQueue->FirstZombie.exchange(nullptr, std::memory_order_acquire);

        while (currentThread != nullptr)
        {
            PThreadUserData* nextThread = currentThread->NextZombie;

            delete_thread_user_data(currentThread);

            currentThread = nextThread;
        }

    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void p_thread_reaper_schedule_cleanup(PThreadUserData* threadData)
{
    if (threadData != nullptr)
    {
        PThreadUserData* oldHead = __app_definition.ThreadReaperQueue->FirstZombie.load(std::memory_order_relaxed);
        do {
            threadData->NextZombie = oldHead;
        } while (!__app_definition.ThreadReaperQueue->FirstZombie.compare_exchange_weak(oldHead, threadData, std::memory_order_release, std::memory_order_relaxed));

        semaphore_release(__app_definition.ThreadReaperQueue->Semaphore);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void thread_entry_trampoline(ThreadEntryPoint_t threadEntry, void* arguments)
{
    try
    {
        void* const result = threadEntry(arguments);
        thread_exit(result);
    }
    catch (const std::exception& exc)
    {
        ThreadInfo threadInfo;
        get_thread_info(get_thread_id(), &threadInfo);
        p_system_log<PLogSeverity::NOTICE>(LogCat_Threads, "Uncaught exception in thread '{}': {}", threadInfo.ThreadName, exc.what());
        thread_exit(nullptr);
    }
    catch (...)
    {
        ThreadInfo threadInfo;
        get_thread_info(get_thread_id(), &threadInfo);
        p_system_log<PLogSeverity::NOTICE>(LogCat_Threads, "Unknown uncaught exception in thread {}.", threadInfo.ThreadName);
        thread_exit(nullptr);
    }
}

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode thread_spawn(thread_id* outHandle, const PThreadAttribs* inAttribs, ThreadEntryPoint_t entryPoint, void* arguments)
{
    PThreadAttribs attribs = (inAttribs != nullptr) ? *inAttribs : PThreadAttribs(nullptr);

    PThreadUserData* const threadData = __app_definition.create_thread_user_data(attribs);

    if (threadData != nullptr)
    {
        const PErrorCode result = __thread_spawn(outHandle, &attribs, threadData, thread_entry_trampoline, entryPoint, arguments);
        if (result == PErrorCode::Success) {
            return result;
        }
        delete_thread_user_data(threadData);

        return result;
    }
    return PErrorCode::NoMemory;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

__attribute__((naked)) void thread_exit(void* returnValue)
{
    __asm volatile (
    "ldr    r12, =%0\n"
        "svc    0\n"
        :: "i"(SYS_thread_exit) : "r12", "memory", "cc"
        );
}

#ifdef __cplusplus
}
#endif
