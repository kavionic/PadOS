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
// Created: 09.01.2026 22:00

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <termios.h>
#include <spawn.h>
#include <sys/wait.h>

#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/KLogging.h>
#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/KPosixSignals.h>
#include <Kernel/KProcess.h>
#include <Kernel/KProcessGroups.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/Kpty.h>
#include <System/AppDefinition.h>
#include <Utils/Logging.h>
#include <Utils/POSIXTokenizer.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::map<PString, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>>& KDebugConsole::GetCommands()
{
    static std::map<PString, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>> commands;
    return commands;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDebugConsole::KDebugConsole(int ptyFD, bool allowTermination)
    : KThread("debug_console")
    , m_PTYFD(ptyFD)
    , m_TerminateSemaphore("debug_console_terminate", CLOCK_MONOTONIC_COARSE, 0)
{
    m_LineEditor.SetSubmissionMode(PTerminalLineEditor::SubmissionMode::ContinueIncomplete);
    m_LineEditor.SetDisplayMode(PTerminalLineEditor::DisplayMode::Wrap);
    m_LineEditor.SetDisconnectOnEmpty(allowTermination);

    m_LineEditor.VFLineSubmitted.Connect(this, &KDebugConsole::HandleLineSubmitted);
    m_LineEditor.VFGetExpansionList.Connect(this, &KDebugConsole::GetExpansionList);
    m_LineEditor.VFDisconnect.Connect(this, &KDebugConsole::HandleDisconnect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::Setup()
{
    Start_trw(KSpawnThreadFlag::SpawnProcess, PThreadDetachState_Joinable);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::Terminate(int exitCode)
{
    m_LastExitCode = exitCode;
    m_ShouldRun = false;
    m_TerminateSemaphore.Release();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KDebugConsole::Run()
{
    ksetpgid_trw(0, 0);

    if (m_PTYFD != -1)
    {
        if (m_PTYFD != STDIN_FILENO) {
            dup2(m_PTYFD, STDIN_FILENO);
        }
        if (m_PTYFD != STDOUT_FILENO) {
            dup2(m_PTYFD, STDOUT_FILENO);
        }
        if (m_PTYFD != STDERR_FILENO) {
            dup2(m_PTYFD, STDERR_FILENO);
        }
        if (m_PTYFD != STDIN_FILENO && m_PTYFD != STDOUT_FILENO && m_PTYFD != STDERR_FILENO) {
            close(m_PTYFD);
        }

        m_StdInFD = STDIN_FILENO;
        m_StdOutFD = STDOUT_FILENO;
        m_StdErrFD = STDERR_FILENO;
    }

    UpdateCmdPrompt();
    m_LineEditor.Initialize(m_StdInFD, m_StdOutFD);
    m_LineEditor.BeginInput();

    KObjectWaitGroup waitGroup("debug_console_wait");
    waitGroup.AddFile_trw(m_StdInFD);
    waitGroup.AddObject_trw(&m_TerminateSemaphore);

    while (m_ShouldRun)
    {
        try
        {
            waitGroup.Wait_trw();

            if (m_TerminateSemaphore.TryAcquire() == PErrorCode::Success) {
                break;
            }

            const PTerminalLineEditor::ReadResult result = m_LineEditor.ReadInput();
            if (result == PTerminalLineEditor::ReadResult::Error) {
                continue;
            }
            if (result == PTerminalLineEditor::ReadResult::EndOfInput) {
                break;
            }
        }
        catch(std::exception& exc)
        {
            snooze_ms(100);
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::SendText(const char* text, size_t length)
{
    write(m_StdOutFD, text, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KDebugConsole::AddJob(pid_t pid, bool isStopped, const PString& commandLine)
{
    const int nextNum = m_Jobs.empty() ? 1 : m_Jobs.rbegin()->first + 1;
    m_Jobs[nextNum] = { pid, commandLine, isStopped };
    return nextNum;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::RemoveJob(int jobNum)
{
    auto it = m_Jobs.find(jobNum);
    if (it != m_Jobs.end()) {
        m_Jobs.erase(it);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KDebugConsole::FindJob(pid_t pid)
{
    for (const auto& job : m_Jobs) {
        if (job.second.PID == pid) return job.first;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const KDebugConsole::JobEntry& KDebugConsole::GetJobInfo(int jobNum) const
{
    auto it = m_Jobs.find(jobNum);
    if (it != m_Jobs.end()) {
        return it->second;
    }
    static JobEntry dummy;
    return dummy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::SetJobStopped(int jobNum, bool stopped)
{
    auto it = m_Jobs.find(jobNum);
    if (it != m_Jobs.end()) {
        it->second.Stopped = stopped;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::WaitForForegroundProcess(pid_t pid, const PString& commandLine)
{
    for (;;)
    {
        ktcsetpgrp_trw(STDOUT_FILENO, pid);

        siginfo_t info;
        const PErrorCode result = kwaitid(P_PID, pid, &info, WEXITED | WSTOPPED | WCONTINUED);

        if (result != PErrorCode::Success)
        {
            kprintf("kwaitpid() failed: %s(%d)\n", p_strerror(result), int(result));
            return;
        }

        switch (info.si_code)
        {
            case CLD_EXITED:
                m_LastExitCode = info.si_status;
                if (info.si_status != 0) {
                    kprintf("'%s' exited with code: %d\n", commandLine.c_str(), info.si_status);
                }
                // Remove from job list if it was a background job brought to foreground.
                RemoveJob(FindJob(pid));
                return;

            case CLD_KILLED:
                m_LastExitCode = 128 + info.si_status;
                kprintf("'%s' killed: %s(%d)\n", commandLine.c_str(), strsignal(info.si_status), info.si_status);
                RemoveJob(FindJob(pid));
                return;

            case CLD_STOPPED:
            {
                int jobNum = FindJob(pid);
                if (jobNum < 0) {
                    jobNum = AddJob(pid, true, commandLine);
                } else {
                    SetJobStopped(jobNum, true);
                }
                kprintf("\n[%d]+  Stopped\t%s\n", jobNum, commandLine.c_str());
                return;
            }

            case CLD_CONTINUED:
            {
                const int jobNum = FindJob(pid);
                if (jobNum >= 0) {
                    SetJobStopped(jobNum, false);
                }
                break; // Process continued — keep waiting.
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::CheckBackgroundJobs()
{
    for (auto it = m_Jobs.begin(); it != m_Jobs.end(); )
    {
        siginfo_t info = {};
        const PErrorCode result = kwaitid(P_PID, it->second.PID, &info, WNOHANG | WEXITED | WSTOPPED | WCONTINUED);

        if (result != PErrorCode::Success || info.si_pid == 0)
        {
            ++it;
            continue;
        }

        switch (info.si_code)
        {
            case CLD_EXITED:
                kprintf("[%d]  Done\t%s\n", it->first, it->second.CommandLine.c_str());
                it = m_Jobs.erase(it);
                continue;

            case CLD_KILLED:
                kprintf("[%d]  Killed\t%s\n", it->first, it->second.CommandLine.c_str());
                it = m_Jobs.erase(it);
                continue;

            case CLD_STOPPED:
                SetJobStopped(it->first, true);
                kprintf("[%d]+  Stopped\t%s\n", it->first, it->second.CommandLine.c_str());
                break;

            case CLD_CONTINUED:
                SetJobStopped(it->first, false);
                break;
        }
        ++it;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::HandleLineSubmitted(const PString& line)
{
    PPOSIXTokenizer tokenizer(line);
    ProcessCmdLine(std::move(tokenizer));
    UpdateCmdPrompt();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::HandleDisconnect()
{
    m_ShouldRun = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTerminalLineEditor::CompletionResult KDebugConsole::GetExpansionList(const PTerminalLineEditor::CompletionContext& context)
{
    if (context.TokenIndex == 0) {
        return ExpandCommandName(context);
    }
    return ExpandFilePath(context);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::UpdateCmdPrompt()
{
    CheckBackgroundJobs();

    char currentPath[PATH_MAX];

    const PString setColor = PANSIEscapeCodeParser::FormatANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::FgColor_Yellow));
    const PString resetColor = PANSIEscapeCodeParser::FormatANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::Reset));

    const char* environmentPath = getenv("PATH");
    bool pathIsValid = false;

    if (environmentPath != nullptr && strlen(environmentPath) <= PATH_MAX)
    {
        strcpy(currentPath, environmentPath);

        stat_t pathStat;
        stat_t currentDirectoryStat;
        pathIsValid = stat(environmentPath, &pathStat) == 0 && stat(".", &currentDirectoryStat) == 0 && pathStat.st_dev == currentDirectoryStat.st_dev && pathStat.st_ino == currentDirectoryStat.st_ino;
    }
    if (!pathIsValid) {
        pathIsValid = getcwd(currentPath, sizeof(currentPath)) != nullptr;
    }
    if (pathIsValid)
    {
        const PString prompt = PString::format_string("[{}{}{}]$ ", setColor, currentPath, resetColor);
        const size_t visibleLength = prompt.size() - setColor.size() - resetColor.size();
        m_LineEditor.SetPrimaryPrompt(prompt, visibleLength);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTerminalLineEditor::CompletionResult KDebugConsole::ExpandCommandName(const PTerminalLineEditor::CompletionContext& context)
{
    PTerminalLineEditor::CompletionResult result;

    for (const auto& command : GetCommands())
    {
        if (command.first.starts_with_nocase(context.TokenText.c_str())) {
            result.Candidates.push_back({command.first, true});
        }
    }

    const std::vector<const PAppDefinition*> applications = PAppDefinition::GetApplicationList();
    for (const PAppDefinition* application : applications)
    {
        PString entryName(application->Name);
        if (entryName.starts_with_nocase(context.TokenText.c_str())) {
            result.Candidates.push_back({std::move(entryName), true});
        }
    }

    result.ReplaceStartInToken = 0;
    result.ReplaceEndInToken = context.TokenText.size();

    std::sort(result.Candidates.begin(), result.Candidates.end(), [](const PTerminalLineEditor::CompletionCandidate& lhs, const PTerminalLineEditor::CompletionCandidate& rhs)
        {
            return lhs.Text < rhs.Text;
        }
    );
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTerminalLineEditor::CompletionResult KDebugConsole::ExpandFilePath(const PTerminalLineEditor::CompletionContext& context)
{
    PTerminalLineEditor::CompletionResult result;
    PString folderPath;
    PString filename;

    for (ssize_t characterIndex = ssize_t(context.TokenOffset) - 1; characterIndex >= 0; --characterIndex)
    {
        if (context.TokenText[size_t(characterIndex)] == '/')
        {
            result.ReplaceStartInToken = size_t(characterIndex) + 1;
            result.ReplaceEndInToken = context.TokenText.size();
            folderPath.insert(folderPath.begin(), context.TokenText.data(), context.TokenText.data() + characterIndex + 1);
            filename.insert(filename.begin(), context.TokenText.data() + characterIndex + 1, context.TokenText.data() + context.TokenText.size());

            folderPath += ".";
            break;
        }
    }
    if (folderPath.empty())
    {
        folderPath = ".";
        filename = context.TokenText;
        result.ReplaceStartInToken = 0;
        result.ReplaceEndInToken = context.TokenText.size();
    }

    const int directory = open(folderPath.c_str(), O_RDONLY | O_DIRECTORY);
    if (directory != -1)
    {
        dirent_t entry;
        while (kread_directory(directory, &entry, sizeof(entry)) == sizeof(entry))
        {
            PString entryName(entry.d_name, entry.d_namlen);
            if (entryName != "." && entryName != ".." && entryName.starts_with_nocase(filename.c_str()))
            {
                bool isDirectory = entry.d_type == DT_DIR;
                if (!isDirectory && entry.d_type == DT_LNK)
                {
                    struct stat targetStat;
                    isDirectory = fstatat(directory, entry.d_name, &targetStat, 0) == 0 && S_ISDIR(targetStat.st_mode);
                }
                if (isDirectory) {
                    entryName += "/";
                }
                result.Candidates.push_back({std::move(entryName), !isDirectory});
            }
        }
        close(directory);
    }

    std::sort(result.Candidates.begin(), result.Candidates.end(), [](const PTerminalLineEditor::CompletionCandidate& lhs, const PTerminalLineEditor::CompletionCandidate& rhs)
        {
            return lhs.Text < rhs.Text;
        }
    );
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::ProcessCmdLine(PPOSIXTokenizer&& tokenizer)
{
    std::vector<std::string> tokens;

    for (const PPOSIXTokenizer::Token& token : tokenizer.GetTokens()) {
        tokens.push_back(tokenizer.GetTokenText(token));
    }

    if (!tokens.empty())
    {
#ifdef PADOS_MODULE_POSIX_SPAWN
        const PString path = (tokens[0].empty() || tokens[0][0] == '/') ? tokens[0] : (PString("/bin/") + tokens[0]);

        stat_t statBuf;
        if (stat(path.c_str(), &statBuf) == 0 && (statBuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        {
            std::vector<char*> argv;
            argv.reserve(tokens.size());
            for (auto& token : tokens) {
                argv.push_back(token.data());
            }
            argv.push_back(nullptr);

            posix_spawnattr_t spawnAttrs;

            posix_spawnattr_init(&spawnAttrs);
            PScopeExit cleanup([&spawnAttrs] { posix_spawnattr_destroy(&spawnAttrs); });

            posix_spawnattr_setflags(&spawnAttrs, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF);

            sigset_t defSigs;
            sigfillset(&defSigs);

            posix_spawnattr_setsigdefault(&spawnAttrs, &defSigs);

            pid_t pid;
            const int spawnResult = posix_spawn(&pid, path.c_str(), nullptr, &spawnAttrs, argv.data(), environ);
            if (spawnResult != 0)
            {
                kprintf("Failed to execute '%s': %s\n", path.c_str(), strerror(spawnResult));
                return;
            }

            WaitForForegroundProcess(pid, tokenizer.GetText());
            return;
        }
#endif // PADOS_MODULE_POSIX_SPAWN

        auto cmdIt = GetCommands().find(tokens[0]);

        if (cmdIt != GetCommands().end())
        {
            const Ptr<KConsoleCommand>& cmd = cmdIt->second(this);

            try {
                m_LastExitCode = cmd->Invoke(std::move(tokens));
            } catch(const std::exception& exc) {
                kprintf("%s\n", exc.what());
            }
        }
        else
        {
            kprintf("Unknown command: %s\n", tokens[0].c_str());
        }
    }
}

} // namespace kernel
