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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <Storage/FileReference.h>
#include <Storage/Symlink.h>
#include <Storage/Path.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FileReference::FileReference()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FileReference::FileReference(const FileReference& reference) : m_Directory(reference.m_Directory), m_Name(reference.m_Name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FileReference::FileReference(const PString& path, bool followLinks)
{
    SetTo(path, followLinks);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FileReference::FileReference(const Directory& directory, const PString& name, bool followLinks)
{
    SetTo(directory, name, followLinks);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FileReference::~FileReference()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::SetTo(const FileReference& reference)
{
    if (!m_Directory.SetTo(reference.m_Directory)) {
        return false;
    }
    m_Name = reference.m_Name;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::SetTo(const PString& pathString, bool followLinks)
{
    Path path(pathString);

    while (followLinks)
    {
        SymLink link(path.GetPath());
        if (!link.IsValid()) {
            break;
        }
        Path newPath;
        if (!link.ConstructPath(path.GetDir(), newPath)) {
            break;
        }
        path = newPath;
    }
    m_Name = path.GetLeaf();
    if (m_Name.empty()) {
        errno = EINVAL;
        return false;
    }

    if (!m_Directory.Open(path.GetDir(), O_RDONLY)) {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::SetTo(const Directory& directory, const PString& name, bool followLinks)
{
    if (!m_Directory.SetTo(directory)) {
        return false;
    }
    m_Name = name;
    if (followLinks)
    {
        SymLink link(directory, name);
        if (link.IsValid())
        {
            Path newPath;
            if (link.ConstructPath(directory, newPath)) {
                SetTo(newPath.GetPath(), true);
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FileReference::Unset()
{
    m_Directory.Close();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::IsValid() const
{
    return m_Directory.IsValid();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString FileReference::GetName() const
{
    return m_Name;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::GetPath(PString& outPath) const
{
    PString path;

    if (!m_Directory.GetPath(path)) {
        return false;
    }
    path += "/";
    path += m_Name;
    outPath = path;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::Rename(const PString& newName)
{
    PString oldPath;
    if (!m_Directory.GetPath(oldPath)) {
        return false;
    }
    oldPath += "/";
    PString newPath(oldPath);
    oldPath += m_Name;

    if (newName[0] == '/') {
        newPath = newName;
    }
    else {
        newPath += newName;
    }
    status_t result = rename(oldPath.c_str(), newPath.c_str());
    if (result < 0)
    {
        return false;
    }
    else
    {
        if (strchr(newName.c_str(), '/') == nullptr) {
            m_Name = newName;
            return true;
        } else {
            return SetTo(newPath, false);
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::Delete()
{
    PString path;
    if (!GetPath(path)) {
        return false;
    }
    return unlink(path.c_str()) != -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileReference::GetStat(struct ::stat* statBuffer) const
{
    int file = openat(m_Directory.GetFileDescriptor(), m_Name.c_str(), O_RDONLY);
    if (file < 0) {
        return false;
    }
    status_t result = fstat(file, statBuffer);
    close(file);
    return result != -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const Directory& FileReference::GetDirectory() const
{
    return m_Directory;
}
