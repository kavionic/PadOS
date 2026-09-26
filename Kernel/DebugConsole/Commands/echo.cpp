// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 13.01.2026 23:30

#include <argparse/argparse.hpp>

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdEcho : public KConsoleCommand
{
public:
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        for (size_t i = 1; i < args.size(); ++i)
        {
            const std::string& arg = args[i];
            if (i == 1) {
                Print("{}", arg);
            } else {
                Print(" {}", arg);
            }
        }
        Print("\n");
        return 0;
    }
    static PString GetDescription() { return "Echo all arguments."; }
};

static KConsoleCommandRegistrator<CCmdEcho> g_RegisterCCmdEcho("echo");

} // namespace kernel
