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

#include "cp.h"

#include <array>
#include <optional>
#include <string_view>

#include <argparse/argparse.hpp>

#include <Storage/FSNode.h>
#include <System/AppDefinition.h>

#include "FileUtilityHelpers.h"


namespace shutil_cp
{

int CmdCp::Run(int argc, char* argv[])
{
    std::vector<PString> sourcePaths;
    PString destinationPath;

    if (!ParseArguments(
        argc,
        argv,
        sourcePaths,
        destinationPath)) {
        return m_HadError ? 1 : 0;
    }

    const bool destinationIsDirectory =
        !m_NoTargetDirectory &&
        shutil::IsDirectory(destinationPath, true);

    if ((sourcePaths.size() > 1 || m_Parents) &&
        !destinationIsDirectory)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: target '{}' is not a directory\n",
                m_CommandName,
                destinationPath));
        return 1;
    }

    for (const PString& sourcePath : sourcePaths)
    {
        PString finalDestination;

        if (m_Parents)
        {
            finalDestination = BuildParentsDestination(
                sourcePath,
                destinationPath);
        }
        else if (destinationIsDirectory)
        {
            finalDestination = shutil::MakeChildPath(
                destinationPath,
                shutil::GetBaseName(sourcePath));
        }
        else
        {
            finalDestination = destinationPath;
        }

        CopyCommandLinePath(sourcePath, finalDestination);
    }

    return m_HadError ? 1 : 0;
}

