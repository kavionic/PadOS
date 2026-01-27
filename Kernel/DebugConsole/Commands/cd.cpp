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
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        if (args.size() == 2)
        {
            chdir(args[1].c_str());
            return 0;
        }
        return 1;
    }
    virtual PString GetDescription() const override { return "Change working directory."; }
};

static KConsoleCommandRegistrator<CCmdCD> g_RegisterCCmdCD("cd");

} // namespace kernel
