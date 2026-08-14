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