bool CmdCp::ParseArguments(
    int argc,
    char* argv[],
    std::vector<PString>& sourcePaths,
    PString& destinationPath)
{
    std::vector<std::string> parserArguments;
    parserArguments.reserve(static_cast<size_t>(argc));
    parserArguments.emplace_back(argv[0]);

    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument(argv[index]);

        if (argument == "--backup") {
            parserArguments.emplace_back("--backup=existing");
        } else if (argument == "--preserve") {
            parserArguments.emplace_back(
                "--preserve=mode,ownership,timestamps");
        } else {
            parserArguments.emplace_back(argument);
        }
    }

    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Copy files and directories.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("-a", "--archive")
        .help("Archive using supported attributes: -R -P and preserve mode, ownership, timestamps.")
        .flag();
    program.add_argument("--attributes-only")
        .help("Do not copy file data, only supported attributes.")
        .flag();
    program.add_argument("--backup")
        .help("Make a backup of each existing destination file.")
        .metavar("CONTROL");
    program.add_argument("-b")
        .help("Like --backup=existing.")
        .flag();
    program.add_argument("-d")
        .help("Preserve symbolic links without dereferencing them.")
        .flag();
    program.add_argument("-f", "--force")
        .help("Remove a destination that cannot be opened.")
        .flag();
    program.add_argument("-H")
        .help("Follow command-line symbolic links in SOURCE.")
        .flag();
    program.add_argument("-i", "--interactive")
        .help("Prompt before overwriting.")
        .flag();
    program.add_argument("-L", "--dereference")
        .help("Always follow symbolic links in SOURCE.")
        .flag();
    program.add_argument("-n", "--no-clobber")
        .help("Do not overwrite an existing file.")
        .flag();
    program.add_argument("-P", "--no-dereference")
        .help("Never follow symbolic links in SOURCE.")
        .flag();
    program.add_argument("-p")
        .help("Preserve mode, ownership, and timestamps.")
        .flag();
    program.add_argument("--preserve")
        .help("Preserve mode, ownership, timestamps, or all supported attributes.")
        .metavar("ATTR_LIST");
    program.add_argument("--no-preserve")
        .help("Do not preserve attributes from ATTR_LIST.")
        .metavar("ATTR_LIST");
    program.add_argument("--parents")
        .help("Use the source file name under DIRECTORY.")
        .flag();
    program.add_argument("-r", "-R", "--recursive")
        .help("Copy directories recursively.")
        .flag();
    program.add_argument("--remove-destination")
        .help("Remove each existing destination before opening it.")
        .flag();
    program.add_argument("--strip-trailing-slashes")
        .help("Remove trailing slashes from each SOURCE argument.")
        .flag();
    program.add_argument("-s", "--symbolic-link")
        .help("Make symbolic links instead of copying.")
        .flag();
    program.add_argument("-S", "--suffix")
        .help("Override the usual backup suffix.")
        .metavar("SUFFIX");
    program.add_argument("-t", "--target-directory")
        .help("Copy all SOURCE arguments into DIRECTORY.")
        .metavar("DIRECTORY");
    program.add_argument("-T", "--no-target-directory")
        .help("Treat DEST as a normal file.")
        .flag();
    program.add_argument("-u", "--update")
        .help("Copy only when SOURCE is newer or DEST is missing.")
        .flag();
    program.add_argument("-v", "--verbose")
        .help("Explain what is being done.")
        .flag();
    program.add_argument("-x", "--one-file-system")
        .help("Stay on the command-line source file system.")
        .flag();
    program.add_argument("operands")
        .help("SOURCE... followed by DEST unless -t is used.")
        .metavar("SOURCE... [DEST]")
        .nargs(argparse::nargs_pattern::any);

    try
    {
        program.parse_args(parserArguments);
    }
    catch (const std::exception& exception)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string("{}\n", exception.what()));
        shutil::WriteAll(STDERR_FILENO, program.help().str());
        m_HadError = true;
        return false;
    }

    if (program.get<bool>("--help"))
    {
        shutil::WriteAll(STDOUT_FILENO, program.help().str());
        return false;
    }

    m_CommandName = argv[0];
    const bool archive = program.get<bool>("--archive");
    m_AttributesOnly = program.get<bool>("--attributes-only");
    m_Force = program.get<bool>("--force");
    m_Interactive = program.get<bool>("--interactive");
    m_NoClobber = program.get<bool>("--no-clobber");
    m_OneFileSystem = program.get<bool>("--one-file-system");
    m_Parents = program.get<bool>("--parents");
    m_Recursive = archive || program.get<bool>("--recursive");
    m_RemoveDestination = program.get<bool>("--remove-destination");
    m_StripTrailingSlashes =
        program.get<bool>("--strip-trailing-slashes");
    m_SymbolicLink = program.get<bool>("--symbolic-link");
    m_NoTargetDirectory = program.get<bool>("--no-target-directory");
    m_Update = program.get<bool>("--update");
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
        m_HadError = true;
        return false;
    }

    const bool dereference = program.get<bool>("--dereference");
    const bool commandLineDereference = program.get<bool>("-H");
    const bool noDereference =
        archive ||
        program.get<bool>("-d") ||
        program.get<bool>("--no-dereference");
    const size_t dereferenceOptionCount =
        size_t(dereference) +
        size_t(commandLineDereference) +
        size_t(noDereference);

    if (dereferenceOptionCount > 1)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: -H, -L, -P, -d, and -a dereference modes conflict\n",
                m_CommandName));
        m_HadError = true;
        return false;
    }
    if (dereference) {
        m_DereferenceMode = DereferenceMode::Always;
    } else if (noDereference) {
        m_DereferenceMode = DereferenceMode::Never;
    } else if (commandLineDereference) {
        m_DereferenceMode = DereferenceMode::CommandLineOnly;
    } else {
        m_DereferenceMode = m_Recursive
            ? DereferenceMode::Never
            : DereferenceMode::CommandLineOnly;
    }

    if (archive || program.get<bool>("-p")) {
        m_PreserveAttributes |= PreserveAttribute_All;
    }
    if (program.is_used("--preserve") &&
        !ParsePreserveAttributes(
            program.get("--preserve"),
            true)) {
        return false;
    }
    if (program.is_used("--no-preserve") &&
        !ParsePreserveAttributes(
            program.get("--no-preserve"),
            false)) {
        return false;
    }

    if (program.get<bool>("-b")) {
        m_BackupMode = BackupMode::Existing;
    }
    if (program.is_used("--backup") &&
        !ParseBackupMode(program.get("--backup"))) {
        return false;
    }
    if (program.is_used("--suffix"))
    {
        m_BackupSuffix = program.get("--suffix");

        if (m_BackupSuffix.empty())
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: backup suffix must not be empty\n",
                    m_CommandName));
            m_HadError = true;
            return false;
        }
    }

    if (m_NoTargetDirectory &&
        program.is_used("--target-directory"))
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: -t and -T are mutually exclusive\n",
                m_CommandName));
        m_HadError = true;
        return false;
    }

    const std::vector<std::string> operands =
        program.is_used("operands")
            ? program.get<std::vector<std::string>>("operands")
            : std::vector<std::string>();
    const std::optional<std::string> targetDirectory =
        program.is_used("--target-directory")
            ? std::optional<std::string>(
                program.get("--target-directory"))
            : std::nullopt;

    return ConfigureOperands(
        operands,
        targetDirectory,
        sourcePaths,
        destinationPath);
}

