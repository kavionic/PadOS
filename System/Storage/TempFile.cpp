// This file is part of PadOS.
//
// Copyright (C) 2001-2020 Kurt Skauen <http://kavionic.com/>
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

#include <stdio.h>
#include <unistd.h>
#include <Storage/TempFile.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTempFile::PTempFile(const PString& prefix, const PString& path, int access) : PFile()
{
    m_DeleteFile = true;
    while (true)
    {
        char* tempName = tempnam(path.empty() ? nullptr : path.c_str(), prefix.empty() ? nullptr : prefix.c_str());
        if (tempName == nullptr) {
            break;
        }
        m_Path = tempName;
        free(tempName);
        if (!Open(m_Path, O_RDWR | O_CREAT | O_EXCL))
        {
            if (errno == EEXIST) {
                continue;
            } else {
                break;
            }
        }
        else
        {
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTempFile::~PTempFile()
{
    if (m_DeleteFile) {
        unlink(m_Path.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTempFile::Detatch()
{
    m_DeleteFile = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTempFile::Unlink()
{
    if (m_DeleteFile)
    {
        m_DeleteFile = false;
        return unlink(m_Path.c_str()) != -1;
    }
    else
    {
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PTempFile::GetPath() const
{
    return m_Path;
}
