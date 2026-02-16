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
// Created: 18.01.2026 22:30

#include <argparse/argparse.hpp>

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdCD : public KConsoleCommand
{
public:
    CCmdCD(KDebugConsole* console) : KConsoleCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        if (args.size() == 2)
        {
            const std::string& pathArg = args[1];
            PPath path;
            if (!pathArg.empty() && pathArg[0] == '/')
            {
                path.SetTo(pathArg);
            }
            else
            {
                const char* envPath = getenv("PATH");
                if (envPath != nullptr)
                {
                    path.SetTo(envPath);
                }
                else
                {
                    char cwd[PATH_MAX];
                    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                        path.SetTo(cwd);
                    }
                }
                path.Append(pathArg);
            }
            if (chdir(path.c_str()) == 0)
            {
                setenv("PATH", path.c_str(), true);
                return 0;
            }
        }
        return 1;
    }
    virtual PString GetDescription() const override { return "Change working directory."; }
};

static KConsoleCommandRegistrator<CCmdCD> g_RegisterCCmdCD("cd");

} // namespace kernel
