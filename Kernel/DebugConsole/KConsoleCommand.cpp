// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 10.01.2026 15:30

#include <vector>

#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t KConsoleCommand::WriteOutput(const void* data, size_t length)
{
    return write(STDOUT_FILENO, data, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KConsoleCommandRegistratorBase::RegisterCommand(const PString& name, const PString& description, bool isInternal, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>&& commandCreator)
{
    KDebugConsole::RegisterCommand(name, description, isInternal, std::move(commandCreator));
}


} // namespace kernel
