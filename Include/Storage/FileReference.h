// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
