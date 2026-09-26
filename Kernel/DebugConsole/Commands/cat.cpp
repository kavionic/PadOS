// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18.01.2026 22:30

#include <fcntl.h>
#include <unistd.h>

#include <argparse/argparse.hpp>

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdCat : public KConsoleCommand
{
public:
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        bool lastCharIsNewline = false;
        for (size_t i = 1; i < args.size(); ++i)
        {
            const std::string& arg = args[i];

            int file = open(arg.c_str(), O_RDONLY);

            if (file != -1)
            {
                for (;;)
                {
                    char buffer[256];
                    size_t length = read(file, buffer, sizeof(buffer));
                    if (length <= 0) {
                        break;
                    }
                    write(1, buffer, length);
                    lastCharIsNewline = buffer[length - 1] == '\n';
                }
                close(file);
            }
        }
        if (!lastCharIsNewline) {
            Print("\n");
        }
        return 0;
    }
    static PString GetDescription() { return "Concatenate FILE(s) to standard output."; }
};

static KConsoleCommandRegistrator<CCmdCat> g_RegisterCCmdCat("cat");

} // namespace kernel