bool CmdCp::ParsePreserveAttributes(
    const std::string& attributeList,
    bool preserve)
{
    if (attributeList.empty())
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: empty attribute list\n",
                m_CommandName));
        m_HadError = true;
        return false;
    }

    size_t attributeStart = 0;

    while (attributeStart <= attributeList.size())
    {
        const size_t separator = attributeList.find(',', attributeStart);
        const size_t attributeEnd =
            (separator == std::string::npos)
                ? attributeList.size()
                : separator;
        const std::string_view attribute(
            attributeList.data() + attributeStart,
            attributeEnd - attributeStart);
        uint32_t attributeMask = PreserveAttribute_None;

        if (attribute == "mode")
        {
            attributeMask = PreserveAttribute_Mode;
        }
        else if (attribute == "ownership")
        {
            attributeMask = PreserveAttribute_Ownership;
        }
        else if (attribute == "timestamps")
        {
            attributeMask = PreserveAttribute_Timestamps;
        }
        else if (attribute == "all")
        {
            attributeMask = PreserveAttribute_All;
        }
        else
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: unsupported attribute '{}'; supported attributes are mode, ownership, timestamps, all\n",
                    m_CommandName,
                    attribute));
            m_HadError = true;
            return false;
        }

        if (preserve) {
            m_PreserveAttributes |= attributeMask;
        } else {
            m_PreserveAttributes &= ~attributeMask;
        }

        if (separator == std::string::npos) {
            break;
        }
        attributeStart = separator + 1;
    }
    return true;
}

bool CmdCp::ParseBackupMode(const std::string& control)
{
    if (control == "none" || control == "off")
    {
        m_BackupMode = BackupMode::None;
    }
    else if (control == "numbered" || control == "t")
    {
        m_BackupMode = BackupMode::Numbered;
    }
    else if (control == "existing" || control == "nil")
    {
        m_BackupMode = BackupMode::Existing;
    }
    else if (control == "simple" || control == "never")
    {
        m_BackupMode = BackupMode::Simple;
    }
    else
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: invalid backup control '{}'\n",
                m_CommandName,
                control));
        m_HadError = true;
        return false;
    }
    return true;
}

bool CmdCp::ConfigureOperands(
    const std::vector<std::string>& operands,
    const std::optional<std::string>& targetDirectory,
    std::vector<PString>& sourcePaths,
    PString& destinationPath)
{
    if (targetDirectory.has_value())
    {
        if (operands.empty())
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: missing file operand\n",
                    m_CommandName));
            m_HadError = true;
            return false;
        }
        destinationPath = targetDirectory.value();

        for (const std::string& operand : operands) {
            sourcePaths.emplace_back(operand);
        }
    }
    else
    {
        if (operands.size() < 2)
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: missing destination file operand\n",
                    m_CommandName));
            m_HadError = true;
            return false;
        }

        destinationPath = operands.back();

        for (size_t index = 0; index + 1 < operands.size(); ++index) {
            sourcePaths.emplace_back(operands[index]);
        }
    }

    if (m_StripTrailingSlashes)
    {
        for (PString& sourcePath : sourcePaths) {
            sourcePath = shutil::TrimTrailingSlashes(sourcePath);
        }
    }
    if (m_Parents)
    {
        for (const PString& sourcePath : sourcePaths)
        {
            if (!IsSafeParentsPath(sourcePath))
            {
                shutil::WriteAll(
                    STDERR_FILENO,
                    PString::format_string(
                        "{}: with --parents, source path '{}' must not contain '..'\n",
                        m_CommandName,
                        sourcePath));
                m_HadError = true;
                return false;
            }
        }
    }
    return true;
}

