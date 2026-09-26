// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <sys/stat.h>

#include <Utils/String.h>


namespace shutil_mv
{

class CmdMv
{
public:
    int Run(int argc, char* argv[]);

private:
    bool MovePath(
        const PString& sourcePath,
        const PString& destinationPath);
    bool CopyAcrossFilesystems(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat,
        const stat_t* destinationStat);
    bool CopyRegularFile(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat);
    bool CopyDirectory(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat);
    bool CopySymlink(
        const PString& sourcePath,
        const PString& destinationPath,
        const stat_t& sourceStat);
    bool RemoveSource(const PString& path);
    bool WriteFileContents(
        int sourceFile,
        int destinationFile,
        const PString& sourcePath,
        const PString& destinationPath);
    void PrintMoved(
        const PString& sourcePath,
        const PString& destinationPath) const;
    void ReportError(
        const PString& operation,
        const PString& path,
        int errorCode);
    void ReportMoveError(
        const PString& sourcePath,
        const PString& destinationPath,
        int errorCode);

    PString m_CommandName;
    bool    m_Force = false;
    bool    m_Interactive = false;
    bool    m_NoClobber = false;
    bool    m_NoTargetDirectory = false;
    bool    m_Verbose = false;
    bool    m_HadError = false;
};

int mv_main(int argc, char* argv[]);

} // namespace shutil_mv
