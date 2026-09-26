// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18.01.2026 22:30

#include <argparse/argparse.hpp>

#include <Kernel/DebugConsole/KConsoleCommand.h>
#include <Storage/Path.h>

namespace kernel
{

class CCmdCD : public KConsoleInternalCommand
{
public:
    CCmdCD(KDebugConsole* console) : KConsoleInternalCommand(console) {}

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
    static PString GetDescription() { return "Change working directory."; }
};

static KConsoleCommandRegistrator<CCmdCD> g_RegisterCCmdCD("cd");

} // namespace kernel
