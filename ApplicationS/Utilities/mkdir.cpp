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

#include "mkdir.h"

#include <argparse/argparse.hpp>

#include <System/AppDefinition.h>

#include "FileUtilityHelpers.h"


namespace shutil_mkdir
{

int CmdMkdir::Run(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Create directories.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("-m", "--mode")
        .help("Set file mode (octal), not a=rwx minus umask.")
        .metavar("MODE");
    program.add_argument("-p", "--parents")
        .help("Make parent directories as needed; no error if existing.")
        .flag();
    program.add_argument("-v", "--verbose")
        .help("Print a message for each created directory.")
        .flag();
    program.add_argument("directories")
        .help("Directories to create.")
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
    m_CreateParents = program.get<bool>("--parents");
    m_Verbose = program.get<bool>("--verbose");

    if (program.is_used("--mode"))
    {
        const std::string& modeText = program.get("--mode");

        if (!shutil::ParseOctalMode(modeText, m_Mode))
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: invalid mode '{}'; only octal modes are supported\n",
                    m_CommandName,
                    modeText));
            return 1;
        }
    }

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

    for (const std::string& directory : directories) {
        CreateDirectory(directory, true);
    }
    return m_HadError ? 1 : 0;
}

bool CmdMkdir::CreateDirectory(
    const PString& inputPath,
    bool requestedDirectory)
{
    const PString path = shutil::TrimTrailingSlashes(inputPath);
    const mode_t createMode = requestedDirectory ? m_Mode : 0777;

    if (path.empty())
    {
        ReportError(inputPath, ENOENT);
        return false;
    }
    if (mkdir(path.c_str(), createMode) == 0)
    {
        if (m_Verbose)
        {
            shutil::WriteAll(
                STDOUT_FILENO,
                PString::format_string(
                    "{}: created directory '{}'\n",
                    m_CommandName,
                    path));
        }
        return true;
    }

    int errorCode = errno;

    if (m_CreateParents && errorCode == EEXIST && shutil::IsDirectory(path)) {
        return true;
    }
    if (m_CreateParents && errorCode == ENOENT)
    {
        const PString parentPath = shutil::GetParentPath(path);

        if (!parentPath.empty() && parentPath != path &&
            CreateDirectory(parentPath, false) &&
            mkdir(path.c_str(), createMode) == 0)
        {
            if (m_Verbose)
            {
                shutil::WriteAll(
                    STDOUT_FILENO,
                    PString::format_string(
                        "{}: created directory '{}'\n",
                        m_CommandName,
                        path));
            }
            return true;
        }
        errorCode = errno;

        if (errorCode == EEXIST && shutil::IsDirectory(path)) {
            return true;
        }
    }

    ReportError(path, errorCode);
    return false;
}

void CmdMkdir::ReportError(const PString& path, int errorCode)
{
    m_HadError = true;
    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: cannot create directory '{}': {}\n",
            m_CommandName,
            path,
            strerror(errorCode)));
}

int mkdir_main(int argc, char* argv[])
{
    CmdMkdir command;
    return command.Run(argc, argv);
}

static PAppDefinition g_MkdirAppDef(
    "mkdir",
    "Create directories.",
    mkdir_main);

} // namespace shutil_mkdir
