// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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
#include <string.h>
#include <assert.h>

#include <Storage/Path.h>
#include <Storage/Directory.h>
#include <Kernel/VFS/KFilesystem.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Path::Path()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Path::Path(const Path& cPath)
{
    SetTo(cPath.m_Path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Path::Path(const String& path)
{
    SetTo(path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Path::Path(const char* path, size_t length)
{
    SetTo(path, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Path::~Path()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Path::operator =(const Path& cPath)
{
    SetTo(cPath.m_Path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Path::operator =(const String& path)
{
    SetTo(path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Path::operator==(const Path& path) const
{
    return m_Path == path.m_Path;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Path::SetTo(const String& path)
{
    if (path.empty())
    {
        m_Path = GetCWD();
    }
    else if (path[0] != '/')
    {
        m_Path = GetCWD() + "/" + path;
    }
    else
    {
        m_Path = path;
    }
    Normalize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Path::SetTo(const char* path, size_t length)
{
    SetTo(String(path, length));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Path::Append(const String& path)
{
    if (path.empty()) {
        return;
    }
    m_Path += String("/") + path;
    Normalize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Path::Append(const Path& cPath)
{
    Append(cPath.m_Path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String Path::GetLeaf() const
{
    String name(m_Path.begin() + m_NameStart, m_Path.end());
    return name;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String Path::GetDir() const
{
    if (m_NameStart >= m_Path.size()) {
        return m_Path;
    } else if (m_NameStart > 1 && m_Path[m_NameStart - 1] == '/') {
        return String(m_Path.begin(), m_Path.begin() + m_NameStart - 1);
    } else {
        return String(m_Path.begin(), m_Path.begin() + m_NameStart);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Path::CreateFolders(bool includeLeaf, Directory* outLeafDirectory, int accessMode)
{
    if (m_Path.empty())
    {
        set_last_error(EINVAL);
        return false;
    }
    Directory directory("/");
    if (!directory.IsValid()) {
        return false;
    }
    return directory.CreatePath(String(m_Path.begin() + 1, m_Path.end()), includeLeaf, outLeafDirectory, accessMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const String& Path::GetPath() const
{
    return m_Path;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String Path::GetCWD()
{
    std::vector<char> buffer;
    buffer.resize(128);

    for (;;)
    {
        if (getcwd(buffer.data(), buffer.size()) != nullptr)
        {
            return String(buffer.data());
        }
        else
        {
            if (errno != ERANGE) {
                return String::zero;
            }
            if (buffer.size() < PATH_MAX)
            {
                buffer.resize(buffer.size() * 2);
            }
            else
            {
                errno = ENAMETOOLONG;
                return String::zero;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Path::Normalize()
{
    if (m_Path.size() < 2) {
        m_NameStart = m_Path.size();
        return;
    }

    size_t nameStart = 1;
    size_t prevNameStart = nameStart;

    for (size_t i = nameStart; i <= m_Path.size(); ++i)
    {
        if (i == m_Path.size() || m_Path[i] == '/')
        {
            size_t nameLength = i - nameStart;
            if (nameLength == 0)
            {
                for (size_t j = i; j <= m_Path.size(); ++j)
                {
                    if (j == m_Path.size() || m_Path[j] != '/')
                    {
                        size_t consecutiveSlashes = j - i;
                        if (consecutiveSlashes > 0)
                        {
                            m_Path.erase(i, consecutiveSlashes);
                        }
                        break;
                    }
                }
            }
            else if (nameLength == 1 && m_Path[nameStart] == '.')
            {
                if (nameStart != 0)
                {
                    if (nameStart == m_Path.size() - 1) {
                        m_Path.erase(nameStart, 1);
                    }
                    else {
                        m_Path.erase(nameStart, 2);
                    }
                    --i;
                }
                else
                {
                    prevNameStart = nameStart;
                    nameStart = i + 1;
                }
            }
            else if (nameLength == 2 && m_Path[nameStart] == '.' && m_Path[nameStart + 1] == '.')
            {
                if (prevNameStart > 0 && prevNameStart == nameStart) // Two ".." in a row.
                {
                    prevNameStart--;
                    while (prevNameStart > 0 && m_Path[prevNameStart - 1] != '/') prevNameStart--;
                }
                if (prevNameStart == 0) {
                    m_Path.erase(1, i - prevNameStart);
                } else {
                    m_Path.erase(prevNameStart, i - prevNameStart + 1);
                }
                nameStart = prevNameStart;
                i = prevNameStart;
            }
            else
            {
                prevNameStart = nameStart;
                nameStart = i + 1;
            }
        }
    }
    if (m_Path.size() > 1 && m_Path[m_Path.size() - 1] == '/') {
        m_Path.erase(m_Path.begin() + m_Path.size() - 1);
    }
    
    if (prevNameStart >= m_Path.size())
    {
        m_NameStart = m_Path.size();
        while (m_NameStart > 0 && m_Path[m_NameStart - 1] != '/') m_NameStart--;
    }
    else
    {
        m_NameStart = prevNameStart;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Path::operator String() const
{
    return m_Path;
}
