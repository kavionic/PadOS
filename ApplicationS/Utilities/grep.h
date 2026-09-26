// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