bool CmdCp::CopyCommandLinePath(
    const PString& sourcePath,
    const PString& destinationPath)
{
    stat_t sourceStat;

    if (!shutil::ReadNodeStat(sourcePath, sourceStat, false))
    {
        ReportError("cannot stat", sourcePath, errno);
        return false;
    }

    if (S_ISLNK(sourceStat.st_mode) &&
        m_DereferenceMode != DereferenceMode::Never &&
        !shutil::ReadNodeStat(sourcePath, sourceStat, true))
    {
        ReportError("cannot dereference", sourcePath, errno);
        return false;
    }

    if (m_Parents && !EnsureParentDirectories(destinationPath)) {
        return false;
    }

    return CopyPath(
        sourcePath,
        destinationPath,
        true,
        sourceStat.st_dev);
}

bool CmdCp::CopyPath(
    const PString& sourcePath,
    const PString& destinationPath,
    bool commandLineOperand,
    dev_t traversalDevice)
{
    if (m_SymbolicLink) {
        return CreateSymbolicLink(sourcePath, destinationPath);
    }

    stat_t sourceStat;

    if (!shutil::ReadNodeStat(sourcePath, sourceStat, false))
    {
        ReportError("cannot stat", sourcePath, errno);
        return false;
    }

    const bool followSymlink =
        S_ISLNK(sourceStat.st_mode) &&
        (m_DereferenceMode == DereferenceMode::Always ||
         (m_DereferenceMode == DereferenceMode::CommandLineOnly &&
          commandLineOperand));

    if (followSymlink &&
        !shutil::ReadNodeStat(sourcePath, sourceStat, true))
    {
        ReportError("cannot dereference", sourcePath, errno);
        return false;
    }

    if (S_ISDIR(sourceStat.st_mode))
    {
        if (!m_Recursive)
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: -r not specified; omitting directory '{}'\n",
                    m_CommandName,
                    sourcePath));
            m_HadError = true;
            return false;
        }

        for (const auto& directoryIdentity : m_DirectoryStack)
        {
            if (directoryIdentity.first == sourceStat.st_dev &&
                directoryIdentity.second == sourceStat.st_ino)
            {
                shutil::WriteAll(
                    STDERR_FILENO,
                    PString::format_string(
                        "{}: cannot copy cyclic symbolic link '{}'\n",
                        m_CommandName,
                        sourcePath));
                m_HadError = true;
                return false;
            }
        }

        m_DirectoryStack.emplace_back(
            sourceStat.st_dev,
            sourceStat.st_ino);
        const bool result = CopyDirectory(
            sourcePath,
            destinationPath,
            sourceStat,
            followSymlink,
            traversalDevice);
        m_DirectoryStack.pop_back();
        return result;
    }
    if (S_ISREG(sourceStat.st_mode)) {
        return CopyRegularFile(sourcePath, destinationPath, sourceStat);
    }
    if (S_ISLNK(sourceStat.st_mode)) {
        return CopySymlink(sourcePath, destinationPath, sourceStat);
    }

    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: unsupported file type '{}'\n",
            m_CommandName,
            sourcePath));
    m_HadError = true;
    return false;
}

