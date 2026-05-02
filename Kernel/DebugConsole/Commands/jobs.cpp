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
// Created: 07.04.2026 23:00

#ifdef PADOS_MODULE_POSIX_SIGNALS

#include <signal.h>

#include <Kernel/DebugConsole/KConsoleCommand.h>
#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/KPosixSignals.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// Shared helper: resolve a job specifier ("%N" or bare "N") from args[1].
/// Returns the matching job number, or -1 with an error printed.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int ResolveJobArg(KDebugConsole* console, KConsoleCommand* cmd, const std::vector<std::string>& args)
{
    const std::map<int, KDebugConsole::JobEntry>& jobs = console->GetJobs();

    if (jobs.empty())
    {
        cmd->Print("no current job\n");
        return -1;
    }

    // No argument: use the last job (current job, marked with '+').
    if (args.size() < 2)
    {
        return jobs.rbegin()->first;
    }

    const PString spec(args[1]);

    if (spec == "+" || spec == "%%" || spec == "%+") {
        return jobs.rbegin()->first;
    } else if (spec == "-" || spec == "%-") {
        return (jobs.size() > 1) ? (--jobs.rbegin())->first : -1;
    }

    // Strip optional leading '%' — both "2" and "%2" mean job number 2.
    const char* numStart = spec.c_str();
    if (*numStart == '%') ++numStart;

    bool isNumeric = *numStart != '\0';
    for (const char* p = numStart; *p; ++p)
    {
        if (*p < '0' || *p > '9')
        {
            isNumeric = false;
            break;
        }
    }

    if (isNumeric)
    {
        const int jobNum = std::stoi(numStart);
        if (jobs.find(jobNum) != jobs.end()) {
            return jobNum;
        }
        cmd->Print("{}: no such job\n", spec);
        return -1;
    }

    cmd->Print("usage: {} [+|%%|%+|-|%-|%N|N]\n", args[0]);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// jobs — list background and stopped jobs
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class CCmdJobs : public KConsoleCommand
{
public:
    CCmdJobs(KDebugConsole* console) : KConsoleCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        m_Console->CheckBackgroundJobs();

        const std::map<int, KDebugConsole::JobEntry>& jobs = m_Console->GetJobs();

        auto rit = jobs.rbegin();
        const int defaultJob = (rit != jobs.rend()) ? (rit++)->first : -1;
        const int nextDefJob = (rit != jobs.rend()) ? (rit++)->first : -1;

        for (auto it : jobs)
        {
            const KDebugConsole::JobEntry& job = it.second; // jobs[i];
            const char current = (it.first == defaultJob) ? '+' : ((it.first == nextDefJob) ? '-' : ' ');
            Print("[{}]{}  {}\t{}\n",
                it.first,
                current,
                job.Stopped ? "Stopped" : "Running",
                job.CommandLine);
        }
        return 0;
    }

    virtual PString GetDescription() const override
    {
        return "List background and stopped jobs.";
    }
};

///////////////////////////////////////////////////////////////////////////////
/// fg — bring a job to the foreground
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class CCmdFg : public KConsoleCommand
{
public:
    CCmdFg(KDebugConsole* console) : KConsoleCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        const int jobNum = ResolveJobArg(m_Console, this, args);
        if (jobNum < 0) {
            return 1;
        }

        // Snapshot the data we need before removing the entry.
        const KDebugConsole::JobEntry& job = m_Console->GetJobInfo(jobNum);

        pid_t   pid         = job.PID;
        PString commandLine = job.CommandLine;

        Print("{}\n", commandLine);

        kkill(pid, SIGCONT);
        m_Console->WaitForForegroundProcess(pid, commandLine);
        return 0;
    }

    virtual PString GetDescription() const override
    {
        return "Bring a job to the foreground (fg [N]).";
    }
};

///////////////////////////////////////////////////////////////////////////////
/// bg — resume a stopped job in the background
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class CCmdBg : public KConsoleCommand
{
public:
    CCmdBg(KDebugConsole* console) : KConsoleCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        const int jobNum = ResolveJobArg(m_Console, this, args);
        if (jobNum < 0) {
            return 1;
        }

        pid_t   pid         = -1;
        PString commandLine;
        bool    stopped     = false;

        const KDebugConsole::JobEntry& job = m_Console->GetJobInfo(jobNum);
        
        pid = job.PID;
        commandLine = job.CommandLine;
        stopped = job.Stopped;

        if (!stopped)
        {
            Print("bg: job {} already in background\n", jobNum);
            return 1;
        }

        m_Console->SetJobStopped(jobNum, false);
        Print("[{}]+  {} &\n", jobNum, commandLine);
        kkill(pid, SIGCONT);
        return 0;
    }

    virtual PString GetDescription() const override
    {
        return "Resume a stopped job in the background (bg [N]).";
    }
};

static KConsoleCommandRegistrator<CCmdJobs> g_RegisterCCmdJobs("jobs");
static KConsoleCommandRegistrator<CCmdFg>   g_RegisterCCmdFg("fg");
static KConsoleCommandRegistrator<CCmdBg>   g_RegisterCCmdBg("bg");

} // namespace kernel

#endif // PADOS_MODULE_POSIX_SIGNALS
