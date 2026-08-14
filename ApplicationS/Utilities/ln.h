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
