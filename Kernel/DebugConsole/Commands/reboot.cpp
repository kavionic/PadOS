// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 13.01.2026 23:30

#include <argparse/argparse.hpp>
#include <sys/pados_syscalls.h>

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdReboot : public KConsoleCommand
{
public:
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        Print("Rebooting...\n");
        __reboot(BootMode_Application);
        return 0;
    }
    static PString GetDescription() { return "Reboot device."; }
};

static KConsoleCommandRegistrator<CCmdReboot> g_RegisterCCmdReboot("reboot");

} // namespace kernel
