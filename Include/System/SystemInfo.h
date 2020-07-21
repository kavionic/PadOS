// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 18.07.2020 00:00

#pragma once

namespace os
{


typedef struct
{
    int64_t     KernelVersion;
    char        KernelName[OS_NAME_LENGTH];	 	    // Name of kernel image.
    char        KernelBuildDate[OS_NAME_LENGTH];	// Date of kernel built.
    char        KernelBuildTime[OS_NAME_LENGTH];	// Time of kernel built.
    bigtime_t   BootTime;				            // Time of boot (# usec since 1/1/70).
    CPUInfo     CPUInfo;
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
} system_info;

} // namespace
