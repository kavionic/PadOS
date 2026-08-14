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

#include "rm.h"

#include <argparse/argparse.hpp>

#include <System/AppDefinition.h>

#include "FileUtilityHelpers.h"


namespace shutil_rm
{

int CmdRm::Run(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Remove files or directories.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("-f", "--force")
        .help("Ignore nonexistent files and never prompt.")
        .flag();
    program.add_argument("-i")
        .help("Prompt before every removal.")
        .flag();
    program.add_argument("-r", "-R", "--recursive")
        .help("Remove directories and their contents recursively.")
        .flag();
    program.add_argument("-d", "--dir")
        .help("Remove empty directories.")
        .flag();
    program.add_argument("-v", "--verbose")
        .help("Explain what is being done.")
        .flag();
    program.add_argument("files")
        .help("Files or directories to remove.")
        .metavar("FILE")
        .nargs(argparse::nargs_pattern::any);

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& exception)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string("{}\n", exception.what()));
        shutil::WriteAll(STDERR_FILENO, program.help().str());
        return 1;
    }

    if (program.get<bool>("--help"))
    {
        shutil::WriteAll(STDOUT_FILENO, program.help().str());
        return 0;
    }

    m_CommandName = argv[0];
    m_Force = program.get<bool>("--force");
    m_Interactive = program.get<bool>("-i") && !m_Force;
    m_Recursive = program.get<bool>("--recursive");
    m_RemoveEmptyDirectories = program.get<bool>("--dir");
    m_Verbose = program.get<bool>("--verbose");

    const std::vector<std::string> files =
        program.get<std::vector<std::string>>("files");

    if (files.empty() && !m_Force)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: missing operand\nTry '{} --help' for more information.\n",
                m_CommandName,
                m_CommandName));
        return 1;
    }

    for (const std::string& file : files) {
        RemovePath(file);
    }
    return m_HadError ? 1 : 0;
}

bool CmdRm::RemovePath(const PString& inputPath)
{
    const PString path = shutil::TrimTrailingSlashes(inputPath);
    const PString baseName = shutil::GetBaseName(path);

    if (baseName == "." || baseName == "..")
    {
        ReportError("refusing to remove", path, EINVAL);
        return false;
    }
    if (shutil::GetAbsolutePath(path) == "/")
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: refusing to remove '/' recursively\n",
                m_CommandName));
        m_HadError = true;
        return false;
    }

    stat_t statBuffer;

    if (!shutil::ReadNodeStat(path, statBuffer, false))
    {
        const int errorCode = errno;

        if (m_Force && errorCode == ENOENT) {
            return true;
        }
        ReportError("cannot remove", path, errorCode);
        return false;
    }

    if (S_ISDIR(statBuffer.st_mode))
    {
        if (m_Recursive) {
            return RemoveDirectoryTree(path);
        }
        if (!m_RemoveEmptyDirectories)
        {
            ReportError("cannot remove", path, EISDIR);
            return false;
        }
        if (!ShouldRemove(path, true)) {
            return true;
        }
        if (rmdir(path.c_str()) != 0)
        {
            ReportError("cannot remove", path, errno);
            return false;
        }
    }
    else
    {
        if (!ShouldRemove(path, false)) {
            return true;
        }
        if (unlink(path.c_str()) != 0)
        {
            ReportError("cannot remove", path, errno);
            return false;
        }
    }

    if (m_Verbose)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            PString::format_string("removed '{}'\n", path));
    }
    return true;
}

bool CmdRm::RemoveDirectoryTree(const PString& path)
{
    std::vector<PString> entries;
    int errorCode = 0;

    if (!shutil::ReadDirectoryEntries(path, entries, errorCode))
    {
        ReportError("cannot read directory", path, errorCode);
        return false;
    }

    bool success = true;

    for (const PString& entry : entries)
    {
        if (!RemovePath(shutil::MakeChildPath(path, entry))) {
            success = false;
        }
    }

    if (!success) {
        return false;
    }
    if (!ShouldRemove(path, true)) {
        return true;
    }
    if (rmdir(path.c_str()) != 0)
    {
        ReportError("cannot remove directory", path, errno);
        return false;
    }
    if (m_Verbose)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            PString::format_string("removed directory '{}'\n", path));
    }
    return true;
}

bool CmdRm::ShouldRemove(const PString& path, bool isDirectory) const
{
    if (!m_Interactive) {
        return true;
    }
    return shutil::Confirm(PString::format_string(
        "{}: remove {}'{}'? ",
        m_CommandName,
        isDirectory ? "directory " : "",
        path));
}

void CmdRm::ReportError(
    const PString& operation,
    const PString& path,
    int errorCode)
{
    m_HadError = true;
    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: {} '{}': {}\n",
            m_CommandName,
            operation,
            path,
            strerror(errorCode)));
}

int rm_main(int argc, char* argv[])
{
    CmdRm command;
    return command.Run(argc, argv);
}

static PAppDefinition g_RmAppDef(
    "rm",
    "Remove files or directories.",
    rm_main);

} // namespace shutil_rm
