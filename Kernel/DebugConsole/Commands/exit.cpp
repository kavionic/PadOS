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
// Created: 14.05.2026

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdExit : public KConsoleCommand
{
public:
    CCmdExit(KDebugConsole* console) : KConsoleCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        if (args.size() > 2) {
            Print("{}: too many arguments\n", args[0]);
            return 1;
        }

        int exitCode = m_Console->GetLastExitCode();

        if (args.size() == 2)
        {
            if (args[1] == "-h" || args[1] == "--help")
            {
                Print("{}: {} [n]\n    Exit the shell.\n\n    Exits the shell with a status of N.  If N is omitted, the exit status\n    is that of the last command executed.\n", args[0], args[0]);
                return 0;
            }
            try
            {
                size_t pos = 0;
                exitCode = std::stoi(args[1], &pos);
                if (pos != args[1].size()) {
                    throw std::invalid_argument("not a number");
                }
            }
            catch (...)
            {
                Print("{}: {}: numeric argument required\n", args[0], args[1]);
                return 1;
            }
        }
        m_Console->Terminate(exitCode);
        return exitCode;
    }

    virtual PString GetDescription() const override { return "Exit the shell."; }
};

static KConsoleCommandRegistrator<CCmdExit> g_RegisterCCmdExit("exit");

} // namespace kernel
