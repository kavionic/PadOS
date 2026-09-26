// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 10.01.2026 17:00

#include <map>

#include <System/AppDefinition.h>
#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdHelp : public KConsoleCommand
{
public:
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        const std::map<PString, KDebugConsole::CommandEntry>& commands = KDebugConsole::GetCommands();
        const std::vector<const PAppDefinition*> apps = PAppDefinition::GetApplicationList();

        std::map<PString, PString> commandNames;

        for (const auto& command : commands) {
            commandNames[command.first] = command.second.Description;
        }

        for (const PAppDefinition* app : apps)
        {
            if (app->Description != nullptr && app->Description[0] != '\0') {
                commandNames[app->Name] = app->Description;
            }
        }
        size_t longestName = 0;
        for (auto cmdNode : commandNames)
        {
            if (cmdNode.first.size() > longestName) longestName = cmdNode.first.size();
        }
        for (auto cmdNode : commandNames)
        {
            Print("{:{}} - {}\n", cmdNode.first, longestName + 1, cmdNode.second);
        }
        return 0;
    }

    static PString GetDescription() { return "List available commands."; }

private:
};

static KConsoleCommandRegistrator<CCmdHelp> g_RegisterCCmdHelp("help");

} // namespace kernel
