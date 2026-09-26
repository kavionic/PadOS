// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.03.2026 16:00

#pragma once
#include <Threads/Threads.h>
#include <System/GProf.h>


struct PFirmwareImageDefinition;
struct PPosixSpawnFileActions;

PThreadControlBlock* create_thread_tls_block(const PFirmwareImageDefinition& imageDefinition, void* buffer = nullptr);
void delete_thread_tls_block(PThreadControlBlock* tlsBlock);

#ifdef PADOS_MODULE_USER_SPACE
struct PThreadUserData
{
    pid_t                   ThreadID;
    PThreadUserData*        NextZombie;
    size_t                  StackSize;
    void*                   StackBuffer;
    PThreadControlBlock*    TLSData;
    PPosixSpawnFileActions* SpawnFileActions;
    bool                    IsStackUserProvided;
    bool                    CancellationPending;
    bool                    IsCanceled;
    bool                    IsCanceling;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    PGProfThreadState       GProfState;
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
};


void __thread_terminated(void* returnValue, PThreadUserData* threadData);


void             p_set_thread_user_data(PThreadUserData* threadData);
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
__attribute__((no_instrument_function, target("general-regs-only")))
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
PThreadUserData* p_get_thread_user_data();

PThreadUserData* create_thread_user_data(const PFirmwareImageDefinition& imageDefinition, PThreadAttribs& attribs);
void delete_thread_user_data(PThreadUserData* threadData);

#endif // PADOS_MODULE_USER_SPACE