bool CmdCp::CopyRegularFile(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat)
{
    stat_t destinationStat;
    const bool destinationExists =
        shutil::ReadNodeStat(destinationPath, destinationStat, false);

    if (destinationExists)
    {
        if (S_ISDIR(destinationStat.st_mode))
        {
            ReportError(
                "cannot overwrite directory",
                destinationPath,
                EISDIR);
            return false;
        }
        if (destinationStat.st_dev == sourceStat.st_dev &&
            destinationStat.st_ino == sourceStat.st_ino)
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: '{}' and '{}' are the same file\n",
                    m_CommandName,
                    sourcePath,
                    destinationPath));
            m_HadError = true;
            return false;
        }
        if (m_Update && !IsSourceNewer(sourceStat, destinationStat)) {
            return true;
        }

        bool skipped = false;

        if (!PrepareDestination(
            sourcePath,
            destinationPath,
            sourceStat,
            destinationStat,
            m_RemoveDestination ||
                m_BackupMode != BackupMode::None,
            skipped)) {
            return skipped;
        }
    }

    const int sourceFile = m_AttributesOnly
        ? -1
        : open(sourcePath.c_str(), O_RDONLY);

    if (!m_AttributesOnly && sourceFile == -1)
    {
        ReportError("cannot open", sourcePath, errno);
        return false;
    }

    int openFlags = O_WRONLY | O_CREAT;

    if (!m_AttributesOnly) {
        openFlags |= O_TRUNC;
    }

    int destinationFile = open(
        destinationPath.c_str(),
        openFlags,
        sourceStat.st_mode & 0777);

    if (destinationFile == -1 &&
        m_Force &&
        destinationExists &&
        !m_RemoveDestination &&
        m_BackupMode == BackupMode::None)
    {
        if (unlink(destinationPath.c_str()) == 0)
        {
            destinationFile = open(
                destinationPath.c_str(),
                openFlags,
                sourceStat.st_mode & 0777);
        }
    }

    if (destinationFile == -1)
    {
        const int errorCode = errno;

        if (sourceFile != -1) {
            close(sourceFile);
        }
        ReportError("cannot create", destinationPath, errorCode);
        return false;
    }

    bool success = true;

    if (!m_AttributesOnly)
    {
        success = WriteFileContents(
            sourceFile,
            destinationFile,
            sourcePath,
            destinationPath);
    }

    int closeSourceResult = 0;
    int closeSourceError = 0;

    if (sourceFile != -1)
    {
        closeSourceResult = close(sourceFile);
        closeSourceError = errno;
    }

    const int closeDestinationResult = close(destinationFile);
    const int closeDestinationError = errno;

    if (closeSourceResult != 0)
    {
        ReportError("cannot close", sourcePath, closeSourceError);
        return false;
    }
    if (closeDestinationResult != 0)
    {
        ReportError(
            "cannot close",
            destinationPath,
            closeDestinationError);
        return false;
    }
    if (!success) {
        return false;
    }
    if (!ApplyAttributes(destinationPath, sourceStat, true)) {
        return false;
    }

    PrintCopied(sourcePath, destinationPath);
    return true;
}

bool CmdCp::CopyDirectory(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat,
    bool followSourceSymlink,
    dev_t traversalDevice)
{
    if (shutil::IsPathWithin(sourcePath, destinationPath))
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: cannot copy directory '{}' into itself, '{}'\n",
                m_CommandName,
                sourcePath,
                destinationPath));
        m_HadError = true;
        return false;
    }

    stat_t destinationStat;
    const bool destinationExists =
        shutil::ReadNodeStat(destinationPath, destinationStat, false);

    if (destinationExists && !S_ISDIR(destinationStat.st_mode))
    {
        ReportError(
            "cannot overwrite non-directory with directory",
            destinationPath,
            ENOTDIR);
        return false;
    }
    if (!destinationExists &&
        mkdir(destinationPath.c_str(), sourceStat.st_mode & 0777) != 0)
    {
        ReportError("cannot create directory", destinationPath, errno);
        return false;
    }

    bool success = true;

    if (!m_OneFileSystem || sourceStat.st_dev == traversalDevice)
    {
        std::vector<PString> entries;
        int errorCode = 0;

        if (!shutil::ReadDirectoryEntries(
            sourcePath,
            entries,
            errorCode,
            followSourceSymlink))
        {
            ReportError("cannot read directory", sourcePath, errorCode);
            return false;
        }

        for (const PString& entry : entries)
        {
            if (!CopyPath(
                shutil::MakeChildPath(sourcePath, entry),
                shutil::MakeChildPath(destinationPath, entry),
                false,
                traversalDevice)) {
                success = false;
            }
        }
    }

    if (!ApplyAttributes(destinationPath, sourceStat, true)) {
        success = false;
    }
    if (success) {
        PrintCopied(sourcePath, destinationPath);
    }
    return success;
}

