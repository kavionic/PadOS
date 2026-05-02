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
// Created: 28.03.2026 23:00

#include <argparse/argparse.hpp>
#include <signal.h>

#include <System/ErrorCodes.h>
#include <Utils/String.h>
#include <Kernel/DebugConsole/KConsoleCommand.h>
#include <Kernel/KPosixSignals.h>

namespace kernel
{

class CCmdKill : public KConsoleCommand
{
public:
    CCmdKill(KDebugConsole* console) : KConsoleCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        argparse::ArgumentParser program(args[0], "1.0", argparse::default_arguments::none);

        std::vector<PString> knownFlags;
        auto registerArg = [&knownFlags](argparse::Argument& arg) -> argparse::Argument&
        {
            const std::string csv = arg.get_names_csv('\x01');
            for (size_t pos = 0; pos < csv.size(); )
            {
                const size_t end = csv.find('\x01', pos);
                const size_t len = (end == std::string::npos ? csv.size() : end) - pos;
                if (len > 0 && csv[pos] == '-') {
                    knownFlags.push_back(csv.substr(pos, len));
                }
                pos += len + 1;
            }
            return arg;
        };

        registerArg(program.add_argument("-h", "--help")
            .help("Print argument help.")
            .default_value(false)
            .implicit_value(true));

        registerArg(program.add_argument("-l", "--list")
            .help("List signal names and numbers.")
            .default_value(false)
            .implicit_value(true));

        registerArg(program.add_argument("-s", "-n", "--signal")
            .help("Signal name or number to send (default: TERM).")
            .default_value(std::string("TERM")));

        program.add_argument("pid")
            .help("Process ID(s) to signal.")
            .remaining()
            .scan<'d', int>();

        const PString inlineSig = ExtractInlineSignal(args, knownFlags);

        try {
            program.parse_args(args);
        }
        catch (const std::exception& exc)
        {
            Print("{}\n", exc.what());
            Print("{}", program.help().str());
            return 1;
        }

        if (program.get<bool>("--help"))
        {
            Print("{}", program.help().str());
            return 0;
        }

        if (program.get<bool>("--list"))
        {
            for (const auto& entry : s_SignalTable) {
                Print("{:2}  SIG{}\n", entry.Number, entry.Name);
            }
            return 0;
        }

        const PString sigStr = inlineSig.empty() ? PString(program.get<std::string>("--signal")) : inlineSig;
        const int sigNum = ParseSignal(sigStr);
        if (sigNum < 0)
        {
            Print("kill: unknown signal '{}'\n", sigStr);
            return 1;
        }

        std::vector<int> pids;
        try {
            pids = program.get<std::vector<int>>("pid");
        }
        catch (const std::exception&) {}

        if (pids.empty())
        {
            Print("kill: missing PID\n");
            Print("{}", program.help().str());
            return 1;
        }

        int exitCode = 0;
        for (const int pid : pids)
        {
            const PErrorCode result = kkill(static_cast<pid_t>(pid), sigNum);
            if (result != PErrorCode::Success)
            {
                Print("kill: ({}): {}\n", pid, p_strerror(result));
                exitCode = 1;
            }
        }
        return exitCode;
    }

    virtual PString GetDescription() const override
    {
        return "Send a signal to a process.";
    }

private:
    // Extract and remove a -<signal> style argument (e.g. -9, -KILL, -SIGKILL) from args.
    // Returns the signal specifier (without the leading '-'), or an empty string if none found.
    PString ExtractInlineSignal(std::vector<std::string>& args, const std::vector<PString>& knownFlags)
    {
        for (size_t i = 1; i < args.size(); ++i)
        {
            const PString arg = args[i];

            if (arg.size() < 2 || arg[0] != '-') {
                continue; // Not a flag.
            }
            if (arg[1] == '-') {
                continue; // Long flag, skip.
            }
            // Skip known flags.
            const bool isKnown = std::find_if(knownFlags.begin(), knownFlags.end(),
                [&arg](const PString& flag) { return arg.compare_nocase(flag) == 0; }) != knownFlags.end();

            if (isKnown) continue;

            // Check if the part after '-' is a valid signal spec.
            const PString spec(arg.c_str() + 1);
            if (ParseSignal(spec) >= 0)
            {
                args.erase(args.begin() + i);
                return spec;
            }
        }
        return PString();
    }

    int ParseSignal(const PString& str)
    {
        // Try numeric first.
        bool isNumeric = !str.empty();
        for (char c : str)
        {
            if (c < '0' || c > '9')
            {
                isNumeric = false;
                break;
            }
        }
        if (isNumeric) {
            return std::stoi(str);
        }

        // Strip leading "SIG" prefix (case-insensitive).
        const PString name = str.starts_with_nocase("SIG") ? PString(str.c_str() + 3) : str;

        for (const auto& entry : s_SignalTable)
        {
            if (name.compare_nocase(entry.Name) == 0) {
                return entry.Number;
            }
        }
        return -1;
    }

    struct SignalEntry { const char* Name; int Number; };

    static constexpr SignalEntry s_SignalTable[] = {
        { "HUP",   SIGHUP   },
        { "INT",   SIGINT   },
        { "QUIT",  SIGQUIT  },
        { "ILL",   SIGILL   },
        { "TRAP",  SIGTRAP  },
        { "ABRT",  SIGABRT  },
        { "EMT",   SIGEMT   },
        { "FPE",   SIGFPE   },
        { "KILL",  SIGKILL  },
        { "BUS",   SIGBUS   },
        { "SEGV",  SIGSEGV  },
        { "SYS",   SIGSYS   },
        { "PIPE",  SIGPIPE  },
        { "ALRM",  SIGALRM  },
        { "TERM",  SIGTERM  },
        { "URG",   SIGURG   },
        { "STOP",  SIGSTOP  },
        { "TSTP",  SIGTSTP  },
        { "CONT",  SIGCONT  },
        { "CHLD",  SIGCHLD  },
        { "TTIN",  SIGTTIN  },
        { "TTOU",  SIGTTOU  },
        { "IO",    SIGIO    },
        { "WINCH", SIGWINCH },
        { "PWR",   SIGPWR   },
        { "USR1",  SIGUSR1  },
        { "USR2",  SIGUSR2  },
    };
};

static KConsoleCommandRegistrator<CCmdKill> g_RegisterCCmdKill("kill");

} // namespace kernel
