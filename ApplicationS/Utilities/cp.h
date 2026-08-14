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

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sys/pados_types.h>
#include <sys/stat.h>

#include <Utils/String.h>


namespace shutil_cp
{

enum class DereferenceMode
{
    CommandLineOnly,
    Always,
    Never
};

enum class BackupMode
{
    None,
    Simple,
    Numbered,
    Existing
};

enum PreserveAttribute : uint32_t
{
    PreserveAttribute_None       = 0x00,
    PreserveAttribute_Mode       = 0x01,
    PreserveAttribute_Ownership  = 0x02,
    PreserveAttribute_Timestamps = 0x04,
    PreserveAttribute_All        = PreserveAttribute_Mode |
                                   PreserveAttribute_Ownership |
                                   PreserveAttribute_Timestamps
};

class CmdCp
{
public:
    int Run(int argc, char* argv[]);

private:
    bool ParseArguments(
        int argc,
        char* argv[],
        std::vector<PString>& sourcePaths,
        PString& destinationPath);
    bool ParsePreserveAttributes(
        const std::string& attributeList,
        bool preserve);
    bool ParseBackupMode(const std::string& control);
    bool ConfigureOperands(
        const std::vector<std::string>& operands,
        const std::optional<std::string>& targetDirectory,
        std::vector<PString>& sourcePaths,
        PString& destinationPath);

    bool CopyCommandLinePath(
        const PString& sourcePath,
        const PString& destinationPath);
    bool CopyPath(
        const PString& sourcePath,
        const PString& destinationPath,
        bool commandLineOperand,
        dev_t traversalDevice);
    bool CopyRegularFile(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat);
    bool CopyDirectory(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat,
        bool followSourceSymlink,
        dev_t traversalDevice);
    bool CopySymlink(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat);
    bool CreateSymbolicLink(
        const PString& sourcePath,
        const PString& destinationPath);

    bool PrepareDestination(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat,
        const stat_t& destinationStat,
        bool removeDestination,
        bool& outSkipped);
    bool RemoveDestination(const PString& destinationPath);
    bool BackupDestination(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat);
    PString GetBackupPath(const PString& destinationPath) const;
    size_t GetHighestBackupNumber(
        const PString& destinationPath) const;
    PString GetNumberedBackupPath(const PString& destinationPath) const;

    bool ApplyAttributes(
        const PString& destinationPath,
        const stat_t& sourceStat,
        bool followDestinationSymlink);
    bool EnsureParentDirectories(const PString& path);
    PString BuildParentsDestination(
        const PString& sourcePath,
        const PString& destinationDirectory) const;
    static PString GetParentsRelativePath(const PString& sourcePath);
    static bool IsSafeParentsPath(const PString& sourcePath);
    static bool IsSourceNewer(
        const stat_t& sourceStat,
        const stat_t& destinationStat);
    bool WriteFileContents(
        int sourceFile,
        int destinationFile,
        const PString& sourcePath,
        const PString& destinationPath);
    void PrintCopied(
        const PString& sourcePath,
        const PString& destinationPath) const;
    void ReportError(
        const PString& operation,
        const PString& path,
        int errorCode);
    void ReportCopyError(
        const PString& sourcePath,
        const PString& destinationPath,
        int errorCode);

    PString         m_CommandName;
    DereferenceMode m_DereferenceMode = DereferenceMode::CommandLineOnly;
    BackupMode      m_BackupMode = BackupMode::None;
    PString         m_BackupSuffix = "~";
    uint32_t        m_PreserveAttributes = PreserveAttribute_None;
    bool            m_AttributesOnly = false;
    bool            m_Force = false;
    bool            m_Interactive = false;
    bool            m_NoClobber = false;
    bool            m_OneFileSystem = false;
    bool            m_Parents = false;
    bool            m_Recursive = false;
    bool            m_RemoveDestination = false;
    bool            m_StripTrailingSlashes = false;
    bool            m_SymbolicLink = false;
    bool            m_NoTargetDirectory = false;
    bool            m_Update = false;
    bool            m_Verbose = false;
    bool            m_HadError = false;
    std::vector<std::pair<dev_t, ino_t>> m_DirectoryStack;
};

int cp_main(int argc, char* argv[]);

} // namespace shutil_cp