bool CmdCp::CopySymlink(
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

    stat_t destinationStat;

    if (shutil::ReadNodeStat(destinationPath, destinationStat, false))
    {
        bool skipped = false;

        if (!PrepareDestination(
            sourcePath,
            destinationPath,
            sourceStat,
            destinationStat,
            true,
            skipped)) {
            return skipped;
        }
    }
    if (symlink(target.c_str(), destinationPath.c_str()) != 0)
    {
        ReportError(
            "cannot create symbolic link",
            destinationPath,
            errno);
        return false;
    }
    if (!ApplyAttributes(destinationPath, sourceStat, false)) {
        return false;
    }

    PrintCopied(sourcePath, destinationPath);
    return true;
}

bool CmdCp::CreateSymbolicLink(
    const PString& sourcePath,
    const PString& destinationPath)
{
    PString destinationParent =
        shutil::GetParentPath(destinationPath);

    if (destinationParent.empty()) {
        destinationParent = ".";
    }
    if ((sourcePath.empty() || sourcePath.front() != '/') &&
        shutil::GetAbsolutePath(destinationParent) !=
            shutil::GetAbsolutePath("."))
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: relative source '{}' cannot be used with -s outside the current directory\n",
                m_CommandName,
                sourcePath));
        m_HadError = true;
        return false;
    }

    stat_t sourceStat;

    if (!shutil::ReadNodeStat(sourcePath, sourceStat, false))
    {
        ReportError("cannot stat", sourcePath, errno);
        return false;
    }

    stat_t destinationStat;

    if (shutil::ReadNodeStat(destinationPath, destinationStat, false))
    {
        bool skipped = false;

        if (!PrepareDestination(
            sourcePath,
            destinationPath,
            sourceStat,
            destinationStat,
            true,
            skipped)) {
            return skipped;
        }
    }

    if (symlink(sourcePath.c_str(), destinationPath.c_str()) != 0)
    {
        ReportError(
            "cannot create symbolic link",
            destinationPath,
            errno);
        return false;
    }

    PrintCopied(sourcePath, destinationPath);
    return true;
}

bool CmdCp::PrepareDestination(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat,
    const stat_t& destinationStat,
    bool removeDestination,
    bool& outSkipped)
{
    outSkipped = false;

    if (sourceStat.st_dev == destinationStat.st_dev &&
        sourceStat.st_ino == destinationStat.st_ino)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: '{}' and '{}' are the same file\n",
                m_CommandName,
                sourcePath,
                destinationPath));
        m_HadError = true;
        return false;
    }
    if (m_NoClobber)
    {
        outSkipped = true;
        return false;
    }
    if (m_Update && !IsSourceNewer(sourceStat, destinationStat))
    {
        outSkipped = true;
        return false;
    }
    if (m_Interactive &&
        !shutil::Confirm(PString::format_string(
            "{}: overwrite '{}'? ",
            m_CommandName,
            destinationPath)))
    {
        outSkipped = true;
        return false;
    }
    if (!removeDestination) {
        return true;
    }
    if (S_ISDIR(destinationStat.st_mode))
    {
        ReportCopyError(sourcePath, destinationPath, EISDIR);
        return false;
    }
    if (m_BackupMode != BackupMode::None)
    {
        return BackupDestination(
            sourcePath,
            destinationPath,
            sourceStat);
    }
    return RemoveDestination(destinationPath);
}

