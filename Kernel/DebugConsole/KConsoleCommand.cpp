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
// Created: 10.01.2026 15:30

#pragma once

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
    return write(m_Console->GetStdOutFD(), data, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KConsoleCommandRegistratorBase::RegisterCommand(const PString& name, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>&& commandCreator)
{
    KDebugConsole::RegisterCommand(name, std::move(commandCreator));
}


} // namespace kernel
