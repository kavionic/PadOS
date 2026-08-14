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

#include "mv.h"

#include <array>
#include <cstdio>

#include <argparse/argparse.hpp>

#include <System/AppDefinition.h>

#include "FileUtilityHelpers.h"


namespace shutil_mv
{

int CmdMv::Run(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Move or rename files and directories.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("-f", "--force")
        .help("Do not prompt before overwriting.")
        .flag();
    program.add_argument("-i", "--interactive")
        .help("Prompt before overwriting.")
        .flag();
    program.add_argument("-n", "--no-clobber")
        .help("Do not overwrite an existing file.")
        .flag();
    program.add_argument("-T", "--no-target-directory")
        .help("Treat DEST as a normal file.")
        .flag();
    program.add_argument("-v", "--verbose")
        .help("Explain what is being done.")
        .flag();
    program.add_argument("operands")
        .help("SOURCE... followed by DEST.")
        .metavar("SOURCE... DEST")
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
    m_NoClobber = program.get<bool>("--no-clobber");
    m_NoTargetDirectory = program.get<bool>("--no-target-directory");
    m_Verbose = program.get<bool>("--verbose");

    const size_t overwriteOptionCount =
        size_t(m_Force) + size_t(m_Interactive) + size_t(m_NoClobber);

    if (overwriteOptionCount > 1)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: -f, -i, and -n are mutually exclusive\n",
                m_CommandName));
        return 1;
    }

    const std::vector<std::string> operands =
        program.is_used("operands")
            ? program.get<std::vector<std::string>>("operands")
            : std::vector<std::string>();

    if (operands.size() < 2)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: missing destination file operand\n",
                m_CommandName));
        return 1;
    }

    const PString destinationOperand = operands.back();
    const bool destinationIsDirectory =
        !m_NoTargetDirectory &&
        shutil::IsDirectory(destinationOperand, true);

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

    const size_t sourceCount = operands.size() - 1;

    for (size_t index = 0; index < sourceCount; ++index)
    {
        const PString sourcePath =
            shutil::TrimTrailingSlashes(operands[index]);
        const PString destinationPath = destinationIsDirectory
            ? shutil::MakeChildPath(
                destinationOperand,
                shutil::GetBaseName(sourcePath))
            : destinationOperand;

        MovePath(sourcePath, destinationPath);
    }

    return m_HadError ? 1 : 0;
}

bool CmdMv::MovePath(
    const PString& sourcePath,
    const PString& destinationPath)
{
    stat_t sourceStat;

    if (!shutil::ReadNodeStat(sourcePath, sourceStat, false))
    {
        ReportError("cannot stat", sourcePath, errno);
        return false;
    }
    if (S_ISDIR(sourceStat.st_mode) &&
        shutil::IsPathWithin(sourcePath, destinationPath))
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: cannot move directory '{}' into itself, '{}'\n",
                m_CommandName,
                sourcePath,
                destinationPath));
        m_HadError = true;
        return false;
    }

    stat_t destinationStat;
    const bool destinationExists =
        shutil::ReadNodeStat(destinationPath, destinationStat, false);

    if (destinationExists)
    {
        if (sourceStat.st_dev == destinationStat.st_dev &&
            sourceStat.st_ino == destinationStat.st_ino)
        {
            if (m_Verbose)
            {
                shutil::WriteAll(
                    STDOUT_FILENO,
                    PString::format_string(
                        "{}: '{}' and '{}' are the same file\n",
                        m_CommandName,
                        sourcePath,
                        destinationPath));
            }
            return true;
        }
        if (m_NoClobber) {
            return true;
        }
        if (m_Interactive &&
            !shutil::Confirm(PString::format_string(
                "{}: overwrite '{}'? ",
                m_CommandName,
                destinationPath))) {
            return true;
        }
    }

    if (rename(sourcePath.c_str(), destinationPath.c_str()) == 0)
    {
        PrintMoved(sourcePath, destinationPath);
        return true;
    }

    const int renameError = errno;

    if (renameError != EXDEV)
    {
        ReportMoveError(sourcePath, destinationPath, renameError);
        return false;
    }
    if (!CopyAcrossFilesystems(
        sourcePath,
        destinationPath,
        sourceStat,
        destinationExists ? &destinationStat : nullptr)) {
        return false;
    }
    if (!RemoveSource(sourcePath)) {
        return false;
    }

    PrintMoved(sourcePath, destinationPath);
    return true;
}

bool CmdMv::CopyAcrossFilesystems(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat,
    const stat_t* destinationStat)
{
    if (destinationStat != nullptr)
    {
        if (S_ISDIR(sourceStat.st_mode))
        {
            if (!S_ISDIR(destinationStat->st_mode))
            {
                ReportMoveError(sourcePath, destinationPath, ENOTDIR);
                return false;
            }
            if (rmdir(destinationPath.c_str()) != 0)
            {
                ReportMoveError(sourcePath, destinationPath, errno);
                return false;
            }
        }
        else
        {
            if (S_ISDIR(destinationStat->st_mode))
            {
                ReportMoveError(sourcePath, destinationPath, EISDIR);
                return false;
            }
            if (unlink(destinationPath.c_str()) != 0)
            {
                ReportMoveError(sourcePath, destinationPath, errno);
                return false;
            }
        }
    }

    if (S_ISREG(sourceStat.st_mode)) {
        return CopyRegularFile(sourcePath, destinationPath, sourceStat);
    }
    if (S_ISDIR(sourceStat.st_mode)) {
        return CopyDirectory(sourcePath, destinationPath, sourceStat);
    }
    if (S_ISLNK(sourceStat.st_mode)) {
        return CopySymlink(sourcePath, destinationPath, sourceStat);
    }

    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: cannot move unsupported file type '{}'\n",
            m_CommandName,
            sourcePath));
    m_HadError = true;
    return false;
}

