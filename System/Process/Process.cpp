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

#include <Process/Process.h>
#include <System/AppDefinition.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode spawn_execl(pid_t* outPID, const char* name, int priority, const char* arg0, ...)
{
    va_list args;

    int argc = 1;
    va_start(args, arg0);
    for (; va_arg(args, const char*) != nullptr; ++argc);
    va_end(args);

    const char** argv = static_cast<const char**>(alloca((argc + 1) * sizeof(const char*)));

    argv[0] = arg0;
    va_start(args, arg0);
    for (int i = 1; i < argc; ++i ) {
        argv[i] = va_arg(args, const char*);
    }
    va_end(args);
    argv[argc] = nullptr;

    return spawn_execv(outPID, name, priority, const_cast<char* const*>(argv));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode spawn_execle(pid_t* outPID, const char* name, int priority, const char* arg0, ...)
{
    va_list args;

    int argc = 1;
    va_start(args, arg0);
    for (; va_arg(args, const char*) != nullptr; ++argc);
    va_end(args);

    const char** argv = static_cast<const char**>(alloca((argc + 1) * sizeof(const char*)));

    argv[0] = arg0;
    va_start(args, arg0);
    for (int i = 1; i < argc; ++i) {
        argv[i] = va_arg(args, const char*);
    }
    char* const* envp = va_arg(args, char* const*);
    va_end(args);
    argv[argc] = nullptr;

    return spawn_execve(outPID, name, priority, const_cast<char* const*>(argv), envp);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode spawn_execv(pid_t* outPID, const char* name, int priority, char* const argv[])
{
    return spawn_execve(outPID, name, priority, argv, environ);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode spawn_execve(pid_t* outPID, const char* name, int priority, char* const argv[], char* const envp[])
{
    PThreadControlBlock* tlsBlock = create_thread_tls_block(__app_definition);
    if (tlsBlock == nullptr) {
        return PErrorCode::NoMemory;
    }
    size_t argc = 0;
    size_t totalArgSize = 0;
    if (argv != nullptr)
    {
        for (; argv[argc] != nullptr; ++argc) {
            totalArgSize += strlen(argv[argc]) + 1;
        }
    }
    char** argvCopy = static_cast<char**>(malloc(sizeof(char*) * (argc + 1) + totalArgSize));
    if (argvCopy == nullptr)
    {
        delete_thread_tls_block(tlsBlock);
        return PErrorCode::NoMemory;
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

    const PErrorCode result = __spawn_execve(outPID, name, priority, tlsBlock, argvCopy, envp);
    if (result != PErrorCode::Success) {
        delete_thread_tls_block(tlsBlock);
        free(argvCopy);
    }
    return result;
}
