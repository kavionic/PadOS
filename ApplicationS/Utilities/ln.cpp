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

#include "ln.h"

#include <argparse/argparse.hpp>

#include <System/AppDefinition.h>

#include "FileUtilityHelpers.h"


namespace shutil_ln
{

int CmdLn::Run(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description(
        "Create symbolic links. Hard links are not supported by PadOS.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("-s", "--symbolic")
        .help("Create symbolic links; this option is required.")
        .flag();
    program.add_argument("-f", "--force")
        .help("Remove existing destination files.")
        .flag();
    program.add_argument("-i", "--interactive")
        .help("Prompt whether to remove existing destinations.")
        .flag();
    program.add_argument("-n", "--no-dereference")
        .help("Treat a destination symlink to a directory as a file.")
        .flag();
    program.add_argument("-T", "--no-target-directory")
        .help("Always treat LINK_NAME as a normal file.")
        .flag();
    program.add_argument("-v", "--verbose")
        .help("Print the name of each linked file.")
        .flag();
    program.add_argument("operands")
        .help("TARGET... and optional LINK_NAME or DIRECTORY.")
        .metavar("TARGET [LINK_NAME]")
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
    m_Interactive = program.get<bool>("--interactive");
    m_NoDereference = program.get<bool>("--no-dereference");
    m_NoTargetDirectory = program.get<bool>("--no-target-directory");
    m_Verbose = program.get<bool>("--verbose");

    if (!program.get<bool>("--symbolic"))
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: hard links are not supported; use -s to create a symbolic link\n",
                m_CommandName));
        return 1;
    }

    const std::vector<std::string> operands =
        program.is_used("operands")
            ? program.get<std::vector<std::string>>("operands")
            : std::vector<std::string>();

    if (operands.empty())
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: missing file operand\n",
                m_CommandName));
        return 1;
    }
    if (operands.size() == 1)
    {
        CreateLink(operands[0], shutil::GetBaseName(operands[0]));
        return m_HadError ? 1 : 0;
    }

    const PString destinationOperand = operands.back();
    bool destinationIsDirectory = false;

    if (!m_NoTargetDirectory)
    {
        stat_t destinationStat;
        const bool followDestination =
            !m_NoDereference ||
            !shutil::ReadNodeStat(destinationOperand, destinationStat, false) ||
            !S_ISLNK(destinationStat.st_mode);

        destinationIsDirectory = shutil::IsDirectory(
            destinationOperand,
            followDestination);
    }

    if (operands.size() > 2 && !destinationIsDirectory)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: target '{}' is not a directory\n",
                m_CommandName,
                destinationOperand));
        return 1;
    }

    const size_t targetCount = operands.size() - 1;

    for (size_t index = 0; index < targetCount; ++index)
    {
        const PString target = operands[index];
        const PString linkPath = destinationIsDirectory
            ? shutil::MakeChildPath(
                destinationOperand,
                shutil::GetBaseName(target))
            : destinationOperand;

        CreateLink(target, linkPath);
    }

    return m_HadError ? 1 : 0;
}

bool CmdLn::CreateLink(const PString& target, const PString& linkPath)
{
    stat_t destinationStat;

    if (shutil::ReadNodeStat(linkPath, destinationStat, false) &&
        !RemoveExistingLinkPath(linkPath)) {
        return false;
    }

    if (symlink(target.c_str(), linkPath.c_str()) != 0)
    {
        ReportError("failed to create symbolic link", linkPath, errno);
        return false;
    }

    if (m_Verbose)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            PString::format_string(
                "'{}' -> '{}'\n",
                linkPath,
                target));
    }
    return true;
}

bool CmdLn::RemoveExistingLinkPath(const PString& linkPath)
{
    if (!m_Force && !m_Interactive)
    {
        ReportError("failed to create symbolic link", linkPath, EEXIST);
        return false;
    }
    if (m_Interactive &&
        !shutil::Confirm(PString::format_string(
            "{}: replace '{}'? ",
            m_CommandName,
            linkPath))) {
        return false;
    }
    if (unlink(linkPath.c_str()) != 0)
    {
        ReportError("cannot remove destination", linkPath, errno);
        return false;
    }
    return true;
}

void CmdLn::ReportError(
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

int ln_main(int argc, char* argv[])
{
    CmdLn command;
    return command.Run(argc, argv);
}

static PAppDefinition g_LnAppDef(
    "ln",
    "Create symbolic links.",
    ln_main);

} // namespace shutil_ln
