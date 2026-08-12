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

#include <functional>
#include <type_traits>
#include <vector>
#include <unistd.h>

#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include <Utils/String.h>

namespace kernel
{
class KDebugConsole;

class KConsoleCommand : public PtrTarget
{
public:
    template<typename ...ARGS>
    void Print(PFormatString<ARGS...>&& fmt, ARGS&&... args)
    {
        const PString text = PString::format_string(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...);
        WriteOutput(text.c_str(), text.size());
    }
    ssize_t WriteOutput(const void* data, size_t length);

    virtual int Invoke(std::vector<std::string>&& args) = 0;

    virtual PString ExpandArgument(const std::vector<PString>& args, size_t argIndex, size_t cursorPos) { return PString::zero; }
};

class KConsoleInternalCommand : public KConsoleCommand
{
protected:
    KConsoleInternalCommand(KDebugConsole* console) : m_Console(console) {}

    KDebugConsole* m_Console = nullptr;
};

class KConsoleCommandRegistratorBase
{
protected:
    void RegisterCommand(const PString& name, const PString& description, bool isInternal, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>&& commandCreator);
};

template<typename T>
class KConsoleCommandRegistrator : public KConsoleCommandRegistratorBase
{
public:
    KConsoleCommandRegistrator(const PString name)
    {
        if constexpr (std::is_base_of_v<KConsoleInternalCommand, T>) {
            RegisterCommand(name, T::GetDescription(), true, [](KDebugConsole* console)
                {
                    return ptr_new<T>(console);
                }
            );
        } else {
            RegisterCommand(name, T::GetDescription(), false, [](KDebugConsole*)
                {
                    return ptr_new<T>();
                }
            );
        }
    }
};


} // namespace kernel
