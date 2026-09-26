// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Utils/String.h>


namespace shutil_rmdir
{

class CmdRmdir
{
public:
    int Run(int argc, char* argv[]);

private:
    bool RemoveDirectory(const PString& path, bool reportIgnoredFailure);
    void ReportError(const PString& path, int errorCode);

    PString m_CommandName;
    bool    m_IgnoreNonEmpty = false;
    bool    m_RemoveParents = false;
    bool    m_Verbose = false;
    bool    m_HadError = false;
};

int rmdir_main(int argc, char* argv[]);

} // namespace shutil_rmdir
