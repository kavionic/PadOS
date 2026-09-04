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
// Created: 04.09.2026 00:00

#include <unistd.h>

#include <Kernel/DebugConsole/KConsoleCommand.h>
#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/KTime.h>

#ifdef PADOS_MODULE_GPROF_SAMPLING
#include <Kernel/KGProfSampler.h>
#include <System/ErrorCodes.h>
#endif // PADOS_MODULE_GPROF_SAMPLING

namespace kernel
{

static void TimeCommandPrintDuration(const char* label, TimeValNanos duration)
{
    const bigtime_t totalMilliseconds = duration.AsMilliseconds();
    const bigtime_t minutes = totalMilliseconds / 60000;
    const bigtime_t seconds = totalMilliseconds / 1000 % 60;
    const bigtime_t milliseconds = totalMilliseconds % 1000;
    const PString text = PString::format_string(
        "{}\t{}m{}.{:03}s\n",
        label,
        minutes,
        seconds,
        milliseconds);
    write(STDERR_FILENO, text.c_str(), text.size());
}

#ifdef PADOS_MODULE_GPROF_SAMPLING

class TimeCommandProfilerGuard
{
public:
    explicit TimeCommandProfilerGuard(bool armed) : m_Armed(armed) {}

    ~TimeCommandProfilerGuard()
    {
        if (m_Armed) {
            (void)kgprof_stop();
        }
    }

    void Disarm() { m_Armed = false; }

private:
    bool m_Armed;
};

#endif // PADOS_MODULE_GPROF_SAMPLING

class CCmdTime : public KConsoleInternalCommand
{
public:
    CCmdTime(KDebugConsole* console) : KConsoleInternalCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        size_t commandIndex = 1;
        bool profile = false;

        while (commandIndex < args.size())
        {
            const std::string& argument = args[commandIndex];
            if (argument == "-P" || argument == "--profile")
            {
                profile = true;
                ++commandIndex;
            }
            else if (argument == "--")
            {
                ++commandIndex;
                break;
            }
            else
            {
                break;
            }
        }

        if (commandIndex >= args.size())
        {
            Print("Usage: {} [-P|--profile] [--] command [argument ...]\n", args[0]);
            return 1;
        }

        args.erase(args.begin(), args.begin() + commandIndex);

#ifdef PADOS_MODULE_GPROF_SAMPLING
        if (profile)
        {
            const PErrorCode result = kgprof_start();
            if (result != PErrorCode::Success)
            {
                Print("Failed to start profiler: {}\n", p_strerror(result));
                return 1;
            }
        }
        TimeCommandProfilerGuard profilerGuard(profile);
#else
        if (profile)
        {
            Print("Profiling support is not available in this build.\n");
            return 1;
        }
#endif // PADOS_MODULE_GPROF_SAMPLING

        const TimeValNanos startTime = kget_monotonic_time_hires();
        const KDebugConsole::CommandExecutionResult executionResult = m_Console->ExecuteCommand(std::move(args));
        const TimeValNanos elapsedTime = kget_monotonic_time_hires() - startTime;

#ifdef PADOS_MODULE_GPROF_SAMPLING
        if (profile)
        {
            profilerGuard.Disarm();
            const PErrorCode result = kgprof_stop();
            if (result != PErrorCode::Success) {
                Print("Failed to stop profiler: {}\n", p_strerror(result));
            }
        }
#endif // PADOS_MODULE_GPROF_SAMPLING

        write(STDERR_FILENO, "\n", 1);
        TimeCommandPrintDuration("real", elapsedTime);
        TimeCommandPrintDuration("user", executionResult.UserTime);
        TimeCommandPrintDuration("sys", executionResult.SystemTime);
        return executionResult.ExitCode;
    }

    static PString GetDescription() { return "Time a command and optionally collect a GNU gprof profile."; }
};

static KConsoleCommandRegistrator<CCmdTime> g_RegisterCCmdTime("time");

} // namespace kernel
