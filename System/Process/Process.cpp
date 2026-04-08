// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 21.12.2025 17:30

#include <spawn.h>
#include <errno.h>
#include <Process/Process.h>
#include <System/AppDefinition.h>
#include <Threads/ThreadUserspaceState.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __process_entry_trampoline(PThreadUserData* threadData, ThreadEntryPoint_t threadEntry, void* arguments)
{
    try
    {
        p_set_thread_user_data(threadData);
        const PAppDefinition* app = static_cast<const PAppDefinition*>(__app_thread_data->Ptr1);

        char** argv = static_cast<char**>(__app_thread_data->Ptr2);

        PScopeExit cleanup([argv]() { __app_definition.free_memory(argv); });

        int argc = 0;
        if (argv != nullptr) {
            for (; argv[argc] != nullptr; ++argc);
        }
        int result = app->MainEntry(argc, argv);
        exit(result);
    }
    PRETHROW_CANCELLATION
    catch (const std::exception& exc)
    {
        ThreadInfo threadInfo;
        get_thread_info(get_thread_id(), &threadInfo);
        p_system_log<PLogSeverity::NOTICE>(LogCat_Threads, "Uncaught exception in thread '{}': {}", threadInfo.ThreadName, exc.what());
        exit(-1);
    }
    catch (...)
    {
        ThreadInfo threadInfo;
        get_thread_info(get_thread_id(), &threadInfo);
        p_system_log<PLogSeverity::NOTICE>(LogCat_Threads, "Unknown uncaught exception in thread {}.", threadInfo.ThreadName);
        exit(-1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn(pid_t* __restrict outPID,
                const char* __restrict path,
                const posix_spawn_file_actions_t* /*file_actions*/,
                const posix_spawnattr_t* __restrict /*attrp*/,
                char* const argv[],
                char* const envp[])
{
    PThreadAttribs attribs(nullptr);

    PThreadUserData* const threadData = __app_definition.create_thread_user_data(attribs);

    if (threadData == nullptr) {
        return ENOMEM;
    }
    size_t argc = 0;
    size_t totalArgSize = 0;
    if (argv != nullptr)
    {
        for (; argv[argc] != nullptr; ++argc) {
            totalArgSize += strlen(argv[argc]) + 1;
        }
    }
    char** argvCopy = static_cast<char**>(__app_definition.alloc_memory(sizeof(char*) * (argc + 1) + totalArgSize));
    if (argvCopy == nullptr)
    {
        delete_thread_user_data(threadData);
        return ENOMEM;
    }
    char* textBuffer = reinterpret_cast<char*>(argvCopy + argc + 1);
    for (size_t i = 0; i < argc; ++i)
    {
        const size_t argLen = strlen(argv[i]);
        strcpy(textBuffer, argv[i]);
        argvCopy[i] = textBuffer;
        textBuffer += argLen + 1;
    }
    argvCopy[argc] = nullptr;

    const int result = __posix_spawn(outPID, __app_definition.process_entry_trampoline, path, 0, threadData, argvCopy, envp != nullptr ? envp : environ);
    if (result != 0) {
        delete_thread_user_data(threadData);
        __app_definition.free_memory(argvCopy);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawnp(pid_t* __restrict outPID,
                 const char* __restrict file,
                 const posix_spawn_file_actions_t* file_actions,
                 const posix_spawnattr_t* __restrict attrp,
                 char* const argv[],
                 char* const envp[])
{
    return posix_spawn(outPID, file, file_actions, attrp, argv, envp);
}
