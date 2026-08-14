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

#pragma once

#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <sys/pados_types.h>
#include <sys/stat.h>

#include <Utils/String.h>


namespace shutil_find
{

enum class ParseArgumentsResult
{
    Success,
    Help,
    Error
};


enum class FindActionType
{
    Print,
    PrintNull,
    Execute,
    ExecuteBatch,
    ExecuteDirectory,
    ExecuteDirectoryBatch
};


struct FindAction
{
    FindActionType       Type = FindActionType::Print;
    std::vector<PString> CommandArguments;
    std::vector<PString> PendingArguments;
    PString              PendingDirectory;
    size_t               PendingArgumentBytes = 0;
};


class CmdFind
{
public:
    int Run(int argc, char* argv[]);

private:
    ParseArgumentsResult ParseArguments(int argc, char* argv[]);
    bool ExtractActions(
        int argc,
        char* argv[],
        std::vector<std::string>& parserArguments
    );

    void VisitPath(
        const PString& path,
        const PString& name,
        size_t depth,
        const stat_t& statBuffer,
        dev_t traversalDevice
    );
    bool ReadNodeStat(const PString& path, stat_t& statBuffer);
    void ReadDirectoryEntries(const PString& path, std::vector<PString>& entries);
    bool Matches(const PString& name, mode_t mode) const;
    bool MatchesFileType(mode_t mode) const;
    void PerformActions(const PString& path);
    bool ExecuteImmediateAction(const FindAction& action, const PString& path);
    void QueueBatchAction(FindAction& action, const PString& path);
    void FlushPendingActions();
    void FlushPendingAction(FindAction& action);
    bool RunCommand(
        std::vector<PString>&& commandArguments,
        const PString& workingDirectory,
        bool nonZeroExitIsError
    );

    static PString GetBaseName(const PString& path);
    static PString MakeChildPath(const PString& parentPath, const PString& childName);
    static void GetExecutionLocation(
        const PString& path,
        PString& directory,
        PString& argument
    );
    static PString ResolveExecutablePath(
        const PString& command,
        const PString& workingDirectory
    );
    static void ReplaceAll(
        PString& text,
        std::string_view token,
        std::string_view replacement
    );
    static size_t CalculateArgumentBytes(const std::vector<PString>& arguments);
    static bool IsBatchAction(FindActionType actionType);
    static bool IsDirectoryAction(FindActionType actionType);
    static bool WriteAll(int fileDescriptor, std::string_view text);

    void PrintPath(const PString& path, bool nullTerminate);
    void ReportError(const PString& text);

    PString                 m_CommandName;
    std::vector<PString>    m_StartPaths;
    std::vector<FindAction> m_Actions;
    PString                 m_NamePattern;
    size_t                  m_MinDepth = 0;
    size_t                  m_MaxDepth = std::numeric_limits<size_t>::max();
    char                    m_FileType = '\0';
    bool                    m_UseNamePattern = false;
    bool                    m_UseFileType = false;
    bool                    m_StayOnDevice = false;
    bool                    m_HadError = false;
};

int find_main(int argc, char* argv[]);

} // namespace shutil_find
