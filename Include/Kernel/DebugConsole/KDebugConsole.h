// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 09.01.2026 22:00

#pragma once

#include <Kernel/DebugConsole/KConsoleCommand.h>
#include <Kernel/KSemaphore.h>
#include <Kernel/KThread.h>
#include <System/TimeValue.h>
#include <Utils/TerminalLineEditor.h>

class PPOSIXTokenizer;

namespace kernel
{

class KDebugConsole : public KThread, public SignalTarget
{
public:
    struct JobEntry
    {
        pid_t              PID = -1;
        std::vector<pid_t> ProcessIDs;
        PString            CommandLine;
        bool               Stopped = false;
    };

    struct CommandExecutionResult
    {
        CommandExecutionResult() = default;
        explicit CommandExecutionResult(int exitCode) : ExitCode(exitCode) {}

        TimeValNanos UserTime;
        TimeValNanos SystemTime;
        int          ExitCode = 0;
    };

    struct CommandEntry
    {
        std::function<Ptr<KConsoleCommand>(KDebugConsole* console)> Creator;
        PString Description;
        bool    IsInternal = false;
    };

    static std::map<PString, CommandEntry>& GetCommands();
    static void RegisterCommand(const PString& name, const PString& description, bool isInternal, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>&& commandCreator);

    KDebugConsole(int ptyFD, bool allowTermination);

    void Setup();
    void Terminate(int exitCode);
    int  GetLastExitCode() const { return m_LastExitCode; }
    CommandExecutionResult ExecuteCommand(std::vector<std::string>&& arguments);

    virtual void* Run() override;

    int GetStdInFD() const  { return m_StdInFD; }
    int GetStdOutFD() const { return m_StdOutFD; }
    int GetStdErrFD() const { return m_StdErrFD; }

    void SendText(const char* text, size_t length);
    void SendText(const PString& text) { SendText(text.c_str(), text.size()); }
    void SendNewline() { SendText("\n", 1); }

    const std::map<int, JobEntry>& GetJobs() const { return m_Jobs; }

    int  AddJob(pid_t pid, bool isStopped, const PString& commandLine);
    int  AddJob(pid_t processGroupID, std::vector<pid_t>&& processIDs, bool isStopped, const PString& commandLine);
    void RemoveJob(int jobNum);
    int  FindJob(pid_t pid);
    const JobEntry& GetJobInfo(int jobNum) const;
    void SetJobStopped(int jobNum, bool stopped);

    CommandExecutionResult WaitForForegroundProcess(pid_t pid, const PString& commandLine);
    CommandExecutionResult WaitForForegroundProcesses(int jobNum);
    CommandExecutionResult WaitForForegroundProcesses(pid_t processGroupID, std::vector<pid_t> processIDs, PString commandLine);
    void CheckBackgroundJobs();

private:
    void HandleLineSubmitted(const PString& line);
    void HandleDisconnect();
    PTerminalLineEditor::CompletionResult GetExpansionList(const PTerminalLineEditor::CompletionContext& context);

    void UpdateCmdPrompt();

    PTerminalLineEditor::CompletionResult ExpandCommandName(const PTerminalLineEditor::CompletionContext& context);
    PTerminalLineEditor::CompletionResult ExpandFilePath(const PTerminalLineEditor::CompletionContext& context);

    CommandExecutionResult ExecuteCommand(std::vector<std::string>&& arguments, const PString& commandLine);
    void ProcessCmdLine(PPOSIXTokenizer&& tokenizer);
    CommandExecutionResult ExecutePipeline(std::vector<std::vector<std::string>>&& commands, const PString& commandLine);
#ifdef PADOS_MODULE_POSIX_SPAWN
    pid_t SpawnInternalPipelineCommand(const CommandEntry& commandEntry, std::vector<std::string>&& arguments, pid_t processGroupID, int inputFileDescriptor, int outputFileDescriptor, int unusedFileDescriptor);
#endif // PADOS_MODULE_POSIX_SPAWN

    PTerminalLineEditor m_LineEditor;

    int m_PTYFD = -1;
    int m_StdInFD = -1;
    int m_StdOutFD = -1;
    int m_StdErrFD = -1;

    std::map<int, JobEntry> m_Jobs;

    volatile bool m_ShouldRun = true;
    int           m_LastExitCode = 0;
    KSemaphore    m_TerminateSemaphore;
};

} // namespace kernel
