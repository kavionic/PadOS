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
// Created: 10.01.2026 17:00

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdHelp : public KConsoleCommand
{
public:
    CCmdHelp(KDebugConsole* console) : KConsoleCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        std::map<PString, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>> commands = KDebugConsole::GetCommands();
        const std::vector<const PAppDefinition*> apps = PAppDefinition::GetApplicationList();

        std::map<PString, PString> commandNames;

        for (auto cmdNode : commands)
        {
            commandNames[cmdNode.first] = cmdNode.second(m_Console)->GetDescription();
        }

        for (const PAppDefinition* app : apps)
        {
            commandNames[app->Name] = app->Description;
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

    virtual PString GetDescription() const override { return "List available commands."; }

private:
};

static KConsoleCommandRegistrator<CCmdHelp> g_RegisterCCmdHelp("help");

} // namespace kernel
