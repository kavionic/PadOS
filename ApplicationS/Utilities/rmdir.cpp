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

#include "rmdir.h"

#include <argparse/argparse.hpp>

#include <System/AppDefinition.h>

#include "FileUtilityHelpers.h"


namespace shutil_rmdir
{

int CmdRmdir::Run(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Remove empty directories.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("--ignore-fail-on-non-empty")
        .help("Ignore failures caused solely by a non-empty directory.")
        .flag();
    program.add_argument("-p", "--parents")
        .help("Remove DIRECTORY and its empty ancestors.")
        .flag();
    program.add_argument("-v", "--verbose")
        .help("Print a diagnostic for every processed directory.")
        .flag();
    program.add_argument("directories")
        .help("Directories to remove.")
        .metavar("DIRECTORY")
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
    m_IgnoreNonEmpty = program.get<bool>("--ignore-fail-on-non-empty");
    m_RemoveParents = program.get<bool>("--parents");
    m_Verbose = program.get<bool>("--verbose");

    const std::vector<std::string> directories =
        program.is_used("directories")
            ? program.get<std::vector<std::string>>("directories")
            : std::vector<std::string>();

    if (directories.empty())
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: missing operand\n",
                m_CommandName));
        return 1;
    }

    for (const std::string& argument : directories)
    {
        PString path = shutil::TrimTrailingSlashes(argument);

        if (!RemoveDirectory(path, true)) {
            continue;
        }

        while (m_RemoveParents)
        {
            path = shutil::GetParentPath(path);

            if (path.empty() || path == "/") {
                break;
            }
            if (!RemoveDirectory(path, false)) {
                break;
            }
        }
    }

    return m_HadError ? 1 : 0;
}

bool CmdRmdir::RemoveDirectory(
    const PString& path,
    bool reportIgnoredFailure)
{
    if (m_Verbose)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            PString::format_string(
                "{}: removing directory '{}'\n",
                m_CommandName,
                path));
    }

    if (rmdir(path.c_str()) == 0) {
        return true;
    }

    const int errorCode = errno;

    if (m_IgnoreNonEmpty &&
        (errorCode == ENOTEMPTY || errorCode == EEXIST))
    {
        if (reportIgnoredFailure && m_Verbose)
        {
            shutil::WriteAll(
                STDOUT_FILENO,
                PString::format_string(
                    "{}: ignoring non-empty directory '{}'\n",
                    m_CommandName,
                    path));
        }
        return false;
    }

    ReportError(path, errorCode);
    return false;
}

void CmdRmdir::ReportError(const PString& path, int errorCode)
{
    m_HadError = true;
    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: failed to remove '{}': {}\n",
            m_CommandName,
            path,
            strerror(errorCode)));
}

int rmdir_main(int argc, char* argv[])
{
    CmdRmdir command;
    return command.Run(argc, argv);
}

static PAppDefinition g_RmdirAppDef(
    "rmdir",
    "Remove empty directories.",
    rmdir_main);

} // namespace shutil_rmdir
