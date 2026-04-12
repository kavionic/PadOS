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
// Created: 21.12.2025 17:30

#include <spawn.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <Kernel/KPosixSpawn.h>
#include <Process/Process.h>
#include <System/AppDefinition.h>
#include <Threads/ThreadUserspaceState.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PSpawnFileAction* create_file_action(const char* path)
{
    const size_t pathLen = (path != nullptr) ? strlen(path) : 0;

    PSpawnFileAction* action = static_cast<PSpawnFileAction*>(
        __app_definition.alloc_memory(sizeof(PSpawnFileAction) + pathLen + 1));

    if (action == nullptr) {
        return nullptr;
    }

    new (action) PSpawnFileAction();
    
    char* const pathDst = action->GetPath();
    
    if (pathLen > 0) {
        memcpy(pathDst, path, pathLen);
    }
    pathDst[pathLen] = '\0';

    return action;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void free_file_actions(PPosixSpawnFileActions* fileActions) noexcept
{
    PSpawnFileAction* node;
    while ((node = fileActions->Actions.GetFirst()) != nullptr)
    {
        fileActions->Actions.Remove(node);
        __app_definition.free_memory(node);
    }
    __app_definition.free_memory(fileActions);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PPosixSpawnFileActions* duplicate_file_actions(const PPosixSpawnFileActions* src)
{
    PPosixSpawnFileActions* dst = static_cast<PPosixSpawnFileActions*>(
            __app_definition.alloc_memory(sizeof(PPosixSpawnFileActions))
        );

    if (dst == nullptr) {
        return nullptr;
    }
    new (dst) PPosixSpawnFileActions();

    for (const PSpawnFileAction* action : src->Actions)
    {
        PSpawnFileAction* actionCopy = create_file_action(action->GetPath());

        if (actionCopy == nullptr)
        {
            free_file_actions(dst);
            return nullptr;
        }
        actionCopy->Data = action->Data;
        dst->Actions.Append(actionCopy);
    }
    return dst;
}

static void execute_file_actions(PPosixSpawnFileActions* fileActions)
{
    try
    {
        PScopeExit cleanup([fileActions]() { free_file_actions(fileActions); });

        for (const PSpawnFileAction* action : fileActions->Actions)
        {
            switch (action->Data.ActionType)
            {
            case PSpawnFileActionType::Close:
                close(action->Data.FD);
                break;

            case PSpawnFileActionType::Dup2:
                if (dup2(action->Data.FD, action->Data.Dup2.NewFD) == -1) {
                    throw errno;
                }
                break;

            case PSpawnFileActionType::Open:
            {
                const int fd = open(action->GetPath(), action->Data.Open.OFlag, action->Data.Open.Mode);
                if (fd == -1) {
                    throw errno;
                }
                if (fd != action->Data.FD)
                {
                    if (dup2(fd, action->Data.FD) == -1)
                    {
                        close(fd);
                        throw errno;
                    }
                    close(fd);
                }
                break;
            }

            case PSpawnFileActionType::Chdir:
                if (chdir(action->GetPath()) == -1) {
                    throw errno;
                }
                break;

            case PSpawnFileActionType::Fchdir:
                if (fchdir(action->Data.FD) == -1) {
                    throw errno;
                }
                break;
            }
        }
    }
    catch (int errorCode)
    {
        exit(errorCode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __process_entry_trampoline(PThreadUserData* threadData, ThreadEntryPoint_t threadEntry, void* arguments)
{
    try
    {
        p_set_thread_user_data(threadData);

        if (threadData->SpawnFileActions != nullptr)
        {
            PPosixSpawnFileActions* const fileActions = threadData->SpawnFileActions;
            threadData->SpawnFileActions = nullptr;
            execute_file_actions(fileActions);
        }

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
                const posix_spawn_file_actions_t* __restrict fileActions,
                const posix_spawnattr_t* __restrict spawnAttr,
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

    if (fileActions != nullptr && *fileActions != nullptr)
    {
        threadData->SpawnFileActions = duplicate_file_actions(*fileActions);
        if (threadData->SpawnFileActions == nullptr)
        {
            delete_thread_user_data(threadData);
            __app_definition.free_memory(argvCopy);
            return ENOMEM;
        }
    }

    const int result = __posix_spawn(
        outPID,
        __app_definition.process_entry_trampoline,
        path,
        (spawnAttr != nullptr) ? *spawnAttr : nullptr,
        threadData,
        argvCopy,
        (envp != nullptr) ? envp : environ
    );

    if (result != 0)
    {
        if (threadData->SpawnFileActions != nullptr) {
            free_file_actions(threadData->SpawnFileActions);
        }
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
                 const posix_spawn_file_actions_t* __restrict file_actions,
                 const posix_spawnattr_t* __restrict attrp,
                 char* const argv[],
                 char* const envp[])
{
    return posix_spawn(outPID, file, file_actions, attrp, argv, envp);
}

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawnattr_init(posix_spawnattr_t* inAttr)
{
    PPosixSpawnAttribs* attr = static_cast<PPosixSpawnAttribs*>(__app_definition.alloc_memory(sizeof(PPosixSpawnAttribs)));
    if (attr == nullptr) {
        return ENOMEM;
    }
    const int result = __posix_spawnattr_init(attr, sizeof(*attr));
    if (result != 0)
    {
        __app_definition.free_memory(attr);
        return result;
    }

    *inAttr = attr;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawnattr_destroy(posix_spawnattr_t* attr) noexcept
{
    __posix_spawnattr_destroy(*attr);
    __app_definition.free_memory(*attr);
    *attr = nullptr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_init(posix_spawn_file_actions_t* outActions)
{
    PPosixSpawnFileActions* actions = static_cast<PPosixSpawnFileActions*>(
            __app_definition.alloc_memory(sizeof(PPosixSpawnFileActions))
        );

    if (actions == nullptr) {
        return ENOMEM;
    }
    new (actions) PPosixSpawnFileActions();
    *outActions = actions;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* actions) noexcept
{
    free_file_actions(*actions);
    *actions = nullptr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t* __restrict actions, int fd, const char* __restrict path, int oflag, mode_t mode)
{
    PSpawnFileAction* action = create_file_action(path);
    if (action == nullptr) { return ENOMEM; }
    action->Data.ActionType = PSpawnFileActionType::Open;
    action->Data.FD = fd;
    action->Data.Open.OFlag = oflag;
    action->Data.Open.Mode = mode;
    (*actions)->Actions.Append(action);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* actions, int fd, int newfd)
{
    PSpawnFileAction* action = create_file_action(nullptr);
    if (action == nullptr) { return ENOMEM; }
    action->Data.ActionType = PSpawnFileActionType::Dup2;
    action->Data.FD = fd;
    action->Data.Dup2.NewFD = newfd;
    (*actions)->Actions.Append(action);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* actions, int fd)
{
    PSpawnFileAction* action = create_file_action(nullptr);
    if (action == nullptr) { return ENOMEM; }
    action->Data.ActionType = PSpawnFileActionType::Close;
    action->Data.FD = fd;
    (*actions)->Actions.Append(action);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t* __restrict actions, const char* __restrict path)
{
    PSpawnFileAction* action = create_file_action(path);
    if (action == nullptr) { return ENOMEM; }
    action->Data.ActionType = PSpawnFileActionType::Chdir;
    action->Data.FD = -1;
    (*actions)->Actions.Append(action);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_addfchdir(posix_spawn_file_actions_t* __restrict actions, int fd)
{
    PSpawnFileAction* action = create_file_action(nullptr);
    if (action == nullptr) { return ENOMEM; }
    action->Data.ActionType = PSpawnFileActionType::Fchdir;
    action->Data.FD = fd;
    (*actions)->Actions.Append(action);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t* __restrict actions, const char* __restrict path)
{
    return posix_spawn_file_actions_addchdir(actions, path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int posix_spawn_file_actions_addfchdir_np(posix_spawn_file_actions_t* __restrict actions, int fd)
{
    return posix_spawn_file_actions_addfchdir(actions, fd);
}

} // extern "C"