bool CmdMv::CopyRegularFile(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat)
{
    const int sourceFile = open(sourcePath.c_str(), O_RDONLY);

    if (sourceFile == -1)
    {
        ReportError("cannot open", sourcePath, errno);
        return false;
    }

    const int destinationFile = open(
        destinationPath.c_str(),
        O_WRONLY | O_CREAT | O_EXCL,
        sourceStat.st_mode & 0777);

    if (destinationFile == -1)
    {
        const int errorCode = errno;
        close(sourceFile);
        ReportError("cannot create", destinationPath, errorCode);
        return false;
    }

    const bool success = WriteFileContents(
        sourceFile,
        destinationFile,
        sourcePath,
        destinationPath);
    const int closeSourceResult = close(sourceFile);
    const int closeSourceError = errno;
    const int closeDestinationResult = close(destinationFile);
    const int closeDestinationError = errno;

    if (closeSourceResult != 0)
    {
        ReportError("cannot close", sourcePath, closeSourceError);
        return false;
    }
    if (closeDestinationResult != 0)
    {
        ReportError("cannot close", destinationPath, closeDestinationError);
        return false;
    }
    return success;
}

bool CmdMv::CopyDirectory(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat)
{
    if (shutil::IsPathWithin(sourcePath, destinationPath))
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: cannot move directory '{}' into itself, '{}'\n",
                m_CommandName,
                sourcePath,
                destinationPath));
        m_HadError = true;
        return false;
    }
    if (mkdir(destinationPath.c_str(), sourceStat.st_mode & 0777) != 0)
    {
        ReportError("cannot create directory", destinationPath, errno);
        return false;
    }

    std::vector<PString> entries;
    int errorCode = 0;

    if (!shutil::ReadDirectoryEntries(sourcePath, entries, errorCode))
    {
        ReportError("cannot read directory", sourcePath, errorCode);
        return false;
    }

    bool success = true;

    for (const PString& entry : entries)
    {
        const PString childSource =
            shutil::MakeChildPath(sourcePath, entry);
        const PString childDestination =
            shutil::MakeChildPath(destinationPath, entry);
        stat_t childStat;

        if (!shutil::ReadNodeStat(childSource, childStat, false))
        {
            ReportError("cannot stat", childSource, errno);
            success = false;
        }
        else if (!CopyAcrossFilesystems(
            childSource,
            childDestination,
            childStat,
            nullptr))
        {
            success = false;
        }
    }
    return success;
}

bool CmdMv::CopySymlink(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat)
{
    PString target;
    int errorCode = 0;

    if (!shutil::ReadSymlinkTarget(
        sourcePath,
        sourceStat,
        target,
        errorCode))
    {
        ReportError("cannot read symbolic link", sourcePath, errorCode);
        return false;
    }
    if (symlink(target.c_str(), destinationPath.c_str()) != 0)
    {
        ReportError("cannot create symbolic link", destinationPath, errno);
        return false;
    }
    return true;
}

bool CmdMv::RemoveSource(const PString& path)
{
    stat_t statBuffer;

    if (!shutil::ReadNodeStat(path, statBuffer, false))
    {
        ReportError("cannot stat", path, errno);
        return false;
    }
    if (!S_ISDIR(statBuffer.st_mode))
    {
        if (unlink(path.c_str()) == 0) {
            return true;
        }
        ReportError("cannot remove", path, errno);
        return false;
    }

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
        if (!RemoveSource(shutil::MakeChildPath(path, entry))) {
            success = false;
        }
    }
    if (!success) {
        return false;
    }
    if (rmdir(path.c_str()) == 0) {
        return true;
    }

    ReportError("cannot remove directory", path, errno);
    return false;
}

bool CmdMv::WriteFileContents(
    int sourceFile,
    int destinationFile,
    const PString& sourcePath,
    const PString& destinationPath)
{
    std::array<char, 32768> buffer;

    for (;;)
    {
        const ssize_t bytesRead = read(
            sourceFile,
            buffer.data(),
            buffer.size());

        if (bytesRead < 0)
        {
            if (errno == EINTR) {
                continue;
            }
            ReportError("error reading", sourcePath, errno);
            return false;
        }
        if (bytesRead == 0) {
            return true;
        }

        size_t bytesWritten = 0;

        while (bytesWritten < static_cast<size_t>(bytesRead))
        {
            const ssize_t result = write(
                destinationFile,
                buffer.data() + bytesWritten,
                static_cast<size_t>(bytesRead) - bytesWritten);

            if (result < 0)
            {
                if (errno == EINTR) {
                    continue;
                }
                ReportError("error writing", destinationPath, errno);
                return false;
            }
            if (result == 0)
            {
                ReportError("error writing", destinationPath, EIO);
                return false;
            }
            bytesWritten += static_cast<size_t>(result);
        }
    }
}

void CmdMv::PrintMoved(
    const PString& sourcePath,
    const PString& destinationPath) const
{
    if (m_Verbose)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            PString::format_string(
                "renamed '{}' -> '{}'\n",
                sourcePath,
                destinationPath));
    }
}

void CmdMv::ReportError(
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

void CmdMv::ReportMoveError(
    const PString& sourcePath,
    const PString& destinationPath,
    int errorCode)
{
    m_HadError = true;
    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: cannot move '{}' to '{}': {}\n",
            m_CommandName,
            sourcePath,
            destinationPath,
            strerror(errorCode)));
}

int mv_main(int argc, char* argv[])
{
    CmdMv command;
    return command.Run(argc, argv);
}

static PAppDefinition g_MvAppDef(
    "mv",
    "Move or rename files and directories.",
    mv_main);

} // namespace shutil_mv