bool CmdCp::RemoveDestination(const PString& destinationPath)
{
    if (unlink(destinationPath.c_str()) == 0) {
        return true;
    }

    ReportError("cannot remove", destinationPath, errno);
    return false;
}

bool CmdCp::BackupDestination(
    const PString& sourcePath,
    const PString& destinationPath,
    const stat_t& sourceStat)
{
    const PString backupPath = GetBackupPath(destinationPath);
    stat_t backupStat;

    if (shutil::ReadNodeStat(backupPath, backupStat, false))
    {
        if (sourceStat.st_dev == backupStat.st_dev &&
            sourceStat.st_ino == backupStat.st_ino)
        {
            shutil::WriteAll(
                STDERR_FILENO,
                PString::format_string(
                    "{}: backing up '{}' would destroy source '{}'\n",
                    m_CommandName,
                    destinationPath,
                    sourcePath));
            m_HadError = true;
            return false;
        }
        if (S_ISDIR(backupStat.st_mode))
        {
            ReportError(
                "cannot replace backup directory",
                backupPath,
                EISDIR);
            return false;
        }
        if (unlink(backupPath.c_str()) != 0)
        {
            ReportError("cannot remove backup", backupPath, errno);
            return false;
        }
    }
    if (rename(destinationPath.c_str(), backupPath.c_str()) != 0)
    {
        ReportError("cannot create backup", backupPath, errno);
        return false;
    }
    return true;
}

PString CmdCp::GetBackupPath(const PString& destinationPath) const
{
    switch (m_BackupMode)
    {
        case BackupMode::Numbered:
            return GetNumberedBackupPath(destinationPath);

        case BackupMode::Existing:
            if (GetHighestBackupNumber(destinationPath) != 0) {
                return GetNumberedBackupPath(destinationPath);
            }
            return destinationPath + m_BackupSuffix;

        case BackupMode::Simple:
            return destinationPath + m_BackupSuffix;

        case BackupMode::None:
            return destinationPath;
    }
    return destinationPath;
}

size_t CmdCp::GetHighestBackupNumber(
    const PString& destinationPath) const
{
    PString directoryPath = shutil::GetParentPath(destinationPath);

    if (directoryPath.empty()) {
        directoryPath = ".";
    }

    const PString prefix =
        shutil::GetBaseName(destinationPath) + ".~";
    std::vector<PString> entries;
    int errorCode = 0;

    if (!shutil::ReadDirectoryEntries(
        directoryPath,
        entries,
        errorCode,
        true)) {
        return 0;
    }

    size_t highestNumber = 0;

    for (const PString& entry : entries)
    {
        if (entry.size() <= prefix.size() + 1 ||
            entry.compare(0, prefix.size(), prefix) != 0 ||
            entry.back() != '~') {
            continue;
        }

        size_t number = 0;
        bool valid = true;

        for (size_t index = prefix.size();
             index + 1 < entry.size();
             ++index)
        {
            const char character = entry[index];

            if (character < '0' || character > '9')
            {
                valid = false;
                break;
            }
            if (number >
                (std::numeric_limits<size_t>::max() -
                 static_cast<size_t>(character - '0')) / 10)
            {
                valid = false;
                break;
            }
            number = number * 10 +
                     static_cast<size_t>(character - '0');
        }

        if (valid && number > highestNumber) {
            highestNumber = number;
        }
    }
    return highestNumber;
}

PString CmdCp::GetNumberedBackupPath(
    const PString& destinationPath) const
{
    return destinationPath +
           PString::format_string(
               ".~{}~",
               GetHighestBackupNumber(destinationPath) + 1);
}

