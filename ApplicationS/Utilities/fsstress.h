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
// Created: 31.08.2026 22:00

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <Utils/String.h>


namespace shutil_fsstress
{

enum class ParseArgumentsResult
{
    Success,
    Help,
    Error
};


enum class StressMode
{
    All,
    IO,
    Metadata,
    FAT,
    Capacity
};


class CmdFSStress
{
public:
    int Run(int argc, char* argv[]);

private:
    ParseArgumentsResult ParseArguments(int argc, char* argv[]);
    bool PrepareWorkDirectory();
    bool RunSelectedModes();

    bool RunIOMode();
    bool RunIOCase(uint64_t fileSize, size_t sectionSize, size_t iteration);

    bool RunMetadataMode();
    bool RunMetadataIteration(size_t iteration);

    bool RunFATMode();
    bool RunFATIteration(size_t iteration);
    bool TestResizeAndSparseFile(const PString& directory);
    bool TestDirectoryEntryReuse(const PString& directory);
    bool TestDirectoryRename(const PString& directory);
    bool TestOpenUnlink(const PString& directory);

    bool RunCapacityMode();
    bool RunCapacityIteration(size_t iteration);

    bool WritePatternFile(
        const PString& path,
        uint64_t fileSize,
        size_t sectionSize,
        uint32_t salt,
        double* outDuration = nullptr);
    bool VerifyPatternFile(
        const PString& path,
        uint64_t fileSize,
        size_t sectionSize,
        uint32_t salt);
    bool VerifyZeroRange(
        int fileDescriptor,
        uint64_t start,
        uint64_t length,
        size_t sectionSize);
    bool CreateEmptyFile(const PString& path);
    bool EnumerateDirectory(
        const PString& path,
        std::vector<PString>& entries);
    bool SyncFile(int fileDescriptor, const PString& path);
    bool RemoveTree(const PString& path);

    static bool ParseSizeList(
        const std::string& text,
        std::vector<uint64_t>& sizes);
    static bool ParseSize(const std::string& text, uint64_t& size);
    static bool ParseMode(const std::string& text, StressMode& mode);
    static void FillPattern(
        std::vector<uint8_t>& buffer,
        uint32_t salt);
    static PString MakeIndexedName(
        const char* prefix,
        size_t index,
        const char* suffix = "");
    static PString MakeLongName(size_t index, size_t generation);
    static PString FormatSize(uint64_t bytes);
    static double ElapsedSeconds(
        std::chrono::steady_clock::time_point start,
        std::chrono::steady_clock::time_point end);

    void PrintThroughput(
        const char* operation,
        uint64_t bytes,
        double duration,
        uint64_t fileSize,
        size_t sectionSize);
    void PrintOperationRate(
        const char* operation,
        uint64_t operationCount,
        double duration);
    void ReportError(
        const char* operation,
        const PString& path,
        int errorCode);
    void ReportFailure(const PString& message);

    PString               m_CommandName;
    PString               m_BaseDirectory = ".";
    PString               m_WorkDirectory;
    std::vector<uint64_t> m_FileSizes = {64 * 1024, 1024 * 1024};
    std::vector<uint64_t> m_SectionSizes = {512, 4 * 1024, 32 * 1024};
    StressMode            m_Mode = StressMode::All;
    size_t                m_FileCount = 128;
    size_t                m_DirectoryCount = 16;
    size_t                m_DirectoryScans = 10;
    size_t                m_Iterations = 1;
    uint64_t              m_MaxTotalSize = 64 * 1024 * 1024;
    uint32_t              m_Seed = 0x46535354;
    bool                  m_Sync = false;
    bool                  m_Verify = true;
    bool                  m_Keep = false;
    bool                  m_Verbose = false;
    bool                  m_HadError = false;
};

int fsstress_main(int argc, char* argv[]);

} // namespace shutil_fsstress


