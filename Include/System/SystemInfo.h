// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18.07.2020 00:00

#pragma once


typedef struct
{
    int64_t     KernelVersion;
    char        KernelName[OS_NAME_LENGTH];	 	    // Name of kernel image.
    char        KernelBuildDate[OS_NAME_LENGTH];	// Date of kernel built.
    char        KernelBuildTime[OS_NAME_LENGTH];	// Time of kernel built.
    bigtime_t   BootTime;				            // Time of boot (# usec since 1/1/70).
    PCPUInfo    PCPUInfo;
    int         MutexCount;                 // Number of mutexes in use.
    int         SemaphoreCount;				// Number of semaphores in use.
    int         ConditionVariableCount;     // Number of condition variables in use.
    int         PortCount;				    // Number of message ports in use.
    int         ThreadCount;			 	// Number of living threads.
    int         ProcessCount;			 	// Number of living processes.

    int	        OpenFileCount;
    int         AllocatedInodes;
    int         LoadedInodes;
    int         UsedInodes;
    int	        BlockCacheSize;
    int	        DirtyCacheSize;
    int	        LockedCacheBlocks;
} PSystemInfo;