bool CmdCp::ApplyAttributes(
    const PString& destinationPath,
    const stat_t& sourceStat,
    bool followDestinationSymlink)
{
    if (m_PreserveAttributes == PreserveAttribute_None) {
        return true;
    }

    const int fileDescriptor = open(
        destinationPath.c_str(),
        O_PATH | (followDestinationSymlink ? 0 : O_NOFOLLOW));

    if (fileDescriptor == -1)
    {
        ReportError(
            "cannot open destination attributes",
            destinationPath,
            errno);
        return false;
    }

    PFSNode destinationNode(fileDescriptor, true);
    uint32_t statMask = 0;

    if ((m_PreserveAttributes & PreserveAttribute_Mode) != 0) {
        statMask |= WSTAT_MODE;
    }
    if ((m_PreserveAttributes & PreserveAttribute_Ownership) != 0) {
        statMask |= WSTAT_UID | WSTAT_GID;
    }
    if ((m_PreserveAttributes & PreserveAttribute_Timestamps) != 0) {
        statMask |= WSTAT_ATIME | WSTAT_MTIME;
    }

    if (!destinationNode.SetStats(sourceStat, statMask))
    {
        ReportError(
            "cannot preserve attributes of",
            destinationPath,
            errno);
        return false;
    }
    return true;
}

bool CmdCp::EnsureParentDirectories(const PString& path)
{
    const PString parentPath = shutil::GetParentPath(path);

    if (parentPath.empty() || parentPath == "/") {
        return true;
    }
    if (shutil::IsDirectory(parentPath, true)) {
        return true;
    }
    if (!EnsureParentDirectories(parentPath)) {
        return false;
    }
    if (mkdir(parentPath.c_str(), 0777) == 0) {
        return true;
    }
    if (errno == EEXIST && shutil::IsDirectory(parentPath, true)) {
        return true;
    }

    ReportError("cannot create directory", parentPath, errno);
    return false;
}

PString CmdCp::BuildParentsDestination(
    const PString& sourcePath,
    const PString& destinationDirectory) const
{
    return shutil::MakeChildPath(
        destinationDirectory,
        GetParentsRelativePath(sourcePath));
}

PString CmdCp::GetParentsRelativePath(const PString& sourcePath)
{
    PString path = shutil::TrimTrailingSlashes(sourcePath);

    while (!path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }
    while (path.size() >= 2 &&
           path[0] == '.' &&
           path[1] == '/')
    {
        path.erase(0, 2);
    }
    return path;
}

bool CmdCp::IsSafeParentsPath(const PString& sourcePath)
{
    const PString path = GetParentsRelativePath(sourcePath);
    size_t componentStart = 0;

    while (componentStart <= path.size())
    {
        const size_t separator = path.find('/', componentStart);
        const size_t componentEnd =
            (separator == PString::npos) ? path.size() : separator;

        if (componentEnd - componentStart == 2 &&
            path[componentStart] == '.' &&
            path[componentStart + 1] == '.') {
            return false;
        }
        if (separator == PString::npos) {
            break;
        }
        componentStart = separator + 1;
    }
    return !path.empty();
}

bool CmdCp::IsSourceNewer(
    const stat_t& sourceStat,
    const stat_t& destinationStat)
{
    if (sourceStat.st_mtim.tv_sec != destinationStat.st_mtim.tv_sec) {
        return sourceStat.st_mtim.tv_sec > destinationStat.st_mtim.tv_sec;
    }
    return sourceStat.st_mtim.tv_nsec >
           destinationStat.st_mtim.tv_nsec;
}

bool CmdCp::WriteFileContents(
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

void CmdCp::PrintCopied(
    const PString& sourcePath,
    const PString& destinationPath) const
{
    if (m_Verbose)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            PString::format_string(
                "'{}' -> '{}'\n",
                sourcePath,
                destinationPath));
    }
}

void CmdCp::ReportError(
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

void CmdCp::ReportCopyError(
    const PString& sourcePath,
    const PString& destinationPath,
    int errorCode)
{
    m_HadError = true;
    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: cannot copy '{}' to '{}': {}\n",
            m_CommandName,
            sourcePath,
            destinationPath,
            strerror(errorCode)));
}

int cp_main(int argc, char* argv[])
{
    CmdCp command;
    return command.Run(argc, argv);
}

static PAppDefinition g_CpAppDef(
    "cp",
    "Copy files and directories.",
    cp_main);

} // namespace shutil_cp
