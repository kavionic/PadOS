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

#pragma once

#include <sys/types.h>
#include <fcntl.h>
#include <string>

#include <Utils/String.h>

namespace os
{
class Directory;

///////////////////////////////////////////////////////////////////////////////
/// \ingroup storage
/// \par Description:
///
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class Path
{
public:
    Path();
    Path(const Path& path);
    Path(const String& path);
    Path(const char* path, size_t length);
    ~Path();

    void operator =(const Path& path);
    void operator =(const String& path);
    bool operator==(const Path& path) const;

    void SetTo(const String& path);
    void SetTo(const char* path, size_t length);
    void Append(const Path& path);
    void Append(const String& name);
  
    String          GetLeaf() const;
    const String&   GetPath() const;
    String          GetDir() const;

    size_t          GetExtensionLength() const;
    String          GetExtension() const;
    bool            SetExtension(const String& extension);

    const char* c_str() const { return m_Path.c_str(); }

    bool    CreateFolders(bool includeLeaf = true, Directory* outLeafDirectory = nullptr, int accessMode = S_IRWXU);

    operator String() const;

    static String GetCWD();
private:
    void Normalize();

    String  m_Path;
    size_t  m_NameStart = 0;
};

} // namespace os

using PPath = os::Path;

namespace std
{
template<>
struct formatter<PPath> : formatter<basic_string<PString::value_type, PString::traits_type, PString::allocator_type>, PString::value_type> {};
}
