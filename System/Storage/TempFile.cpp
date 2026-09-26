// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
