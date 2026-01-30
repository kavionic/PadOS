// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 11.03.2018 13:10:28

#include <cstdlib>
#include <sys/reent.h>
#include <sys/pados_syscalls.h>
#include <PadOS/Threads.h>
#include <Threads/Thread.h>
#include <Threads/Threads.h>
#include <System/AppDefinition.h>
#include <System/System.h>
#include <Utils/Logging.h>
#include <Utils/Utils.h>
#include <Kernel/KThreadCB.h>


thread_local PThread* PThread::st_CurrentThread = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThread::PThread(const PString& name) : m_Name(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThread::~PThread()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThread* PThread::GetCurrentThread()
{
    return st_CurrentThread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode PThread::Start(PThreadDetachState detachState, int priority, int stackSize)
{
    if (m_ThreadHandle == INVALID_HANDLE)
    {
        m_DetachState = detachState;
        PThreadAttribs attrs(m_Name.c_str(), priority, detachState, stackSize);
        return thread_spawn(&m_ThreadHandle, &attrs, ThreadEntry, this);
    }
    return PErrorCode::Busy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode PThread::Join(void** outReturnValue, TimeValNanos deadline)
{
    PErrorCode result = PErrorCode::InvalidArg;
    if (m_DetachState == PThreadDetachState_Joinable)
    {
        result = thread_join(m_ThreadHandle, outReturnValue);
        if (result == PErrorCode::Success && m_DeleteOnExit) {
            delete this;
        }
        return result;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PThread::Run()
{
    if (!VFRun.Empty()) {
        return VFRun(this);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void PThread::Exit(void* returnValue)
{
    if (m_DetachState == PThreadDetachState_Detached && m_DeleteOnExit) {
        delete this;
    }
    thread_exit(returnValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PThread::ThreadEntry(void* data)
{
    PThread* self = static_cast<PThread*>(data);

    try
    {
        st_CurrentThread = self;
        self->Exit(self->Run());
    }
    catch(const std::exception& e)
    {
        p_system_log<PLogSeverity::FATAL>(LogCat_Threads, "Uncaught exception in thread {}: {}", self->GetName(), e.what());
        self->Exit(nullptr);
    }
    catch (...)
    {
        p_system_log<PLogSeverity::FATAL>(LogCat_Threads, "Uncaught exception in thread {}: unknown", self->GetName());
        self->Exit(nullptr);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode PThread::Adopt()
{
    if (m_ThreadHandle == INVALID_HANDLE)
    {
        m_ThreadHandle = get_thread_id();
//        m_DetachState = detachState;
        ThreadEntry(this);

        return PErrorCode::Success;
    }
    return PErrorCode::Busy;

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __thread_terminated(thread_id threadID, void* stackBuffer, PThreadControlBlock* threadLocalBuffer)
{
    _reclaim_reent(nullptr);

    PThreadLocalSlotManager::Get().ThreadTerminated();

    delete_thread_tls_block(threadLocalBuffer);
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

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode thread_spawn(thread_id* outHandle, const PThreadAttribs* attribs, ThreadEntryPoint_t entryPoint, void* arguments)
{
    PThreadControlBlock* tlsBlock = create_thread_tls_block(__app_definition);
    if (tlsBlock == nullptr) {
        return PErrorCode::NoMemory;
    }
    const PErrorCode result = __thread_spawn(outHandle, attribs, tlsBlock, entryPoint, arguments);
    if (result != PErrorCode::Success) {
        delete_thread_tls_block(tlsBlock);
    }
    return result;
}


#ifdef __cplusplus
}
#endif

