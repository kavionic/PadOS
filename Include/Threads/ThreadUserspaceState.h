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

#pragma once
#include <Threads/Threads.h>


struct PFirmwareImageDefinition;
struct PPosixSpawnFileActions;

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
};

struct PThreadReaperQueue
{
    std::atomic<PThreadUserData*>   FirstZombie;
    sem_id                          Semaphore;
};


void __thread_terminated(void* returnValue, PThreadUserData* threadData);


void             p_set_thread_user_data(PThreadUserData* threadData);
PThreadUserData* p_get_thread_user_data();

PThreadUserData* create_thread_user_data(const PFirmwareImageDefinition& imageDefinition, PThreadAttribs& attribs);
void delete_thread_user_data(PThreadUserData* threadData);

PThreadControlBlock* create_thread_tls_block(const PFirmwareImageDefinition& imageDefinition, void* buffer = nullptr);
void delete_thread_tls_block(PThreadControlBlock* tlsBlock);

void p_thread_reaper_run();
void p_thread_reaper_schedule_cleanup(PThreadUserData* threadData);
