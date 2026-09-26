// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Utils/String.h>


namespace shutil_ln
{

class CmdLn
{
public:
    int Run(int argc, char* argv[]);

private:
    bool CreateLink(const PString& target, const PString& linkPath);
    bool RemoveExistingLinkPath(const PString& linkPath);
    void ReportError(
        const PString& operation,
        const PString& path,
        int errorCode);

    PString m_CommandName;
    bool    m_Force = false;
    bool    m_Interactive = false;
    bool    m_NoDereference = false;
    bool    m_NoTargetDirectory = false;
    bool    m_Verbose = false;
    bool    m_HadError = false;
};

int ln_main(int argc, char* argv[]);

} // namespace shutil_ln
