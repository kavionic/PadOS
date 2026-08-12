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

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <regex.h>
#include <string_view>
#include <unistd.h>
#include <vector>

#include <argparse/argparse.hpp>

#include <System/AppDefinition.h>
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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

CmdGrep::~CmdGrep()
{
    if (m_HasCompiledRegex) {
        regfree(&m_Regex);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int CmdGrep::Run(int argc, char* argv[])
{
    m_CommandName = argv[0];

    const ParseArgumentsResult parseResult = ParseArguments(argc, argv);

    if (parseResult == ParseArgumentsResult::Help) {
        return 0;
    }
    if (parseResult == ParseArgumentsResult::Error) {
        return 2;
    }
    if (!CompilePattern()) {
        return 2;
    }

    const bool prefixFilename =
        m_FilenameMode == FilenameMode::Always ||
        (m_FilenameMode == FilenameMode::Automatic && m_FileNames.size() > 1);

    bool hadError = false;

    if (m_FileNames.empty())
    {
        if (!SearchPath("-", prefixFilename)) {
            hadError = true;
        }
    }
    else
    {
        for (const PString& fileName : m_FileNames)
        {
            if (!SearchPath(fileName, prefixFilename)) {
                hadError = true;
            }
        }
    }

    int result = 1;
    if (m_AnyMatch) {
        result = 0;
    }
    if (hadError) {
        result = 2;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ParseArgumentsResult CmdGrep::ParseArguments(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Search files for lines matching a regular expression.");

    program.add_argument("--help")
        .help("Print argument help.")
        .flag();

    program.add_argument("-E", "--extended-regexp")
        .help("Interpret PATTERN as an extended regular expression.")
        .flag();

    program.add_argument("-i", "--ignore-case")
        .help("Ignore case distinctions in patterns and input data.")
        .flag();

    program.add_argument("-v", "--invert-match")
        .help("Select non-matching lines.")
        .flag();

    program.add_argument("-n", "--line-number")
        .help("Print the line number with each selected line.")
        .flag();

    program.add_argument("-c", "--count")
        .help("Print only a count of selected lines per input file.")
        .flag();

    program.add_argument("-H", "--with-filename")
        .help("Print the file name with each selected line.")
        .flag();

    program.add_argument("-h", "--no-filename")
        .help("Suppress file names in output.")
        .flag();

    program.add_argument("pattern")
        .help("Basic regular expression to search for.")
        .metavar("PATTERN")
        .nargs(0, 1);

    program.add_argument("files")
        .help("Files to search; use - or omit FILE to read standard input.")
        .metavar("FILE")
        .remaining();

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& exception)
    {
        PrintError(PString::format_string("{}\n", exception.what()));
        PrintError(program.help().str());
        return ParseArgumentsResult::Error;
    }

    if (program.get<bool>("--help"))
    {
        WriteAll(STDOUT_FILENO, program.help().str());
        return ParseArgumentsResult::Help;
    }

    if (!program.is_used("pattern"))
    {
        PrintError(PString::format_string(
            "{}: missing search pattern\n",
            argv[0]));
        PrintError(program.help().str());
        return ParseArgumentsResult::Error;
    }

    m_Pattern = program.get("pattern");
    m_IgnoreCase = program.get<bool>("--ignore-case");
    m_InvertMatch = program.get<bool>("--invert-match");
    m_PrintLineNumbers = program.get<bool>("--line-number");
    m_PrintCount = program.get<bool>("--count");
    m_UseExtendedRegex = program.get<bool>("--extended-regexp");

    if (program.get<bool>("--no-filename")) {
        m_FilenameMode = FilenameMode::Never;
    } else if (program.get<bool>("--with-filename")) {
        m_FilenameMode = FilenameMode::Always;
    }

    if (program.is_used("files"))
    {
        const std::vector<std::string> fileNames =
            program.get<std::vector<std::string>>("files");

        m_FileNames.reserve(fileNames.size());

        for (const std::string& fileName : fileNames) {
            m_FileNames.emplace_back(fileName);
        }
    }

    return ParseArgumentsResult::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdGrep::CompilePattern()
{
    int compileFlags = 0;

    if (m_UseExtendedRegex) {
        compileFlags |= REG_EXTENDED;
    }
    if (m_IgnoreCase) {
        compileFlags |= REG_ICASE;
    }

    const int result = regcomp(&m_Regex, m_Pattern.c_str(), compileFlags);

    if (result != 0)
    {
        PrintError(PString::format_string(
            "{}: invalid regular expression: {}\n",
            m_CommandName,
            GetRegexError(result)));
        return false;
    }

    m_HasCompiledRegex = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdGrep::SearchPath(const PString& fileName, bool prefixFilename)
{
    const bool useStandardInput = fileName == "-";
    const std::string_view displayName =
        useStandardInput ? std::string_view("(standard input)") : std::string_view(fileName);

    int fileDescriptor = STDIN_FILENO;

    if (!useStandardInput)
    {
        fileDescriptor = open(fileName.c_str(), O_RDONLY);

        if (fileDescriptor == -1)
        {
            PrintError(PString::format_string(
                "{}: cannot open '{}': {}\n",
                m_CommandName,
                fileName,
                strerror(errno)));
            return false;
        }
    }

    const bool success =
        SearchDescriptor(fileDescriptor, displayName, prefixFilename);

    if (!useStandardInput) {
        close(fileDescriptor);
    }

    return success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdGrep::SearchDescriptor(
    int fileDescriptor,
    std::string_view displayName,
    bool prefixFilename)
{
    PString currentLine;
    char buffer[2048];
    size_t lineNumber = 1;
    size_t matchCount = 0;

    for (;;)
    {
        const ssize_t bytesRead = read(fileDescriptor, buffer, sizeof(buffer));

        if (bytesRead < 0)
        {
            if (errno == EINTR) {
                continue;
            }

            PrintError(PString::format_string(
                "{}: failed to read '{}': {}\n",
                m_CommandName,
                displayName,
                strerror(errno)));
            return false;
        }
        if (bytesRead == 0) {
            break;
        }

        for (size_t byteIndex = 0; byteIndex < size_t(bytesRead); ++byteIndex)
        {
            if (buffer[byteIndex] == '\n')
            {
                if (!ProcessLine(
                    currentLine,
                    displayName,
                    lineNumber,
                    prefixFilename,
                    matchCount)) {
                    return false;
                }

                currentLine.clear();
                ++lineNumber;
            }
            else
            {
                currentLine.push_back(buffer[byteIndex]);
            }
        }
    }

    if (!currentLine.empty())
    {
        if (!ProcessLine(
            currentLine,
            displayName,
            lineNumber,
            prefixFilename,
            matchCount)) {
            return false;
        }
    }

    if (m_PrintCount)
    {
        PString output;

        if (prefixFilename)
        {
            output.append(displayName.data(), displayName.size());
            output.push_back(':');
        }
        output += PString::format_string("{}\n", matchCount);

        if (!WriteAll(STDOUT_FILENO, output))
        {
            PrintError(PString::format_string(
                "{}: failed to write output: {}\n",
                m_CommandName,
                strerror(errno)));
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdGrep::ProcessLine(
    const PString& line,
    std::string_view displayName,
    size_t lineNumber,
    bool prefixFilename,
    size_t& matchCount)
{
    const int matchResult = regexec(
        &m_Regex,
        line.c_str(),
        0,
        nullptr,
        0);

    if (matchResult != 0 && matchResult != REG_NOMATCH)
    {
        PrintError(PString::format_string(
            "{}: regular expression matching failed: {}\n",
            m_CommandName,
            GetRegexError(matchResult)));
        return false;
    }

    const bool matches = matchResult == 0;
    const bool selected = matches != m_InvertMatch;

    if (!selected) {
        return true;
    }

    m_AnyMatch = true;
    ++matchCount;

    if (m_PrintCount) {
        return true;
    }

    PString output;

    if (prefixFilename)
    {
        output.append(displayName.data(), displayName.size());
        output.push_back(':');
    }
    if (m_PrintLineNumbers) {
        output += PString::format_string("{}:", lineNumber);
    }

    output += line;
    output.push_back('\n');

    if (!WriteAll(STDOUT_FILENO, output))
    {
        PrintError(PString::format_string(
            "{}: failed to write output: {}\n",
            m_CommandName,
            strerror(errno)));
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdGrep::GetRegexError(int errorCode) const
{
    const size_t requiredSize =
        regerror(errorCode, &m_Regex, nullptr, 0);

    if (requiredSize == 0) {
        return "unknown regular expression error";
    }

    PString message;
    message.resize(requiredSize);
    regerror(errorCode, &m_Regex, message.data(), message.size());

    if (!message.empty() && message.back() == '\0') {
        message.pop_back();
    }

    return message;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdGrep::WriteAll(int fileDescriptor, std::string_view text)
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

void CmdGrep::PrintError(std::string_view text)
{
    WriteAll(STDERR_FILENO, text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int grep_main(int argc, char* argv[])
{
    CmdGrep grep;
    return grep.Run(argc, argv);
}


static PAppDefinition g_GrepAppDef(
    "grep",
    "Search files for lines matching a regular expression.",
    grep_main);

} // namespace shutil_grep
