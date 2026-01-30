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

#pragma once

#include <sys/stat.h>
#include <Storage/Directory.h>
#include <Storage/Path.h>

#include <string>


///////////////////////////////////////////////////////////////////////////////
///* Semi persistent reference to a file
/// \ingroup storage
/// \par Description:
///     FileReference's serve much the same functionality as a plain path.
///     It uniquely identifies a file within the file system.
///     The main advantage of a FileReference is that it can partly "track"
///     the file it reference. While moving a file to another directory or
///     renaming the directory it lives in or any of it's subdirectories would
///     break a path, it will not affect a FileReference. Renaming the file
///     itself however from outside the FileReference (ie. not using the
///     FileReference::Rename() member) will break a FileReference though.
///
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PFileReference
{
public:
    PFileReference();
    PFileReference(const PString& path, bool followLinks = false);
    PFileReference(const PDirectory& directory, const PString& name, bool followLinks = false);
    PFileReference(const PFileReference& reference);
    virtual ~PFileReference();
    
    bool    SetTo(const PString& path, bool followLinks = false);
    bool    SetTo(const PDirectory& directory, const PString& name, bool followLinks = false);
    bool    SetTo(const PFileReference& reference);
    void    Unset();
    
    bool    IsValid() const;
    
    PString GetName() const;
    bool    GetPath(PString& outPath) const;

    bool    Rename(const PString& newName);
    bool    Delete();

    bool    GetStat(struct ::stat* statBuffer) const;

    const PDirectory& GetDirectory() const;

private:
    PDirectory   m_Directory;
    PString     m_Name;
};
