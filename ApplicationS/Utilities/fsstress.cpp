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
// Created: 30.08.2026 22:00

#include "fsstress.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <set>
#include <string_view>
#include <unistd.h>
#include <vector>

#include <sys/stat.h>

#include <argparse/argparse.hpp>

#include <System/AppDefinition.h>

#include "FileUtilityHelpers.h"


namespace shutil_fsstress
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int CmdFSStress::Run(int argc, char* argv[])
{
    m_CommandName = argv[0];

    const ParseArgumentsResult parseResult = ParseArguments(argc, argv);

    if (parseResult == ParseArgumentsResult::Help) {
        return 0;
    }
    if (parseResult == ParseArgumentsResult::Error) {
        return 1;
    }
    if (!PrepareWorkDirectory()) {
        return 1;
    }

    shutil::WriteAll(
        STDOUT_FILENO,
        PString::format_string(
            "{}: work directory: {}\n",
            m_CommandName,
            m_WorkDirectory));

    bool runSucceeded = false;
    bool cleanupSucceeded = true;

    try
    {
        runSucceeded = RunSelectedModes();
    }
    catch (const std::exception& exception)
    {
        ReportFailure(PString::format_string(
            "test aborted by exception: {}",
            exception.what()));
    }

    if (m_Keep)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            PString::format_string(
                "{}: keeping test data in {}\n",
                m_CommandName,
                m_WorkDirectory));
    }
    else
    {
        cleanupSucceeded = RemoveTree(m_WorkDirectory);
    }

    return (runSucceeded && cleanupSucceeded && !m_HadError) ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ParseArgumentsResult CmdFSStress::ParseArguments(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description(
        "Stress and benchmark filesystems, with FAT-focused corner-case tests.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("-m", "--mode")
        .help("Run all, io, metadata, fat, or capacity tests (default: all).")
        .metavar("MODE")
        .choices("all", "io", "metadata", "fat", "capacity");
    program.add_argument("-d", "--folder")
        .help("Create the private .fsstress-work directory under PATH (default: .).")
        .metavar("PATH");
    program.add_argument("--file-sizes")
        .help("Comma-separated file sizes; suffixes B, K, M, and G are accepted (default: 64K,1M).")
        .metavar("SIZES");
    program.add_argument("--section-sizes")
        .help("Comma-separated I/O section sizes (default: 512,4K,32K).")
        .metavar("SIZES");
    program.add_argument("--files")
        .help("Files per metadata/FAT/capacity iteration (default: 128).")
        .metavar("COUNT")
        .scan<'u', size_t>();
    program.add_argument("--directories")
        .help("Directories per metadata iteration (default: 16).")
        .metavar("COUNT")
        .scan<'u', size_t>();
    program.add_argument("--directory-scans")
        .help("Directory enumerations per metadata phase (default: 10).")
        .metavar("COUNT")
        .scan<'u', size_t>();
    program.add_argument("--iterations")
        .help("Repeat each selected workload COUNT times (default: 1).")
        .metavar("COUNT")
        .scan<'u', size_t>();
    program.add_argument("--max-total-size")
        .help("Capacity-mode byte limit; 0 disables it (default: 64M).")
        .metavar("SIZE");
    program.add_argument("--seed")
        .help("Data-pattern seed (default: 1179865940).")
        .metavar("NUMBER")
        .scan<'u', uint32_t>();
    program.add_argument("--sync")
        .help("Call fsync() for created/written files and include it in write measurements.")
        .flag();
    program.add_argument("--no-verify")
        .help("Skip data and directory-content verification passes.")
        .flag();
    program.add_argument("--keep")
        .help("Keep the private work directory after the run.")
        .flag();
    program.add_argument("-v", "--verbose")
        .help("Print individual FAT corner-case test names.")
        .flag();

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
        return ParseArgumentsResult::Error;
    }

    if (program.get<bool>("--help"))
    {
        shutil::WriteAll(STDOUT_FILENO, program.help().str());
        return ParseArgumentsResult::Help;
    }

    if (program.is_used("--mode"))
    {
        const std::string& modeText = program.get("--mode");

        if (!ParseMode(modeText, m_Mode))
        {
            ReportFailure(PString::format_string(
                "invalid mode '{}'",
                modeText));
            return ParseArgumentsResult::Error;
        }
    }
    if (program.is_used("--folder")) {
        m_BaseDirectory = program.get("--folder");
    }
    if (program.is_used("--file-sizes"))
    {
        const std::string& sizeText = program.get("--file-sizes");

        if (!ParseSizeList(sizeText, m_FileSizes))
        {
            ReportFailure(PString::format_string(
                "invalid file-size list '{}'",
                sizeText));
            return ParseArgumentsResult::Error;
        }
    }
    if (program.is_used("--section-sizes"))
    {
        const std::string& sizeText = program.get("--section-sizes");

        if (!ParseSizeList(sizeText, m_SectionSizes))
        {
            ReportFailure(PString::format_string(
                "invalid section-size list '{}'",
                sizeText));
            return ParseArgumentsResult::Error;
        }
    }
    if (program.is_used("--files")) {
        m_FileCount = program.get<size_t>("--files");
    }
    if (program.is_used("--directories")) {
        m_DirectoryCount = program.get<size_t>("--directories");
    }
    if (program.is_used("--directory-scans")) {
        m_DirectoryScans = program.get<size_t>("--directory-scans");
    }
    if (program.is_used("--iterations")) {
        m_Iterations = program.get<size_t>("--iterations");
    }
    if (program.is_used("--max-total-size"))
    {
        const std::string& sizeText = program.get("--max-total-size");

        if (!ParseSize(sizeText, m_MaxTotalSize))
        {
            ReportFailure(PString::format_string(
                "invalid maximum total size '{}'",
                sizeText));
            return ParseArgumentsResult::Error;
        }
    }
    if (program.is_used("--seed")) {
        m_Seed = program.get<uint32_t>("--seed");
    }

    m_Sync = program.get<bool>("--sync");
    m_Verify = !program.get<bool>("--no-verify");
    m_Keep = program.get<bool>("--keep");
    m_Verbose = program.get<bool>("--verbose");

    if (m_FileCount == 0 || m_DirectoryCount == 0 ||
        m_DirectoryScans == 0 || m_Iterations == 0)
    {
        ReportFailure(
            "--files, --directories, --directory-scans, and --iterations must be greater than zero");
        return ParseArgumentsResult::Error;
    }

    for (uint64_t sectionSize : m_SectionSizes)
    {
        if (sectionSize > std::numeric_limits<size_t>::max())
        {
            ReportFailure(PString::format_string(
                "section size {} does not fit in memory address space",
                sectionSize));
            return ParseArgumentsResult::Error;
        }
    }

    return ParseArgumentsResult::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::PrepareWorkDirectory()
{
    if (!shutil::IsDirectory(m_BaseDirectory, true))
    {
        ReportError("cannot use base directory", m_BaseDirectory, errno);
        return false;
    }

    m_WorkDirectory = shutil::MakeChildPath(
        m_BaseDirectory,
        ".fsstress-work");

    if (mkdir(m_WorkDirectory.c_str(), 0777) != 0)
    {
        ReportError("cannot create work directory", m_WorkDirectory, errno);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunSelectedModes()
{
    if ((m_Mode == StressMode::All || m_Mode == StressMode::IO) &&
        !RunIOMode()) {
        return false;
    }
    if ((m_Mode == StressMode::All || m_Mode == StressMode::Metadata) &&
        !RunMetadataMode()) {
        return false;
    }
    if ((m_Mode == StressMode::All || m_Mode == StressMode::FAT) &&
        !RunFATMode()) {
        return false;
    }
    if (m_Mode == StressMode::Capacity && !RunCapacityMode()) {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunIOMode()
{
    shutil::WriteAll(STDOUT_FILENO, "I/O throughput:\n");

    for (size_t iteration = 0; iteration < m_Iterations; ++iteration)
    {
        for (uint64_t fileSize : m_FileSizes)
        {
            for (uint64_t sectionSize : m_SectionSizes)
            {
                if (!RunIOCase(
                    fileSize,
                    static_cast<size_t>(sectionSize),
                    iteration)) {
                    return false;
                }
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunIOCase(
    uint64_t fileSize,
    size_t sectionSize,
    size_t iteration)
{
    const PString path = shutil::MakeChildPath(
        m_WorkDirectory,
        PString::format_string(
            "io-{}-{}-{}.dat",
            iteration,
            fileSize,
            sectionSize));
    const uint32_t salt = m_Seed ^ static_cast<uint32_t>(fileSize) ^
                          static_cast<uint32_t>(sectionSize);
    double writeDuration = 0.0;

    if (!WritePatternFile(
        path,
        fileSize,
        sectionSize,
        salt,
        &writeDuration)) {
        return false;
    }

    PrintThroughput(
        "write",
        fileSize,
        writeDuration,
        fileSize,
        sectionSize);

    const int fileDescriptor = open(path.c_str(), O_RDONLY);

    if (fileDescriptor == -1)
    {
        ReportError("cannot open for reading", path, errno);
        return false;
    }

    std::vector<uint8_t> buffer(sectionSize);
    uint64_t bytesRead = 0;
    const auto start = std::chrono::steady_clock::now();

    while (bytesRead < fileSize)
    {
        const size_t requestSize = static_cast<size_t>(std::min<uint64_t>(
            sectionSize,
            fileSize - bytesRead));
        const ssize_t readResult = read(
            fileDescriptor,
            buffer.data(),
            requestSize);

        if (readResult < 0)
        {
            if (errno == EINTR) {
                continue;
            }
            const int errorCode = errno;
            close(fileDescriptor);
            ReportError("read failed", path, errorCode);
            return false;
        }
        if (readResult == 0)
        {
            close(fileDescriptor);
            ReportFailure(PString::format_string(
                "unexpected end of file while reading '{}'",
                path));
            return false;
        }
        bytesRead += static_cast<uint64_t>(readResult);
    }

    if (close(fileDescriptor) != 0)
    {
        ReportError("close failed", path, errno);
        return false;
    }

    const auto end = std::chrono::steady_clock::now();
    const double readDuration = ElapsedSeconds(start, end);

    PrintThroughput(
        "read ",
        fileSize,
        readDuration,
        fileSize,
        sectionSize);

    if (m_Verify &&
        !VerifyPatternFile(path, fileSize, sectionSize, salt)) {
        return false;
    }
    if (unlink(path.c_str()) != 0)
    {
        ReportError("cannot remove I/O test file", path, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunMetadataMode()
{
    shutil::WriteAll(STDOUT_FILENO, "Metadata operations:\n");

    for (size_t iteration = 0; iteration < m_Iterations; ++iteration)
    {
        if (!RunMetadataIteration(iteration)) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunMetadataIteration(size_t iteration)
{
    const PString iterationDirectory = shutil::MakeChildPath(
        m_WorkDirectory,
        PString::format_string("metadata-{}", iteration));
    const PString sourceDirectory = shutil::MakeChildPath(
        iterationDirectory,
        "dense source directory");
    const PString destinationDirectory = shutil::MakeChildPath(
        iterationDirectory,
        "dense destination directory");

    if (mkdir(iterationDirectory.c_str(), 0777) != 0 ||
        mkdir(sourceDirectory.c_str(), 0777) != 0 ||
        mkdir(destinationDirectory.c_str(), 0777) != 0)
    {
        ReportError("cannot create metadata directory", iterationDirectory, errno);
        return false;
    }

    std::vector<PString> directories;
    directories.reserve(m_DirectoryCount);
    auto start = std::chrono::steady_clock::now();

    for (size_t index = 0; index < m_DirectoryCount; ++index)
    {
        const PString path = shutil::MakeChildPath(
            iterationDirectory,
            MakeIndexedName("long directory entry ", index));

        if (mkdir(path.c_str(), 0777) != 0)
        {
            ReportError("mkdir failed", path, errno);
            return false;
        }
        directories.push_back(path);
    }

    auto end = std::chrono::steady_clock::now();
    PrintOperationRate(
        "mkdir",
        m_DirectoryCount,
        ElapsedSeconds(start, end));

    std::vector<PString> fileNames;
    fileNames.reserve(m_FileCount);
    start = std::chrono::steady_clock::now();

    for (size_t index = 0; index < m_FileCount; ++index)
    {
        const PString name = MakeIndexedName(
            "metadata collision file ",
            index,
            ".dat");
        const PString path = shutil::MakeChildPath(sourceDirectory, name);

        if (!CreateEmptyFile(path)) {
            return false;
        }
        fileNames.push_back(name);
    }

    end = std::chrono::steady_clock::now();
    PrintOperationRate(
        "create",
        m_FileCount,
        ElapsedSeconds(start, end));

    start = std::chrono::steady_clock::now();
    for (const PString& name : fileNames)
    {
        stat_t statBuffer;
        const PString path = shutil::MakeChildPath(sourceDirectory, name);

        if (!shutil::ReadNodeStat(path, statBuffer, false))
        {
            ReportError("stat failed", path, errno);
            return false;
        }
    }
    end = std::chrono::steady_clock::now();
    PrintOperationRate("stat  ", m_FileCount, ElapsedSeconds(start, end));

    start = std::chrono::steady_clock::now();
    for (size_t scan = 0; scan < m_DirectoryScans; ++scan)
    {
        std::vector<PString> entries;

        if (!EnumerateDirectory(sourceDirectory, entries)) {
            return false;
        }
        if (m_Verify && entries.size() != m_FileCount)
        {
            ReportFailure(PString::format_string(
                "directory '{}' returned {} entries; expected {}",
                sourceDirectory,
                entries.size(),
                m_FileCount));
            return false;
        }
    }
    end = std::chrono::steady_clock::now();
    PrintOperationRate(
        "readdir",
        m_DirectoryScans,
        ElapsedSeconds(start, end));

    start = std::chrono::steady_clock::now();
    for (size_t index = 0; index < fileNames.size(); ++index)
    {
        const PString oldPath = shutil::MakeChildPath(
            sourceDirectory,
            fileNames[index]);
        const PString newName = MakeIndexedName(
            "renamed metadata collision file ",
            index,
            ".dat");
        const PString newPath = shutil::MakeChildPath(
            destinationDirectory,
            newName);

        if (rename(oldPath.c_str(), newPath.c_str()) != 0)
        {
            ReportError("rename failed", oldPath, errno);
            return false;
        }
        fileNames[index] = newName;
    }
    end = std::chrono::steady_clock::now();
    PrintOperationRate("rename", m_FileCount, ElapsedSeconds(start, end));

    start = std::chrono::steady_clock::now();
    for (size_t scan = 0; scan < m_DirectoryScans; ++scan)
    {
        std::vector<PString> sourceEntries;
        std::vector<PString> destinationEntries;

        if (!EnumerateDirectory(sourceDirectory, sourceEntries) ||
            !EnumerateDirectory(destinationDirectory, destinationEntries)) {
            return false;
        }
        if (m_Verify &&
            (!sourceEntries.empty() || destinationEntries.size() != m_FileCount))
        {
            ReportFailure(PString::format_string(
                "renamed directory contents are inconsistent: source={}, destination={}, expected={}",
                sourceEntries.size(),
                destinationEntries.size(),
                m_FileCount));
            return false;
        }
    }
    end = std::chrono::steady_clock::now();
    PrintOperationRate(
        "readdir after rename",
        m_DirectoryScans * 2,
        ElapsedSeconds(start, end));

    start = std::chrono::steady_clock::now();
    for (const PString& name : fileNames)
    {
        const PString path = shutil::MakeChildPath(destinationDirectory, name);

        if (unlink(path.c_str()) != 0)
        {
            ReportError("unlink failed", path, errno);
            return false;
        }
    }
    end = std::chrono::steady_clock::now();
    PrintOperationRate("unlink", m_FileCount, ElapsedSeconds(start, end));

    start = std::chrono::steady_clock::now();
    for (const PString& path : directories)
    {
        if (rmdir(path.c_str()) != 0)
        {
            ReportError("rmdir failed", path, errno);
            return false;
        }
    }
    end = std::chrono::steady_clock::now();
    PrintOperationRate(
        "rmdir ",
        m_DirectoryCount,
        ElapsedSeconds(start, end));

    if (rmdir(sourceDirectory.c_str()) != 0 ||
        rmdir(destinationDirectory.c_str()) != 0 ||
        rmdir(iterationDirectory.c_str()) != 0)
    {
        ReportError("cannot remove metadata directory", iterationDirectory, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunFATMode()
{
    shutil::WriteAll(STDOUT_FILENO, "FAT corner cases:\n");

    for (size_t iteration = 0; iteration < m_Iterations; ++iteration)
    {
        if (!RunFATIteration(iteration)) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunFATIteration(size_t iteration)
{
    const PString directory = shutil::MakeChildPath(
        m_WorkDirectory,
        PString::format_string("fat-{}", iteration));

    if (mkdir(directory.c_str(), 0777) != 0)
    {
        ReportError("cannot create FAT test directory", directory, errno);
        return false;
    }

    if (!TestResizeAndSparseFile(directory) ||
        !TestDirectoryEntryReuse(directory) ||
        !TestDirectoryRename(directory) ||
        !TestOpenUnlink(directory)) {
        return false;
    }
    if (rmdir(directory.c_str()) != 0)
    {
        ReportError("cannot remove FAT test directory", directory, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::TestResizeAndSparseFile(const PString& directory)
{
    if (m_Verbose) {
        shutil::WriteAll(STDOUT_FILENO, "  resize, truncate, and sparse-gap zeroing\n");
    }

    const uint64_t fileSize = m_FileSizes.front();
    const size_t sectionSize = static_cast<size_t>(m_SectionSizes.front());
    const uint32_t salt = m_Seed ^ 0x5245535a;
    const PString path = shutil::MakeChildPath(
        directory,
        "resize and sparse gap.dat");

    if (!WritePatternFile(path, fileSize, sectionSize, salt)) {
        return false;
    }

    int fileDescriptor = open(path.c_str(), O_RDWR);

    if (fileDescriptor == -1)
    {
        ReportError("cannot open resize test file", path, errno);
        return false;
    }

    const uint64_t shrinkSize = fileSize / 2 + 1;

    if (ftruncate(fileDescriptor, static_cast<off_t>(shrinkSize)) != 0)
    {
        const int errorCode = errno;
        close(fileDescriptor);
        ReportError("truncate failed", path, errorCode);
        return false;
    }
    if (!SyncFile(fileDescriptor, path))
    {
        close(fileDescriptor);
        return false;
    }
    if (close(fileDescriptor) != 0)
    {
        ReportError("close failed", path, errno);
        return false;
    }
    if (m_Verify &&
        !VerifyPatternFile(path, shrinkSize, sectionSize, salt)) {
        return false;
    }

    fileDescriptor = open(path.c_str(), O_RDWR);
    if (fileDescriptor == -1)
    {
        ReportError("cannot reopen resize test file", path, errno);
        return false;
    }
    if (ftruncate(fileDescriptor, static_cast<off_t>(fileSize)) != 0)
    {
        const int errorCode = errno;
        close(fileDescriptor);
        ReportError("extend failed", path, errorCode);
        return false;
    }
    if (m_Verify &&
        !VerifyZeroRange(
            fileDescriptor,
            shrinkSize,
            fileSize - shrinkSize,
            sectionSize))
    {
        close(fileDescriptor);
        return false;
    }

    const uint64_t maximumFATFileSize = std::numeric_limits<uint32_t>::max();
    const uint64_t gapLength = static_cast<uint64_t>(sectionSize) + 17;

    if (fileSize + gapLength < maximumFATFileSize)
    {
        const uint64_t markerPosition = fileSize + gapLength;
        const uint8_t marker = 0xa5;

        if (lseek(fileDescriptor, static_cast<off_t>(markerPosition), SEEK_SET) < 0 ||
            write(fileDescriptor, &marker, sizeof(marker)) != sizeof(marker))
        {
            const int errorCode = errno;
            close(fileDescriptor);
            ReportError("sparse write failed", path, errorCode);
            return false;
        }
        if (m_Verify &&
            !VerifyZeroRange(
                fileDescriptor,
                fileSize,
                gapLength,
                sectionSize))
        {
            close(fileDescriptor);
            return false;
        }

        uint8_t readMarker = 0;
        if (lseek(fileDescriptor, static_cast<off_t>(markerPosition), SEEK_SET) < 0 ||
            read(fileDescriptor, &readMarker, sizeof(readMarker)) != sizeof(readMarker) ||
            readMarker != marker)
        {
            close(fileDescriptor);
            ReportFailure(PString::format_string(
                "sparse-write marker verification failed for '{}'",
                path));
            return false;
        }
    }
    else if (m_Verbose)
    {
        shutil::WriteAll(
            STDOUT_FILENO,
            "    sparse-gap test skipped because it would exceed the FAT file-size limit\n");
    }

    if (!SyncFile(fileDescriptor, path))
    {
        close(fileDescriptor);
        return false;
    }
    if (close(fileDescriptor) != 0)
    {
        ReportError("close failed", path, errno);
        return false;
    }
    if (unlink(path.c_str()) != 0)
    {
        ReportError("cannot remove resize test file", path, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::TestDirectoryEntryReuse(const PString& directory)
{
    if (m_Verbose) {
        shutil::WriteAll(STDOUT_FILENO, "  long-name collisions and directory-entry reuse\n");
    }

    const PString testDirectory = shutil::MakeChildPath(
        directory,
        "long filename collision directory");

    if (mkdir(testDirectory.c_str(), 0777) != 0)
    {
        ReportError("cannot create long-name test directory", testDirectory, errno);
        return false;
    }

    std::vector<PString> names;
    names.reserve(m_FileCount);

    for (size_t index = 0; index < m_FileCount; ++index)
    {
        const PString name = MakeLongName(index, 0);
        const PString path = shutil::MakeChildPath(testDirectory, name);

        if (!CreateEmptyFile(path)) {
            return false;
        }
        names.push_back(name);
    }

    for (size_t index = 0; index < names.size(); index += 2)
    {
        const PString path = shutil::MakeChildPath(
            testDirectory,
            names[index]);

        if (unlink(path.c_str()) != 0)
        {
            ReportError("cannot erase long-name entry", path, errno);
            return false;
        }

        names[index] = MakeLongName(index, 1);
        const PString replacementPath = shutil::MakeChildPath(
            testDirectory,
            names[index]);

        if (!CreateEmptyFile(replacementPath)) {
            return false;
        }
    }

    if (m_Verify)
    {
        std::vector<PString> entries;

        if (!EnumerateDirectory(testDirectory, entries)) {
            return false;
        }

        const std::set<PString> expected(names.begin(), names.end());
        const std::set<PString> actual(entries.begin(), entries.end());

        if (actual != expected)
        {
            ReportFailure(PString::format_string(
                "long-name directory '{}' did not enumerate the expected entries",
                testDirectory));
            return false;
        }
    }

    for (const PString& name : names)
    {
        const PString path = shutil::MakeChildPath(testDirectory, name);

        if (unlink(path.c_str()) != 0)
        {
            ReportError("cannot remove long-name test file", path, errno);
            return false;
        }
    }
    if (rmdir(testDirectory.c_str()) != 0)
    {
        ReportError("cannot remove long-name test directory", testDirectory, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::TestDirectoryRename(const PString& directory)
{
    if (m_Verbose) {
        shutil::WriteAll(STDOUT_FILENO, "  cross-directory rename and parent entry update\n");
    }

    const PString parentA = shutil::MakeChildPath(directory, "rename parent alpha");
    const PString parentB = shutil::MakeChildPath(directory, "rename parent beta");
    const PString childA = shutil::MakeChildPath(parentA, "child before move");
    const PString childB = shutil::MakeChildPath(parentB, "child after move with long name");
    const PString oldFile = shutil::MakeChildPath(childA, "payload before rename.dat");
    const PString newFile = shutil::MakeChildPath(childB, "payload after rename.dat");
    const size_t sectionSize = static_cast<size_t>(m_SectionSizes.front());
    const uint64_t fileSize = std::min<uint64_t>(m_FileSizes.front(), 64 * 1024);
    const uint32_t salt = m_Seed ^ 0x524e414d;

    if (mkdir(parentA.c_str(), 0777) != 0 ||
        mkdir(parentB.c_str(), 0777) != 0 ||
        mkdir(childA.c_str(), 0777) != 0)
    {
        ReportError("cannot create rename test directory", directory, errno);
        return false;
    }
    if (!WritePatternFile(oldFile, fileSize, sectionSize, salt)) {
        return false;
    }
    if (rename(childA.c_str(), childB.c_str()) != 0)
    {
        ReportError("cannot move test directory", childA, errno);
        return false;
    }
    if (rename(
        shutil::MakeChildPath(childB, "payload before rename.dat").c_str(),
        newFile.c_str()) != 0)
    {
        ReportError("cannot rename test payload", newFile, errno);
        return false;
    }

    if (m_Verify)
    {
        stat_t parentStat;
        stat_t dotDotStat;

        if (!shutil::ReadNodeStat(parentB, parentStat, false) ||
            !shutil::ReadNodeStat(
                shutil::MakeChildPath(childB, ".."),
                dotDotStat,
                false))
        {
            ReportError("cannot stat moved directory parent", childB, errno);
            return false;
        }
        if (parentStat.st_dev != dotDotStat.st_dev ||
            parentStat.st_ino != dotDotStat.st_ino)
        {
            ReportFailure(PString::format_string(
                "'..' was not updated after moving '{}'",
                childB));
            return false;
        }
        if (!VerifyPatternFile(newFile, fileSize, sectionSize, salt)) {
            return false;
        }
    }

    if (unlink(newFile.c_str()) != 0 ||
        rmdir(childB.c_str()) != 0 ||
        rmdir(parentA.c_str()) != 0 ||
        rmdir(parentB.c_str()) != 0)
    {
        ReportError("cannot clean rename test", directory, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::TestOpenUnlink(const PString& directory)
{
    if (m_Verbose) {
        shutil::WriteAll(STDOUT_FILENO, "  unlink of an open file\n");
    }

    const PString path = shutil::MakeChildPath(directory, "open then unlink.dat");
    const size_t sectionSize = static_cast<size_t>(m_SectionSizes.front());
    const uint64_t fileSize = std::min<uint64_t>(m_FileSizes.front(), 64 * 1024);
    const uint32_t salt = m_Seed ^ 0x554e4c4b;

    if (!WritePatternFile(path, fileSize, sectionSize, salt)) {
        return false;
    }

    const int fileDescriptor = open(path.c_str(), O_RDWR);

    if (fileDescriptor == -1)
    {
        ReportError("cannot open unlink test file", path, errno);
        return false;
    }
    if (unlink(path.c_str()) != 0)
    {
        const int errorCode = errno;
        close(fileDescriptor);
        ReportError("cannot unlink open file", path, errorCode);
        return false;
    }

    stat_t statBuffer;
    const bool pathVisible = shutil::ReadNodeStat(path, statBuffer, false);
    const int lookupError = errno;

    if (m_Verify && pathVisible)
    {
        close(fileDescriptor);
        ReportFailure(PString::format_string(
            "unlinked path '{}' is still visible",
            path));
        return false;
    }
    if (m_Verify && lookupError != ENOENT)
    {
        close(fileDescriptor);
        ReportError("cannot verify unlinked path", path, lookupError);
        return false;
    }

    const size_t readSize = static_cast<size_t>(std::min<uint64_t>(
        sectionSize,
        fileSize));
    std::vector<uint8_t> expected(sectionSize);
    std::vector<uint8_t> actual(readSize);
    FillPattern(expected, salt);

    size_t received = 0;
    if (lseek(fileDescriptor, 0, SEEK_SET) < 0)
    {
        close(fileDescriptor);
        ReportFailure(PString::format_string(
            "open file descriptor stopped working after unlinking '{}'",
            path));
        return false;
    }
    while (received < readSize)
    {
        const ssize_t readResult = read(
            fileDescriptor,
            actual.data() + received,
            readSize - received);

        if (readResult < 0 && errno == EINTR) {
            continue;
        }
        if (readResult <= 0)
        {
            close(fileDescriptor);
            ReportFailure(PString::format_string(
                "open file descriptor stopped working after unlinking '{}'",
                path));
            return false;
        }
        received += static_cast<size_t>(readResult);
    }
    if (m_Verify && !std::equal(actual.begin(), actual.end(), expected.begin()))
    {
        close(fileDescriptor);
        ReportFailure(PString::format_string(
            "open file data changed after unlinking '{}'",
            path));
        return false;
    }
    if (close(fileDescriptor) != 0)
    {
        ReportError("close failed", path, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunCapacityMode()
{
    shutil::WriteAll(
        STDOUT_FILENO,
        "Capacity stress (this mode can reach ENOSPC):\n");

    for (size_t iteration = 0; iteration < m_Iterations; ++iteration)
    {
        if (!RunCapacityIteration(iteration)) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RunCapacityIteration(size_t iteration)
{
    const PString directory = shutil::MakeChildPath(
        m_WorkDirectory,
        PString::format_string("capacity-{}", iteration));

    if (mkdir(directory.c_str(), 0777) != 0)
    {
        ReportError("cannot create capacity directory", directory, errno);
        return false;
    }

    const uint64_t nominalFileSize = m_FileSizes.back();
    const size_t sectionSize = static_cast<size_t>(m_SectionSizes.back());
    std::vector<uint8_t> buffer(sectionSize);
    FillPattern(buffer, m_Seed ^ 0x43415041);

    uint64_t totalBytes = 0;
    size_t filesCreated = 0;
    bool reachedFull = false;
    const auto start = std::chrono::steady_clock::now();

    for (size_t fileIndex = 0; fileIndex < m_FileCount; ++fileIndex)
    {
        if (m_MaxTotalSize != 0 && totalBytes >= m_MaxTotalSize) {
            break;
        }

        uint64_t targetSize = nominalFileSize;
        if (m_MaxTotalSize != 0) {
            targetSize = std::min(targetSize, m_MaxTotalSize - totalBytes);
        }

        const PString path = shutil::MakeChildPath(
            directory,
            MakeIndexedName("capacity file ", fileIndex, ".dat"));
        const int fileDescriptor = open(
            path.c_str(),
            O_WRONLY | O_CREAT | O_EXCL,
            0666);

        if (fileDescriptor == -1)
        {
            if (errno == ENOSPC)
            {
                reachedFull = true;
                break;
            }
            ReportError("cannot create capacity file", path, errno);
            return false;
        }

        uint64_t fileBytes = 0;
        while (fileBytes < targetSize)
        {
            const size_t requestSize = static_cast<size_t>(std::min<uint64_t>(
                sectionSize,
                targetSize - fileBytes));
            const ssize_t writeResult = write(
                fileDescriptor,
                buffer.data(),
                requestSize);

            if (writeResult < 0)
            {
                if (errno == EINTR) {
                    continue;
                }
                if (errno == ENOSPC)
                {
                    reachedFull = true;
                    break;
                }
                const int errorCode = errno;
                close(fileDescriptor);
                ReportError("capacity write failed", path, errorCode);
                return false;
            }
            if (writeResult == 0)
            {
                close(fileDescriptor);
                ReportFailure(PString::format_string(
                    "capacity write made no progress for '{}'",
                    path));
                return false;
            }
            fileBytes += static_cast<uint64_t>(writeResult);
            totalBytes += static_cast<uint64_t>(writeResult);
        }

        if (m_Sync && fsync(fileDescriptor) != 0)
        {
            if (errno == ENOSPC) {
                reachedFull = true;
            } else {
                const int errorCode = errno;
                close(fileDescriptor);
                ReportError("capacity fsync failed", path, errorCode);
                return false;
            }
        }
        if (close(fileDescriptor) != 0 && !reachedFull)
        {
            ReportError("capacity close failed", path, errno);
            return false;
        }

        ++filesCreated;
        if (reachedFull) {
            break;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const double duration = ElapsedSeconds(start, end);
    const double mebibytesPerSecond = (duration > 0.0)
        ? static_cast<double>(totalBytes) / (1024.0 * 1024.0) / duration
        : 0.0;

    shutil::WriteAll(
        STDOUT_FILENO,
        PString::format_string(
            "  wrote {} in {} files, {:.3f} s, {:.2f} MiB/s{}\n",
            FormatSize(totalBytes),
            filesCreated,
            duration,
            mebibytesPerSecond,
            reachedFull ? "; ENOSPC reached" : "; configured limit reached"));

    if (!m_Keep && !RemoveTree(directory)) {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::WritePatternFile(
    const PString& path,
    uint64_t fileSize,
    size_t sectionSize,
    uint32_t salt,
    double* outDuration)
{
    std::vector<uint8_t> buffer(sectionSize);
    FillPattern(buffer, salt);

    const int fileDescriptor = open(
        path.c_str(),
        O_WRONLY | O_CREAT | O_TRUNC,
        0666);

    if (fileDescriptor == -1)
    {
        ReportError("cannot create file", path, errno);
        return false;
    }

    uint64_t bytesWritten = 0;
    const auto start = std::chrono::steady_clock::now();

    while (bytesWritten < fileSize)
    {
        const size_t requestSize = static_cast<size_t>(std::min<uint64_t>(
            sectionSize,
            fileSize - bytesWritten));
        size_t sectionWritten = 0;

        while (sectionWritten < requestSize)
        {
            const ssize_t writeResult = write(
                fileDescriptor,
                buffer.data() + sectionWritten,
                requestSize - sectionWritten);

            if (writeResult < 0)
            {
                if (errno == EINTR) {
                    continue;
                }
                const int errorCode = errno;
                close(fileDescriptor);
                ReportError("write failed", path, errorCode);
                return false;
            }
            if (writeResult == 0)
            {
                close(fileDescriptor);
                ReportFailure(PString::format_string(
                    "write made no progress for '{}'",
                    path));
                return false;
            }
            sectionWritten += static_cast<size_t>(writeResult);
            bytesWritten += static_cast<uint64_t>(writeResult);
        }
    }

    if (!SyncFile(fileDescriptor, path))
    {
        close(fileDescriptor);
        return false;
    }
    if (close(fileDescriptor) != 0)
    {
        ReportError("close failed", path, errno);
        return false;
    }

    const auto end = std::chrono::steady_clock::now();
    if (outDuration != nullptr) {
        *outDuration = ElapsedSeconds(start, end);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::VerifyPatternFile(
    const PString& path,
    uint64_t fileSize,
    size_t sectionSize,
    uint32_t salt)
{
    const int fileDescriptor = open(path.c_str(), O_RDONLY);

    if (fileDescriptor == -1)
    {
        ReportError("cannot open file for verification", path, errno);
        return false;
    }

    stat_t statBuffer;
    if (fstat(fileDescriptor, &statBuffer) != 0)
    {
        const int errorCode = errno;
        close(fileDescriptor);
        ReportError("cannot stat file for verification", path, errorCode);
        return false;
    }
    if (static_cast<uint64_t>(statBuffer.st_size) != fileSize)
    {
        close(fileDescriptor);
        ReportFailure(PString::format_string(
            "file '{}' has size {}; expected {}",
            path,
            statBuffer.st_size,
            fileSize));
        return false;
    }

    std::vector<uint8_t> expected(sectionSize);
    std::vector<uint8_t> actual(sectionSize);
    FillPattern(expected, salt);

    uint64_t offset = 0;
    while (offset < fileSize)
    {
        const size_t requestSize = static_cast<size_t>(std::min<uint64_t>(
            sectionSize,
            fileSize - offset));
        size_t received = 0;

        while (received < requestSize)
        {
            const ssize_t readResult = read(
                fileDescriptor,
                actual.data() + received,
                requestSize - received);

            if (readResult < 0)
            {
                if (errno == EINTR) {
                    continue;
                }
                const int errorCode = errno;
                close(fileDescriptor);
                ReportError("verification read failed", path, errorCode);
                return false;
            }
            if (readResult == 0)
            {
                close(fileDescriptor);
                ReportFailure(PString::format_string(
                    "unexpected end of file while verifying '{}'",
                    path));
                return false;
            }
            received += static_cast<size_t>(readResult);
        }

        if (!std::equal(
            actual.begin(),
            actual.begin() + requestSize,
            expected.begin()))
        {
            close(fileDescriptor);
            ReportFailure(PString::format_string(
                "data mismatch in '{}' at offset {}",
                path,
                offset));
            return false;
        }
        offset += requestSize;
    }

    if (close(fileDescriptor) != 0)
    {
        ReportError("close failed", path, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::VerifyZeroRange(
    int fileDescriptor,
    uint64_t start,
    uint64_t length,
    size_t sectionSize)
{
    if (lseek(fileDescriptor, static_cast<off_t>(start), SEEK_SET) < 0)
    {
        ReportFailure(PString::format_string(
            "cannot seek to zero-filled range at offset {}",
            start));
        return false;
    }

    std::vector<uint8_t> buffer(sectionSize);
    uint64_t offset = 0;

    while (offset < length)
    {
        const size_t requestSize = static_cast<size_t>(std::min<uint64_t>(
            sectionSize,
            length - offset));
        const ssize_t readResult = read(
            fileDescriptor,
            buffer.data(),
            requestSize);

        if (readResult < 0)
        {
            if (errno == EINTR) {
                continue;
            }
            ReportFailure(PString::format_string(
                "cannot read zero-filled range at offset {}: {}",
                start + offset,
                strerror(errno)));
            return false;
        }
        if (readResult == 0)
        {
            ReportFailure(PString::format_string(
                "unexpected end of zero-filled range at offset {}",
                start + offset));
            return false;
        }

        for (ssize_t index = 0; index < readResult; ++index)
        {
            if (buffer[static_cast<size_t>(index)] != 0)
            {
                ReportFailure(PString::format_string(
                    "non-zero byte in zero-filled range at offset {}",
                    start + offset + static_cast<uint64_t>(index)));
                return false;
            }
        }
        offset += static_cast<uint64_t>(readResult);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::CreateEmptyFile(const PString& path)
{
    const int fileDescriptor = open(
        path.c_str(),
        O_WRONLY | O_CREAT | O_EXCL,
        0666);

    if (fileDescriptor == -1)
    {
        ReportError("cannot create file", path, errno);
        return false;
    }
    if (!SyncFile(fileDescriptor, path))
    {
        close(fileDescriptor);
        return false;
    }
    if (close(fileDescriptor) != 0)
    {
        ReportError("close failed", path, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::EnumerateDirectory(
    const PString& path,
    std::vector<PString>& entries)
{
    int errorCode = 0;
    if (!shutil::ReadDirectoryEntries(
        path,
        entries,
        errorCode,
        false))
    {
        ReportError("cannot enumerate directory", path, errorCode);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::SyncFile(int fileDescriptor, const PString& path)
{
    if (!m_Sync) {
        return true;
    }
    if (fsync(fileDescriptor) != 0)
    {
        ReportError("fsync failed", path, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::RemoveTree(const PString& path)
{
    stat_t statBuffer;
    if (!shutil::ReadNodeStat(path, statBuffer, false))
    {
        if (errno == ENOENT) {
            return true;
        }
        ReportError("cannot stat cleanup path", path, errno);
        return false;
    }

    if (!S_ISDIR(statBuffer.st_mode))
    {
        if (unlink(path.c_str()) != 0)
        {
            ReportError("cannot remove cleanup file", path, errno);
            return false;
        }
        return true;
    }

    std::vector<PString> entries;
    if (!EnumerateDirectory(path, entries)) {
        return false;
    }
    for (const PString& entry : entries)
    {
        if (!RemoveTree(shutil::MakeChildPath(path, entry))) {
            return false;
        }
    }
    if (rmdir(path.c_str()) != 0)
    {
        ReportError("cannot remove cleanup directory", path, errno);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::ParseSizeList(
    const std::string& text,
    std::vector<uint64_t>& sizes)
{
    std::vector<uint64_t> parsedSizes;
    size_t start = 0;

    while (start <= text.size())
    {
        const size_t separator = text.find(',', start);
        const std::string item = text.substr(
            start,
            (separator == std::string::npos)
                ? std::string::npos
                : separator - start);
        uint64_t size = 0;

        if (!ParseSize(item, size) || size == 0) {
            return false;
        }
        parsedSizes.push_back(size);

        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1;
    }

    if (parsedSizes.empty()) {
        return false;
    }
    sizes = std::move(parsedSizes);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::ParseSize(const std::string& text, uint64_t& size)
{
    if (text.empty()) {
        return false;
    }

    errno = 0;
    char* suffixStart = nullptr;
    const unsigned long long value = strtoull(
        text.c_str(),
        &suffixStart,
        10);

    if (errno == ERANGE || suffixStart == text.c_str()) {
        return false;
    }

    std::string suffix(suffixStart);
    std::transform(
        suffix.begin(),
        suffix.end(),
        suffix.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });

    uint64_t multiplier = 1;
    if (suffix.empty() || suffix == "b") {
        multiplier = 1;
    } else if (suffix == "k" || suffix == "kb" || suffix == "kib") {
        multiplier = 1024;
    } else if (suffix == "m" || suffix == "mb" || suffix == "mib") {
        multiplier = 1024 * 1024;
    } else if (suffix == "g" || suffix == "gb" || suffix == "gib") {
        multiplier = uint64_t(1024) * 1024 * 1024;
    } else {
        return false;
    }

    if (value > std::numeric_limits<uint64_t>::max() / multiplier) {
        return false;
    }
    size = static_cast<uint64_t>(value) * multiplier;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdFSStress::ParseMode(const std::string& text, StressMode& mode)
{
    if (text == "all") {
        mode = StressMode::All;
    } else if (text == "io") {
        mode = StressMode::IO;
    } else if (text == "metadata") {
        mode = StressMode::Metadata;
    } else if (text == "fat") {
        mode = StressMode::FAT;
    } else if (text == "capacity") {
        mode = StressMode::Capacity;
    } else {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFSStress::FillPattern(
    std::vector<uint8_t>& buffer,
    uint32_t salt)
{
    uint32_t value = salt;

    for (uint8_t& byte : buffer)
    {
        value ^= value << 13;
        value ^= value >> 17;
        value ^= value << 5;
        byte = static_cast<uint8_t>(value);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdFSStress::MakeIndexedName(
    const char* prefix,
    size_t index,
    const char* suffix)
{
    return PString::format_string(
        "{}{:06}{}",
        prefix,
        index,
        suffix);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdFSStress::MakeLongName(size_t index, size_t generation)
{
    PString name = PString::format_string(
        "fat collision candidate generation {} index {:06} ",
        generation,
        index);
    name.append(170, static_cast<char>('a' + generation % 26));
    name += ".dat";
    return name;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdFSStress::FormatSize(uint64_t bytes)
{
    static constexpr const char* unitNames[] = {
        "B", "KiB", "MiB", "GiB", "TiB"
    };

    double value = static_cast<double>(bytes);
    size_t unitIndex = 0;
    while (value >= 1024.0 && unitIndex + 1 < std::size(unitNames))
    {
        value /= 1024.0;
        ++unitIndex;
    }
    return PString::format_string("{:.2f} {}", value, unitNames[unitIndex]);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

double CmdFSStress::ElapsedSeconds(
    std::chrono::steady_clock::time_point start,
    std::chrono::steady_clock::time_point end)
{
    return std::chrono::duration<double>(end - start).count();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFSStress::PrintThroughput(
    const char* operation,
    uint64_t bytes,
    double duration,
    uint64_t fileSize,
    size_t sectionSize)
{
    const double mebibytesPerSecond = (duration > 0.0)
        ? static_cast<double>(bytes) / (1024.0 * 1024.0) / duration
        : 0.0;

    shutil::WriteAll(
        STDOUT_FILENO,
        PString::format_string(
            "  {} file={} section={}: {:.2f} MiB/s ({:.3f} s)\n",
            operation,
            FormatSize(fileSize),
            FormatSize(sectionSize),
            mebibytesPerSecond,
            duration));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFSStress::PrintOperationRate(
    const char* operation,
    uint64_t operationCount,
    double duration)
{
    const double operationsPerSecond = (duration > 0.0)
        ? static_cast<double>(operationCount) / duration
        : 0.0;

    shutil::WriteAll(
        STDOUT_FILENO,
        PString::format_string(
            "  {}: {} operations, {:.2f} operations/s ({:.3f} s)\n",
            operation,
            operationCount,
            operationsPerSecond,
            duration));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFSStress::ReportError(
    const char* operation,
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdFSStress::ReportFailure(const PString& message)
{
    m_HadError = true;
    shutil::WriteAll(
        STDERR_FILENO,
        PString::format_string(
            "{}: {}\n",
            m_CommandName,
            message));
}

int fsstress_main(int argc, char* argv[])
{
    CmdFSStress command;
    return command.Run(argc, argv);
}

static PAppDefinition g_FSStressAppDef(
    "fsstress",
    "Stress and benchmark filesystems.",
    fsstress_main);

} // namespace shutil_fsstress


