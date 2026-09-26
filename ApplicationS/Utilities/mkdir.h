// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <sys/pados_types.h>

#include <Utils/String.h>


namespace shutil_mkdir
{

class CmdMkdir
{
public:
    int Run(int argc, char* argv[]);

private:
    bool CreateDirectory(const PString& path, bool requestedDirectory);
    void ReportError(const PString& path, int errorCode);

    PString m_CommandName;
    mode_t  m_Mode = 0777;
    bool    m_CreateParents = false;
    bool    m_Verbose = false;
    bool    m_HadError = false;
};

int mkdir_main(int argc, char* argv[]);

} // namespace shutil_mkdir
