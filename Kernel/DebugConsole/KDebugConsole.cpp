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
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include <termios.h>
#include <spawn.h>
#include <sys/wait.h>

#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/KLogging.h>
#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/KPosixSignals.h>
#include <Kernel/KPosixSpawn.h>
#include <Kernel/KProcess.h>
#include <Kernel/KProcessGroups.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/Kpty.h>
#include <System/AppDefinition.h>
#include <Utils/Logging.h>
#include <Utils/POSIXTokenizer.h>


namespace kernel
{

static bool IsKDebugConsolePipeToken(const PPOSIXTokenizer& tokenizer, const PPOSIXTokenizer::Token& token)
{
    return !token.HasFormatting && token.End == token.Start + 1 && tokenizer.GetText()[token.Start] == '|';
}

#ifdef PADOS_MODULE_POSIX_SPAWN

struct KDebugConsolePipelineCommandContext
{
    Ptr<KConsoleCommand>     Command;
    std::vector<std::string> Arguments;
    int                      InputFileDescriptor = -1;
    int                      OutputFileDescriptor = -1;
    int                      UnusedFileDescriptor = -1;
};

static void* KDebugConsolePipelineCommandEntry(void* argument)
{
    std::unique_ptr<KDebugConsolePipelineCommandContext> context(static_cast<KDebugConsolePipelineCommandContext*>(argument));

    bool setupSucceeded = true;
    if (context->InputFileDescriptor != -1) {
        setupSucceeded = dup2(context->InputFileDescriptor, STDIN_FILENO) != -1;
    }
    if (setupSucceeded && context->OutputFileDescriptor != -1) {
        setupSucceeded = dup2(context->OutputFileDescriptor, STDOUT_FILENO) != -1;
    }

    if (context->InputFileDescriptor != -1) {
        close(context->InputFileDescriptor);
    }
    if (context->OutputFileDescriptor != -1) {
        close(context->OutputFileDescriptor);
    }
    if (context->UnusedFileDescriptor != -1) {
        close(context->UnusedFileDescriptor);
    }

    int exitCode = 1;
    if (setupSucceeded)
    {
        try {
            exitCode = context->Command->Invoke(std::move(context->Arguments));
        } catch (const std::exception& exc) {
            write(STDERR_FILENO, exc.what(), strlen(exc.what()));
            write(STDERR_FILENO, "\n", 1);
        }
    }
    return reinterpret_cast<void*>(intptr_t(exitCode));
}

#endif // PADOS_MODULE_POSIX_SPAWN

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::map<PString, KDebugConsole::CommandEntry>& KDebugConsole::GetCommands()
{
    static std::map<PString, CommandEntry> commands;
    return commands;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::RegisterCommand(const PString& name, const PString& description, bool isInternal, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>&& commandCreator)
{
    GetCommands()[name] = {std::move(commandCreator), description, isInternal};
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
    return AddJob(pid, std::vector<pid_t>{pid}, isStopped, commandLine);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KDebugConsole::AddJob(pid_t processGroupID, std::vector<pid_t>&& processIDs, bool isStopped, const PString& commandLine)
{
    const int nextNum = m_Jobs.empty() ? 1 : m_Jobs.rbegin()->first + 1;
    m_Jobs[nextNum] = { processGroupID, std::move(processIDs), commandLine, isStopped };
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
    WaitForForegroundProcesses(pid, std::vector<pid_t>{pid}, commandLine);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::WaitForForegroundProcesses(int jobNum)
{
    const auto jobIterator = m_Jobs.find(jobNum);
    if (jobIterator == m_Jobs.end()) {
        return;
    }

    const JobEntry& job = jobIterator->second;
    WaitForForegroundProcesses(job.PID, job.ProcessIDs, job.CommandLine);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::WaitForForegroundProcesses(pid_t processGroupID, std::vector<pid_t> processIDs, PString commandLine)
{
    if (processIDs.empty()) {
        return;
    }

    const pid_t lastProcessID = processIDs.back();

    ktcsetpgrp_trw(STDOUT_FILENO, processGroupID);

    while (!processIDs.empty())
    {
        const pid_t processID = processIDs.front();

        siginfo_t info;
        const PErrorCode result = kwaitid(P_PID, processID, &info, WEXITED | WSTOPPED | WCONTINUED);

        if (result != PErrorCode::Success)
        {
            kprintf("kwaitpid() failed: %s(%d)\n", p_strerror(result), int(result));
            return;
        }

        switch (info.si_code)
        {
            case CLD_EXITED:
                if (processID == lastProcessID)
                {
                    m_LastExitCode = info.si_status;
                    if (info.si_status != 0) {
                        kprintf("'%s' exited with code: %d\n", commandLine.c_str(), info.si_status);
                    }
                }
                processIDs.erase(processIDs.begin());
                break;

            case CLD_DUMPED:
            case CLD_KILLED:
                if (processID == lastProcessID)
                {
                    m_LastExitCode = 128 + info.si_status;
                    kprintf("'%s' killed: %s(%d)\n", commandLine.c_str(), strsignal(info.si_status), info.si_status);
                }
                processIDs.erase(processIDs.begin());
                break;

            case CLD_STOPPED:
            {
                int jobNum = FindJob(processGroupID);
                if (jobNum < 0)
                {
                    jobNum = AddJob(processGroupID, std::move(processIDs), true, commandLine);
                }
                else
                {
                    m_Jobs[jobNum].ProcessIDs = std::move(processIDs);
                    SetJobStopped(jobNum, true);
                }
                kprintf("\n[%d]+  Stopped\t%s\n", jobNum, commandLine.c_str());
                return;
            }

            case CLD_CONTINUED:
            {
                const int jobNum = FindJob(processGroupID);
                if (jobNum >= 0) {
                    SetJobStopped(jobNum, false);
                }
                break;
            }
        }
    }

    RemoveJob(FindJob(processGroupID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::CheckBackgroundJobs()
{
    for (auto jobIterator = m_Jobs.begin(); jobIterator != m_Jobs.end(); )
    {
        JobEntry& job = jobIterator->second;
        bool wasKilled = false;
        bool isStopped = job.Stopped;

        for (auto processIterator = job.ProcessIDs.begin(); processIterator != job.ProcessIDs.end(); )
        {
            siginfo_t info = {};
            const PErrorCode result = kwaitid(P_PID, *processIterator, &info, WNOHANG | WEXITED | WSTOPPED | WCONTINUED);

            if (result != PErrorCode::Success || info.si_pid == 0)
            {
                ++processIterator;
                continue;
            }

            switch (info.si_code)
            {
                case CLD_EXITED:
                    processIterator = job.ProcessIDs.erase(processIterator);
                    continue;

                case CLD_DUMPED:
                case CLD_KILLED:
                    wasKilled = true;
                    processIterator = job.ProcessIDs.erase(processIterator);
                    continue;

                case CLD_STOPPED:
                    isStopped = true;
                    break;

                case CLD_CONTINUED:
                    isStopped = false;
                    break;
            }
            ++processIterator;
        }

        if (job.ProcessIDs.empty())
        {
            kprintf("[%d]  %s\t%s\n", jobIterator->first, wasKilled ? "Killed" : "Done", job.CommandLine.c_str());
            jobIterator = m_Jobs.erase(jobIterator);
            continue;
        }

        if (!job.Stopped && isStopped) {
            kprintf("[%d]+  Stopped\t%s\n", jobIterator->first, job.CommandLine.c_str());
        }
        job.Stopped = isStopped;
        ++jobIterator;
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
    size_t commandTokenIndex = 0;

    for (size_t tokenIndex = 0; tokenIndex < context.TokenIndex; ++tokenIndex)
    {
        if (IsKDebugConsolePipeToken(context.Tokenizer, context.Tokenizer.GetTokens()[tokenIndex])) {
            commandTokenIndex = tokenIndex + 1;
        }
    }

    if (context.TokenIndex == commandTokenIndex) {
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
    std::vector<std::vector<std::string>> commands(1);

    for (const PPOSIXTokenizer::Token& token : tokenizer.GetTokens())
    {
        if (IsKDebugConsolePipeToken(tokenizer, token))
        {
            if (commands.back().empty())
            {
                kprintf("syntax error near unexpected token '|'\n");
                return;
            }
            commands.emplace_back();
        }
        else
        {
            commands.back().push_back(tokenizer.GetTokenText(token));
        }
    }

    if (commands.back().empty())
    {
        if (commands.size() > 1) {
            kprintf("syntax error near unexpected token '|'\n");
        }
        return;
    }

    if (commands.size() > 1)
    {
        ExecutePipeline(std::move(commands), tokenizer.GetText());
        return;
    }

    std::vector<std::string>& tokens = commands.front();

#ifdef PADOS_MODULE_POSIX_SPAWN
    const PString path = (tokens[0].empty() || tokens[0][0] == '/') ? tokens[0] : (PString("/bin/") + tokens[0]);

    stat_t statBuf;
    if (stat(path.c_str(), &statBuf) == 0 && (statBuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
    {
        ExecutePipeline(std::move(commands), tokenizer.GetText());
        return;
    }
#endif // PADOS_MODULE_POSIX_SPAWN

    auto cmdIt = GetCommands().find(tokens[0]);

    if (cmdIt != GetCommands().end())
    {
        const Ptr<KConsoleCommand>& cmd = cmdIt->second.Creator(this);

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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::ExecutePipeline(std::vector<std::vector<std::string>>&& commands, const PString& commandLine)
{
#ifdef PADOS_MODULE_POSIX_SPAWN
    std::vector<PString> paths(commands.size());
    std::vector<const CommandEntry*> internalCommandEntries(commands.size(), nullptr);

    for (size_t commandIndex = 0; commandIndex < commands.size(); ++commandIndex)
    {
        const std::vector<std::string>& command = commands[commandIndex];
        const PString path = (command[0].empty() || command[0][0] == '/') ? command[0] : (PString("/bin/") + command[0]);

        stat_t statBuf;
        if (stat(path.c_str(), &statBuf) == 0 && (statBuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        {
            paths[commandIndex] = path;
            continue;
        }

        const auto commandIterator = GetCommands().find(command[0]);
        if (commandIterator == GetCommands().end())
        {
            kprintf("Unknown command: %s\n", command[0].c_str());
            return;
        }
        if (commandIterator->second.IsInternal)
        {
            kprintf("Command '%s' must run in the console process and cannot be used in a pipeline\n", command[0].c_str());
            return;
        }

        paths[commandIndex] = command[0];
        internalCommandEntries[commandIndex] = &commandIterator->second;
    }

    std::vector<pid_t> processIDs;
    processIDs.reserve(commands.size());

    pid_t processGroupID = -1;
    int inputFileDescriptor = -1;

    auto terminatePipeline = [&]()
        {
            if (inputFileDescriptor != -1) {
                close(inputFileDescriptor);
            }
            if (processGroupID != -1) {
                kkillpg(processGroupID, SIGKILL);
            }
            for (pid_t processID : processIDs)
            {
                siginfo_t info = {};
                kwaitid(P_PID, processID, &info, WEXITED);
            }
        };

    for (size_t commandIndex = 0; commandIndex < commands.size(); ++commandIndex)
    {
        const bool hasOutputPipe = commandIndex + 1 < commands.size();
        int outputPipe[2] = {-1, -1};

        if (hasOutputPipe && pipe(outputPipe) != 0)
        {
            kprintf("Failed to create pipe: %s\n", strerror(errno));
            terminatePipeline();
            return;
        }

        pid_t processID = -1;
        int spawnResult = 0;
        PString spawnError;

        if (internalCommandEntries[commandIndex] != nullptr)
        {
            try
            {
                processID = SpawnInternalPipelineCommand(
                    *internalCommandEntries[commandIndex],
                    std::move(commands[commandIndex]),
                    processGroupID,
                    inputFileDescriptor,
                    hasOutputPipe ? outputPipe[1] : -1,
                    hasOutputPipe ? outputPipe[0] : -1);
            }
            catch (const std::exception& exc)
            {
                spawnResult = -1;
                spawnError = exc.what();
            }
        }
        else
        {
            posix_spawn_file_actions_t fileActions;
            int actionResult = posix_spawn_file_actions_init(&fileActions);
            if (actionResult == 0 && inputFileDescriptor != -1) {
                actionResult = posix_spawn_file_actions_adddup2(&fileActions, inputFileDescriptor, STDIN_FILENO);
            }
            if (actionResult == 0 && inputFileDescriptor != -1) {
                actionResult = posix_spawn_file_actions_addclose(&fileActions, inputFileDescriptor);
            }
            if (actionResult == 0 && hasOutputPipe) {
                actionResult = posix_spawn_file_actions_adddup2(&fileActions, outputPipe[1], STDOUT_FILENO);
            }
            if (actionResult == 0 && hasOutputPipe) {
                actionResult = posix_spawn_file_actions_addclose(&fileActions, outputPipe[0]);
            }
            if (actionResult == 0 && hasOutputPipe) {
                actionResult = posix_spawn_file_actions_addclose(&fileActions, outputPipe[1]);
            }

            if (actionResult != 0)
            {
                if (hasOutputPipe)
                {
                    close(outputPipe[0]);
                    close(outputPipe[1]);
                }
                kprintf("Failed to configure pipe: %s\n", strerror(actionResult));
                terminatePipeline();
                return;
            }

            PScopeExit destroyFileActions([&fileActions]
                {
                    posix_spawn_file_actions_destroy(&fileActions);
                }
            );

            posix_spawnattr_t spawnAttrs;
            posix_spawnattr_init(&spawnAttrs);
            PScopeExit destroySpawnAttrs([&spawnAttrs]
                {
                    posix_spawnattr_destroy(&spawnAttrs);
                }
            );

            posix_spawnattr_setflags(&spawnAttrs, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF);
            if (processGroupID != -1) {
                posix_spawnattr_setpgroup(&spawnAttrs, processGroupID);
            }

            sigset_t defaultSignals;
            sigfillset(&defaultSignals);
            posix_spawnattr_setsigdefault(&spawnAttrs, &defaultSignals);

            std::vector<char*> arguments;
            arguments.reserve(commands[commandIndex].size() + 1);
            for (std::string& argument : commands[commandIndex]) {
                arguments.push_back(argument.data());
            }
            arguments.push_back(nullptr);

            spawnResult = posix_spawn(&processID, paths[commandIndex].c_str(), &fileActions, &spawnAttrs, arguments.data(), environ);
        }

        if (inputFileDescriptor != -1) {
            close(inputFileDescriptor);
        }
        if (hasOutputPipe) {
            close(outputPipe[1]);
        }
        inputFileDescriptor = hasOutputPipe ? outputPipe[0] : -1;

        if (spawnResult != 0)
        {
            const char* errorMessage = spawnError.empty() ? strerror(spawnResult) : spawnError.c_str();
            kprintf("Failed to execute '%s': %s\n", paths[commandIndex].c_str(), errorMessage);
            terminatePipeline();
            return;
        }

        if (processGroupID == -1) {
            processGroupID = processID;
        }
        processIDs.push_back(processID);
    }

    WaitForForegroundProcesses(processGroupID, std::move(processIDs), commandLine);
#else // PADOS_MODULE_POSIX_SPAWN
    (void)commands;
    (void)commandLine;
    kprintf("Pipelines require POSIX spawn support\n");
#endif // PADOS_MODULE_POSIX_SPAWN
}

#ifdef PADOS_MODULE_POSIX_SPAWN

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

pid_t KDebugConsole::SpawnInternalPipelineCommand(const CommandEntry& commandEntry, std::vector<std::string>&& arguments, pid_t processGroupID, int inputFileDescriptor, int outputFileDescriptor, int unusedFileDescriptor)
{
    std::unique_ptr<KDebugConsolePipelineCommandContext> context = std::make_unique<KDebugConsolePipelineCommandContext>();
    context->Command = commandEntry.Creator(this);
    context->Arguments = std::move(arguments);
    context->InputFileDescriptor = inputFileDescriptor;
    context->OutputFileDescriptor = outputFileDescriptor;
    context->UnusedFileDescriptor = unusedFileDescriptor;

    PPosixSpawnAttribs spawnAttrs = {};
    spawnAttrs.sa_flags = POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF;
    spawnAttrs.sa_pgroup = (processGroupID != -1) ? processGroupID : 0;
    sigfillset(&spawnAttrs.sa_sigdefault);

    PThreadAttribs threadAttrs(context->Arguments[0].c_str(), 0, PThreadDetachState_Joinable, 0);
    const pid_t processID = kthread_spawn_trw(
        &threadAttrs,
        &spawnAttrs,
#ifdef PADOS_MODULE_USER_SPACE
        nullptr,
#endif // PADOS_MODULE_USER_SPACE
        { KSpawnThreadFlag::Privileged, KSpawnThreadFlag::SpawnProcess },
        nullptr,
        KDebugConsolePipelineCommandEntry,
        context.get());

    context.release();
    return processID;
}

#endif // PADOS_MODULE_POSIX_SPAWN

} // namespace kernel
