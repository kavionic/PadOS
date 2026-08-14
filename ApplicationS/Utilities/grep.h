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
// Created: 12.08.2026 23:30

#pragma once

#include <regex.h>
#include <string_view>
#include <vector>

#include <Utils/String.h>


namespace shutil_grep
{

enum class ParseArgumentsResult
{
    Success,
    Help,
    Error
};


enum class FilenameMode
{
    Automatic,
    Always,
    Never
};


class CmdGrep
{
public:
    CmdGrep() = default;
    ~CmdGrep();

    int Run(int argc, char* argv[]);

private:
    ParseArgumentsResult ParseArguments(int argc, char* argv[]);
    bool CompilePattern();
    bool SearchPath(const PString& fileName, bool prefixFilename);
    bool SearchDescriptor(int fileDescriptor, std::string_view displayName, bool prefixFilename);
    bool ProcessLine(
        const PString& line,
        std::string_view displayName,
        size_t lineNumber,
        bool prefixFilename,
        size_t& matchCount
    );

    PString GetRegexError(int errorCode) const;

    static bool WriteAll(int fileDescriptor, std::string_view text);
    static void PrintError(std::string_view text);

    regex_t              m_Regex = {};
    PString              m_CommandName;
    PString              m_Pattern;
    std::vector<PString> m_FileNames;
    FilenameMode         m_FilenameMode = FilenameMode::Automatic;
    bool                 m_IgnoreCase = false;
    bool                 m_InvertMatch = false;
    bool                 m_PrintLineNumbers = false;
    bool                 m_PrintCount = false;
    bool                 m_UseExtendedRegex = false;
    bool                 m_HasCompiledRegex = false;
    bool                 m_AnyMatch = false;
};

int grep_main(int argc, char* argv[]);

} // namespace shutil_grep
