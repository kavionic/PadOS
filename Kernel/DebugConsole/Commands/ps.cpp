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
// Created: 10.01.2026 15:30

#include <argparse/argparse.hpp>

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
{

class CCmdPS : public KConsoleCommand
{
public:
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        argparse::ArgumentParser program(args[0], "1.0", argparse::default_arguments::none);

        program.add_argument("-h", "--help")
            .help("Print argument help.")
            .default_value(false)
            .implicit_value(true);

        program.add_argument("-t", "--thread")
            .help("Print detailed information about a thread.")
            .scan<'d', int>();

        try
        {
            program.parse_args(args);
        }
        catch (const std::exception& exc)
        {
            Print("{}", exc.what());
            Print("{}", program.help().str());
            return 1;
        }

        if (program.get<bool>("--help"))
        {
            Print("{}", program.help().str());
            return 0;
        }

        if (program.is_used("--thread"))
        {
            return PrintThreadInfo(program.get<int>("--thread"));
        }

        ThreadInfo threadInfo;

        for (PErrorCode result = kget_thread_info(INVALID_HANDLE, &threadInfo); result == PErrorCode::Success; result = kget_next_thread_info(&threadInfo))
        {
            if (threadInfo.ThreadID == 0) continue; // Ignore idle-thread.
            Print("{:4} {:9} {}:{}\n", threadInfo.ThreadID, GetStateName(threadInfo.State), threadInfo.ProcessName, threadInfo.ThreadName);
        }
        return 0;
    }

    virtual PString GetDescription() const override { return "Print information about running threads."; }

private:
    int PrintThreadInfo(thread_id threadID)
    {
        ThreadInfo threadInfo;
        const PErrorCode result = kget_thread_info(threadID, &threadInfo);

        if (result != PErrorCode::Success)
        {
            Print("Failed to get info for thread {}: {}\n", threadID, p_strerror(result));
            return 1;
        }
        Print("Name:       {}\n", threadInfo.ThreadName);
        Print("TID:        {}\n", threadID);
        Print("PID:        {}\n", threadInfo.ProcessID);
        Print("Priority:   {}\n", threadInfo.Priority);
        Print("State:      {}\n", GetStateName(threadInfo.State));
        Print("Blocking:   {}\n", threadInfo.BlockingObject);
        Print("Stack size: {}\n", threadInfo.StackSize);
        Print("Sys time:   {}\n", PString::format_time_period(TimeValNanos::FromNanoseconds(threadInfo.SysTimeNano), true));
        Print("User time:  {}\n", PString::format_time_period(TimeValNanos::FromNanoseconds(threadInfo.UserTimeNano), true));
        Print("Real time:  {}\n", PString::format_time_period(TimeValNanos::FromNanoseconds(threadInfo.RealTimeNano), true));

        return 0;
    }

    const char* GetStateName(ThreadState state)
    {
        switch(state)
        {
            case ThreadState_Running:   return "Running";
            case ThreadState_Ready:     return "Ready";
            case ThreadState_Sleeping:  return "Sleeping";
            case ThreadState_Waiting:   return "Waiting";
            case ThreadState_Stopped:   return "Stopped";
            case ThreadState_Zombie:    return "Zombie";
            case ThreadState_Deleted:   return "Deleted";

        }
        return "Unknown";
    }
};

static KConsoleCommandRegistrator<CCmdPS> g_RegisterCCmdPS("ps");

} // namespace kernel
