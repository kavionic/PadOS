// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

class CCmdJobs : public KConsoleInternalCommand
{
public:
    CCmdJobs(KDebugConsole* console) : KConsoleInternalCommand(console) {}

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

    static PString GetDescription()
    {
        return "List background and stopped jobs.";
    }
};

///////////////////////////////////////////////////////////////////////////////
/// fg — bring a job to the foreground
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class CCmdFg : public KConsoleInternalCommand
{
public:
    CCmdFg(KDebugConsole* console) : KConsoleInternalCommand(console) {}

    virtual int Invoke(std::vector<std::string>&& args) override
    {
        const int jobNum = ResolveJobArg(m_Console, this, args);
        if (jobNum < 0) {
            return 1;
        }

        const KDebugConsole::JobEntry& job = m_Console->GetJobInfo(jobNum);

        Print("{}\n", job.CommandLine);

        kkillpg_trw(job.PID, SIGCONT);
        m_Console->WaitForForegroundProcesses(jobNum);
        return 0;
    }

    static PString GetDescription()
    {
        return "Bring a job to the foreground (fg [N]).";
    }
};

///////////////////////////////////////////////////////////////////////////////
/// bg — resume a stopped job in the background
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class CCmdBg : public KConsoleInternalCommand
{
public:
    CCmdBg(KDebugConsole* console) : KConsoleInternalCommand(console) {}

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
        kkillpg_trw(pid, SIGCONT);
        return 0;
    }

    static PString GetDescription()
    {
        return "Resume a stopped job in the background (bg [N]).";
    }
};

static KConsoleCommandRegistrator<CCmdJobs> g_RegisterCCmdJobs("jobs");
static KConsoleCommandRegistrator<CCmdFg>   g_RegisterCCmdFg("fg");
static KConsoleCommandRegistrator<CCmdBg>   g_RegisterCCmdBg("bg");

} // namespace kernel

#endif // PADOS_MODULE_POSIX_SIGNALS
