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
#include <Kernel/VFS/FileIO.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TempFile::TempFile(const String& prefix, const String& path, int access) : File()
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

TempFile::~TempFile()
{
    if (m_DeleteFile) {
        FileIO::Unlink(m_Path.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TempFile::Detatch()
{
    m_DeleteFile = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TempFile::Unlink()
{
    if (m_DeleteFile)
    {
        m_DeleteFile = false;
        return FileIO::Unlink(m_Path.c_str()) != -1;
    }
    else
    {
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String TempFile::GetPath() const
{
    return m_Path;
}
