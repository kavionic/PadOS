// This file is part of PadOS.
//
// Copyright (C) 2025-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 25.11.2025 01:00

#pragma once

#include <vector>
#include <atomic>

#include <stdint.h>
#include <signal.h>

#include <PadOS/Threads.h>
#include <System/ModuleTLSDefinition.h>

struct PThreadUserData;
struct PThreadReaperQueue;

#ifdef PADOS_MODULE_GPROF_SAMPLING
inline constexpr uint32_t PFIRMWARE_PROFILE_INFO_MAGIC = 0x464f5250;
inline constexpr uint16_t PFIRMWARE_PROFILE_INFO_VERSION = 1;
inline constexpr uint16_t PFIRMWARE_PROFILE_REGION_COUNT = 3;

struct PFirmwareExecutableRegion
{
    const uint8_t* Start;
    const uint8_t* End;
};

struct PFirmwareProfileInfo
{
    uint32_t Magic;
    uint16_t Version;
    uint16_t RegionCount;
    PFirmwareExecutableRegion Regions[PFIRMWARE_PROFILE_REGION_COUNT];
};
#endif // PADOS_MODULE_GPROF_SAMPLING

class PAppDefinition
{
public:
    PAppDefinition(const char* name, const char* description, int (*mainEntry)(int argc, char* argv[]), size_t stackSize = 0);
    ~PAppDefinition();

    static const PAppDefinition* FindApplication(const char* name);
    static std::vector<const PAppDefinition*> GetApplicationList();

    const char* Name = nullptr;
    const char* Description = nullptr;

    int (*MainEntry)(int argc, char* argv[]);
    size_t StackSize = 0;

    static PAppDefinition* s_FirstApp;
private:
    PAppDefinition* m_NextApp = nullptr;
};

struct PFirmwareImageDefinition
{
    void (*entry)(
#ifdef PADOS_MODULE_USER_SPACE
        PThreadUserData* threadData
#endif // PADOS_MODULE_USER_SPACE
        );
    void (*process_entry_trampoline)(PThreadUserData* threadData, ThreadEntryPoint_t threadEntry, void* arguments);
    void (*thread_terminated)(void* returnValue, PThreadUserData*);
    void (*signal_trampoline)();
    void (*signal_terminate_thread)(int, siginfo_t*, void*);
    PThreadUserData* (*create_main_thread_user_data)();
    PThreadUserData* (*create_thread_user_data)(PThreadAttribs& attribs);
    void*            (*alloc_memory)(size_t size);
    void             (*free_memory)(void* data);

    PAppDefinition*&        FirstAppPointer;
#ifdef PADOS_MODULE_USER_SPACE
    PThreadReaperQueue*     ThreadReaperQueue;
#endif // PADOS_MODULE_USER_SPACE
    PModuleTLSDefinition    TLSDefinition;
#ifdef PADOS_MODULE_GPROF_SAMPLING
    PFirmwareProfileInfo    ProfileInfo;
#endif // PADOS_MODULE_GPROF_SAMPLING
};

extern "C"
{

#ifdef PADOS_MODULE_USER_SPACE
extern PThreadControlBlock* __app_thread_data;
extern PFirmwareImageDefinition __kernel_definition;
#else
#define __kernel_definition __app_definition
#endif

extern PFirmwareImageDefinition __app_definition;

} // extern "C"

