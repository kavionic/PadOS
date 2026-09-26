// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Utils/String.h>


namespace shutil_rm
{

class CmdRm
{
public:
    int Run(int argc, char* argv[]);

private:
    bool RemovePath(const PString& path);
    bool RemoveDirectoryTree(const PString& path);
    bool ShouldRemove(const PString& path, bool isDirectory) const;
    void ReportError(const PString& operation, const PString& path, int errorCode);

    PString m_CommandName;
    bool    m_Force = false;
    bool    m_Interactive = false;
    bool    m_Recursive = false;
    bool    m_RemoveEmptyDirectories = false;
    bool    m_Verbose = false;
    bool    m_HadError = false;
};

int rm_main(int argc, char* argv[]);

} // namespace shutil_rm
