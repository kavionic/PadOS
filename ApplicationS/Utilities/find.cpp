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
// Created: 12.08.2026 22:00

#include "find.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <limits>
#include <spawn.h>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <vector>

#include <dirent.h>
#include <sys/pados_types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <argparse/argparse.hpp>

#include <Storage/DirectoryEntry.h>
#include <Storage/Path.h>
#include <System/AppDefinition.h>
#include <Utils/String.h>


namespace shutil_find
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int CmdFind::Run(int argc, char* argv[])
{
    m_CommandName = argv[0];

    const ParseArgumentsResult parseResult = ParseArguments(argc, argv);

    if (parseResult == ParseArgumentsResult::Help) {
        return 0;
    }
    if (parseResult == ParseArgumentsResult::Error) {
        return 1;
    }

    if (m_StartPaths.empty()) {
        m_StartPaths.emplace_back(".");
    }

    for (const PString& startPath : m_StartPaths)
    {
        stat_t statBuffer;

        if (ReadNodeStat(startPath, statBuffer))
        {
            VisitPath(
                startPath,
                GetBaseName(startPath),
                0,
                statBuffer,
                statBuffer.st_dev);
        }
    }

    FlushPendingActions();
    return m_HadError ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ParseArgumentsResult CmdFind::ParseArguments(int argc, char* argv[])
{
    std::vector<std::string> parserArguments;

    if (!ExtractActions(argc, argv, parserArguments)) {
        return ParseArgumentsResult::Error;
    }

    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Search for files in a directory hierarchy.");

    program.add_argument("--help")
        .help("Print argument help.")
        .flag();

    program.add_argument("-name")
        .help("Match the base name against PATTERN.")
        .metavar("PATTERN");

    program.add_argument("-type")
        .help("Match files of type b, c, d, f, l, p, or s.")
        .metavar("TYPE");

    program.add_argument("-mindepth")
        .help("Do not match entries at depths less than LEVEL.")
        .metavar("LEVEL")
        .scan<'u', size_t>();

    program.add_argument("-maxdepth")
        .help("Do not descend below LEVEL.")
        .metavar("LEVEL")
        .scan<'u', size_t>();

    program.add_argument("-print")
        .help("Print matching paths; this is the default action.")
        .flag();

    program.add_argument("-print0")
        .help("Print matching paths followed by a null byte.")
        .flag();

    program.add_argument("-exec")
        .help("Run COMMAND for matching paths; terminate with ; or +.")
        .metavar("COMMAND... ;|+");

    program.add_argument("-execdir")
        .help("Run COMMAND from each matching path's directory.")
        .metavar("COMMAND... ;|+");

    program.add_argument("-xdev")
        .help("Do not descend into directories on other devices.")
        .flag();

    program.add_argument("paths")
        .help("Starting paths; defaults to the current directory.")
        .metavar("PATH")
        .nargs(argparse::nargs_pattern::any);

    try
    {
        program.parse_args(parserArguments);
    }
    catch (const std::exception& exception)
    {
        WriteAll(
            STDERR_FILENO,
            PString::format_string("{}\n", exception.what()));
        WriteAll(STDERR_FILENO, program.help().str());
        return ParseArgumentsResult::Error;
    }

    if (program.get<bool>("--help"))
    {
        WriteAll(STDOUT_FILENO, program.help().str());
        return ParseArgumentsResult::Help;
    }

    if (program.is_used("-name"))
    {
        m_NamePattern = program.get("-name");
        m_UseNamePattern = true;
    }

    if (program.is_used("-type"))
    {
        const std::string& fileType = program.get("-type");

        if (fileType.size() != 1 ||
            std::string_view("bcdflps").find(fileType[0]) ==
                std::string_view::npos)
        {
            WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: invalid file type '{}'; expected b, c, d, f, l, p, or s\n",
                    argv[0],
                    fileType));
            return ParseArgumentsResult::Error;
        }

        m_FileType = fileType[0];
        m_UseFileType = true;
    }

    if (program.is_used("-mindepth")) {
        m_MinDepth = program.get<size_t>("-mindepth");
    }
    if (program.is_used("-maxdepth")) {
        m_MaxDepth = program.get<size_t>("-maxdepth");
    }
    m_StayOnDevice = program.get<bool>("-xdev");

    const std::vector<std::string> paths =
        program.get<std::vector<std::string>>("paths");

    m_StartPaths.reserve(paths.size());

    for (const std::string& path : paths) {
        m_StartPaths.emplace_back(path);
    }

    if (m_Actions.empty()) {
        m_Actions.emplace_back();
    }

    return ParseArgumentsResult::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::ExtractActions(
    int argc,
    char* argv[],
    std::vector<std::string>& parserArguments)
{
    parserArguments.reserve(size_t(argc));
    parserArguments.emplace_back(argv[0]);

    bool parseOptions = true;

    for (size_t argumentIndex = 1;
         argumentIndex < size_t(argc);
         ++argumentIndex)
    {
        const std::string_view argument = argv[argumentIndex];

        if (parseOptions && argument == "--")
        {
            parseOptions = false;
            parserArguments.emplace_back(argument);
            continue;
        }

        if (parseOptions && (argument == "-print" || argument == "-print0"))
        {
            FindAction action;
            action.Type = (argument == "-print")
                ? FindActionType::Print
                : FindActionType::PrintNull;
            m_Actions.push_back(std::move(action));
            continue;
        }

        if (parseOptions && (argument == "-exec" || argument == "-execdir"))
        {
            const bool executeInDirectory = argument == "-execdir";
            const size_t commandStart = argumentIndex + 1;
            size_t terminatorIndex = commandStart;
            bool batchCommand = false;

            for (; terminatorIndex < size_t(argc); ++terminatorIndex)
            {
                const std::string_view commandArgument = argv[terminatorIndex];

                if (commandArgument == ";") {
                    break;
                }
                if (commandArgument == "+" &&
                    terminatorIndex > commandStart &&
                    std::string_view(argv[terminatorIndex - 1]) == "{}")
                {
                    batchCommand = true;
                    break;
                }
            }

            if (terminatorIndex == size_t(argc))
            {
                ReportError(PString::format_string(
                    "{}: missing terminator for {}\n",
                    m_CommandName,
                    argument));
                return false;
            }
            if (terminatorIndex == commandStart)
            {
                ReportError(PString::format_string(
                    "{}: missing command for {}\n",
                    m_CommandName,
                    argument));
                return false;
            }

            FindAction action;
            action.Type = executeInDirectory
                ? (batchCommand
                    ? FindActionType::ExecuteDirectoryBatch
                    : FindActionType::ExecuteDirectory)
                : (batchCommand
                    ? FindActionType::ExecuteBatch
                    : FindActionType::Execute);

            action.CommandArguments.reserve(terminatorIndex - commandStart);

            for (size_t commandIndex = commandStart;
                 commandIndex < terminatorIndex;
                 ++commandIndex) {
                action.CommandArguments.emplace_back(argv[commandIndex]);
            }

            if (batchCommand)
            {
                action.CommandArguments.pop_back();

                if (action.CommandArguments.empty())
                {
                    ReportError(PString::format_string(
                        "{}: missing command before '{{}} +'\n",
                        m_CommandName));
                    return false;
                }

                for (const PString& commandArgument : action.CommandArguments)
                {
                    if (commandArgument.find("{}") != PString::npos)
                    {
                        ReportError(PString::format_string(
                            "{}: '{{}}' must appear exactly once at the end of a + command\n",
                            m_CommandName));
                        return false;
                    }
                }
            }

            if (action.CommandArguments[0].empty())
            {
                ReportError(PString::format_string(
                    "{}: command name cannot be empty\n",
                    m_CommandName));
                return false;
            }

            m_Actions.push_back(std::move(action));
            argumentIndex = terminatorIndex;
            continue;
        }

        parserArguments.emplace_back(argument);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::VisitPath(
    const PString& path,
    const PString& name,
    size_t depth,
    const stat_t& statBuffer,
    dev_t traversalDevice)
{
    if (depth >= m_MinDepth && Matches(name, statBuffer.st_mode)) {
        PerformActions(path);
    }

    if (!S_ISDIR(statBuffer.st_mode) ||
        depth >= m_MaxDepth ||
        (m_StayOnDevice && statBuffer.st_dev != traversalDevice)) {
        return;
    }

    std::vector<PString> entries;
    ReadDirectoryEntries(path, entries);

    for (const PString& entryName : entries)
    {
        const PString childPath = MakeChildPath(path, entryName);
        stat_t childStatBuffer;

        if (ReadNodeStat(childPath, childStatBuffer))
        {
            VisitPath(
                childPath,
                entryName,
                depth + 1,
                childStatBuffer,
                traversalDevice);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::ReadNodeStat(const PString& path, stat_t& statBuffer)
{
    const int fileDescriptor =
        open(path.c_str(), O_PATH | O_NOFOLLOW);

    if (fileDescriptor == -1)
    {
        ReportError(PString::format_string(
            "{}: cannot access '{}': {}\n",
            m_CommandName,
            path,
            strerror(errno)));
        return false;
    }

    if (fstat(fileDescriptor, &statBuffer) != 0)
    {
        const int errorCode = errno;
        close(fileDescriptor);

        ReportError(PString::format_string(
            "{}: cannot stat '{}': {}\n",
            m_CommandName,
            path,
            strerror(errorCode)));
        return false;
    }

    close(fileDescriptor);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::ReadDirectoryEntries(
    const PString& path,
    std::vector<PString>& entries)
{
    const int directoryHandle =
        open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW);

    if (directoryHandle == -1)
    {
        ReportError(PString::format_string(
            "{}: cannot open directory '{}': {}\n",
            m_CommandName,
            path,
            strerror(errno)));
        return;
    }

    PDirEntryBuffer directoryEntryBuffer;

    for (;;)
    {
        const ssize_t readResult = posix_getdents(
            directoryHandle,
            directoryEntryBuffer.GetBuffer(),
            directoryEntryBuffer.GetSize(),
            0);

        if (readResult == 0) {
            break;
        }
        if (readResult < 0)
        {
            ReportError(PString::format_string(
                "{}: cannot read directory '{}': {}\n",
                m_CommandName,
                path,
                strerror(errno)));
            break;
        }

        for (PDirEntryIterator iterator(directoryEntryBuffer.GetBuffer(), size_t(readResult)); iterator; ++iterator)
        {
            const dirent_t& directoryEntry = *iterator;
            if (PString::is_dot_or_dot_dot(
                directoryEntry.d_name,
                directoryEntry.d_namlen)) {
                continue;
            }

            entries.emplace_back(
                directoryEntry.d_name,
                directoryEntry.d_namlen);
        }
    }

    close(directoryHandle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::Matches(const PString& name, mode_t mode) const
{
    if (m_UseNamePattern &&
        fnmatch(m_NamePattern.c_str(), name.c_str(), 0) != 0) {
        return false;
    }
    if (m_UseFileType && !MatchesFileType(mode)) {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::MatchesFileType(mode_t mode) const
{
    switch (m_FileType)
    {
        case 'b':
            return S_ISBLK(mode);
        case 'c':
            return S_ISCHR(mode);
        case 'd':
            return S_ISDIR(mode);
        case 'f':
            return S_ISREG(mode);
        case 'l':
            return S_ISLNK(mode);
        case 'p':
            return S_ISFIFO(mode);
        case 's':
            return S_ISSOCK(mode);
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::PerformActions(const PString& path)
{
    for (FindAction& action : m_Actions)
    {
        switch (action.Type)
        {
            case FindActionType::Print:
                PrintPath(path, false);
                break;

            case FindActionType::PrintNull:
                PrintPath(path, true);
                break;

            case FindActionType::Execute:
            case FindActionType::ExecuteDirectory:
                if (!ExecuteImmediateAction(action, path)) {
                    return;
                }
                break;

            case FindActionType::ExecuteBatch:
            case FindActionType::ExecuteDirectoryBatch:
                QueueBatchAction(action, path);
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::ExecuteImmediateAction(
    const FindAction& action,
    const PString& path)
{
    PString workingDirectory;
    PString replacement = path;

    if (IsDirectoryAction(action.Type)) {
        GetExecutionLocation(path, workingDirectory, replacement);
    }

    std::vector<PString> commandArguments = action.CommandArguments;

    for (PString& commandArgument : commandArguments) {
        ReplaceAll(commandArgument, "{}", replacement);
    }

    return RunCommand(
        std::move(commandArguments),
        workingDirectory,
        false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::QueueBatchAction(
    FindAction& action,
    const PString& path)
{
    PString workingDirectory;
    PString argument = path;

    if (IsDirectoryAction(action.Type))
    {
        GetExecutionLocation(path, workingDirectory, argument);

        if (!action.PendingArguments.empty() &&
            action.PendingDirectory != workingDirectory) {
            FlushPendingAction(action);
        }
    }

    const size_t argumentBytes =
        sizeof(char*) + argument.size() + 1;
    const size_t fixedArgumentBytes =
        CalculateArgumentBytes(action.CommandArguments);

    if (!action.PendingArguments.empty() &&
        fixedArgumentBytes + action.PendingArgumentBytes + argumentBytes >
            size_t(ARG_MAX)) {
        FlushPendingAction(action);
    }

    if (action.PendingArguments.empty()) {
        action.PendingDirectory = workingDirectory;
    }

    action.PendingArguments.push_back(std::move(argument));
    action.PendingArgumentBytes += argumentBytes;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::FlushPendingActions()
{
    for (FindAction& action : m_Actions)
    {
        if (IsBatchAction(action.Type)) {
            FlushPendingAction(action);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::FlushPendingAction(FindAction& action)
{
    if (action.PendingArguments.empty()) {
        return;
    }

    std::vector<PString> commandArguments = action.CommandArguments;
    commandArguments.reserve(
        commandArguments.size() + action.PendingArguments.size());

    for (PString& argument : action.PendingArguments) {
        commandArguments.push_back(std::move(argument));
    }

    const PString workingDirectory = action.PendingDirectory;
    action.PendingArguments.clear();
    action.PendingDirectory.clear();
    action.PendingArgumentBytes = 0;

    RunCommand(
        std::move(commandArguments),
        workingDirectory,
        true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::RunCommand(
    std::vector<PString>&& commandArguments,
    const PString& workingDirectory,
    bool nonZeroExitIsError)
{
    const PString executablePath = ResolveExecutablePath(
        commandArguments[0],
        workingDirectory);

    posix_spawn_file_actions_t fileActions;
    bool fileActionsInitialized = false;
    const posix_spawn_file_actions_t* fileActionsPointer = nullptr;

    if (!workingDirectory.empty())
    {
        int actionResult = posix_spawn_file_actions_init(&fileActions);

        if (actionResult == 0)
        {
            fileActionsInitialized = true;
            actionResult = posix_spawn_file_actions_addchdir(
                &fileActions,
                workingDirectory.c_str());
        }

        if (actionResult != 0)
        {
            if (fileActionsInitialized) {
                posix_spawn_file_actions_destroy(&fileActions);
            }

            ReportError(PString::format_string(
                "{}: cannot prepare execution of '{}': {}\n",
                m_CommandName,
                commandArguments[0],
                strerror(actionResult)));
            return false;
        }

        fileActionsPointer = &fileActions;
    }

    std::vector<char*> argumentPointers;
    argumentPointers.reserve(commandArguments.size() + 1);

    for (PString& commandArgument : commandArguments) {
        argumentPointers.push_back(commandArgument.data());
    }
    argumentPointers.push_back(nullptr);

    pid_t processID = -1;
    const int spawnResult = posix_spawn(
        &processID,
        executablePath.c_str(),
        fileActionsPointer,
        nullptr,
        argumentPointers.data(),
        environ);

    if (fileActionsInitialized) {
        posix_spawn_file_actions_destroy(&fileActions);
    }

    if (spawnResult != 0)
    {
        ReportError(PString::format_string(
            "{}: cannot execute '{}': {}\n",
            m_CommandName,
            commandArguments[0],
            strerror(spawnResult)));
        return false;
    }

    int waitStatus = 0;
    pid_t waitResult;

    do
    {
        waitResult = waitpid(processID, &waitStatus, 0);
    }
    while (waitResult == -1 && errno == EINTR);

    if (waitResult != processID)
    {
        ReportError(PString::format_string(
            "{}: cannot wait for '{}': {}\n",
            m_CommandName,
            commandArguments[0],
            strerror(errno)));
        return false;
    }

    const bool commandSucceeded =
        WIFEXITED(waitStatus) && WEXITSTATUS(waitStatus) == 0;

    if (nonZeroExitIsError && !commandSucceeded) {
        m_HadError = true;
    }
    return commandSucceeded;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdFind::GetBaseName(const PString& path)
{
    if (path.empty()) {
        return path;
    }

    size_t nameEnd = path.size();

    while (nameEnd > 1 && path[nameEnd - 1] == '/') {
        --nameEnd;
    }

    if (nameEnd == 1 && path[0] == '/') {
        return "/";
    }

    const size_t separator = path.rfind('/', nameEnd - 1);

    if (separator == PString::npos) {
        return PString(path.data(), nameEnd);
    }

    return PString(
        path.data() + separator + 1,
        nameEnd - separator - 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdFind::MakeChildPath(
    const PString& parentPath,
    const PString& childName)
{
    PString childPath = parentPath;

    if (childPath.empty() || childPath.back() != '/') {
        childPath.push_back('/');
    }

    childPath += childName;
    return childPath;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::GetExecutionLocation(
    const PString& path,
    PString& directory,
    PString& argument)
{
    size_t pathEnd = path.size();

    while (pathEnd > 1 && path[pathEnd - 1] == '/') {
        --pathEnd;
    }

    if (pathEnd == 1 && path[0] == '/')
    {
        directory = "/";
        argument = "./";
        return;
    }

    const size_t separator = path.rfind('/', pathEnd - 1);
    PString leaf;

    if (separator == PString::npos)
    {
        directory = ".";
        leaf.assign(path.data(), pathEnd);
    }
    else
    {
        directory.assign(
            path.data(),
            (separator == 0) ? 1 : separator);
        leaf.assign(
            path.data() + separator + 1,
            pathEnd - separator - 1);
    }

    argument = "./";
    argument += leaf;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdFind::ResolveExecutablePath(
    const PString& command,
    const PString& workingDirectory)
{
    if (command.empty() || command[0] == '/') {
        return command;
    }

    if (command.find('/') == PString::npos) {
        return PString("/bin/") + command;
    }

    PString path = command;

    if (!workingDirectory.empty()) {
        path = MakeChildPath(workingDirectory, command);
    }

    return PPath(path).GetPath();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::ReplaceAll(
    PString& text,
    std::string_view token,
    std::string_view replacement)
{
    size_t position = 0;

    while ((position = text.find(token, position)) != PString::npos)
    {
        text.replace(position, token.size(), replacement);
        position += replacement.size();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t CmdFind::CalculateArgumentBytes(
    const std::vector<PString>& arguments)
{
    size_t byteCount = sizeof(char*);

    for (const PString& argument : arguments) {
        byteCount += sizeof(char*) + argument.size() + 1;
    }

    return byteCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::IsBatchAction(FindActionType actionType)
{
    return actionType == FindActionType::ExecuteBatch ||
        actionType == FindActionType::ExecuteDirectoryBatch;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::IsDirectoryAction(FindActionType actionType)
{
    return actionType == FindActionType::ExecuteDirectory ||
        actionType == FindActionType::ExecuteDirectoryBatch;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFind::WriteAll(int fileDescriptor, std::string_view text)
{
    size_t bytesWritten = 0;

    while (bytesWritten < text.size())
    {
        const ssize_t result = write(
            fileDescriptor,
            text.data() + bytesWritten,
            text.size() - bytesWritten);

        if (result > 0)
        {
            bytesWritten += size_t(result);
        }
        else if (result < 0 && errno == EINTR)
        {
            continue;
        }
        else
        {
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::PrintPath(const PString& path, bool nullTerminate)
{
    PString output = path;
    output.push_back(nullTerminate ? '\0' : '\n');

    if (!WriteAll(STDOUT_FILENO, output))
    {
        ReportError(PString::format_string(
            "{}: failed to write output: {}\n",
            m_CommandName,
            strerror(errno)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFind::ReportError(const PString& text)
{
    m_HadError = true;
    WriteAll(STDERR_FILENO, text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int find_main(int argc, char* argv[])
{
    CmdFind find;
    return find.Run(argc, argv);
}


static PAppDefinition g_FindAppDef(
    "find",
    "Search for files in a directory hierarchy.",
    find_main);

} // namespace shutil_find
