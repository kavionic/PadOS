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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <dirent.h>

#include <Utils/String.h>
#include <System/Types.h>
#include <Storage/Path.h>

namespace os
{

class FileReference;
class Directory;

///////////////////////////////////////////////////////////////////////////////
/// Lowlevel filesystem node class.
/// \ingroup storage
/// \par Description:
///     This class give access to the lowest level of a file system node.
///     A node can be a directory, regular file, symlink or named pipe.
///
///     It give you access to stats that is common to all nodes in the
///     file system like time-stamps, access-rights, i-node and device
///     numbers, and most important the file-attributes.
///
///     The native AtheOS file system (AFS) support "attributes" which is
///     extra data-streams associated with file system nodes. An attribute
///     can have a specific type like int, float, string, etc etc, or it
///     can be a untyped stream of data. Attributes can be used to store
///     information associated by the file but that don't belong to file
///     content itself (for example the file's icon-image and mime-type).
///
/// \sa os::FileReference, os::File, os::Directory
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////


class FSNode
{
public:
    FSNode();
    FSNode(const String& path, int openFlags = O_RDONLY);
    FSNode(const Directory& directory, const String& path, int openFlags = O_RDONLY);
    FSNode(const FileReference& reference, int openFlags = O_RDONLY);
    FSNode(int fileDescriptor, bool takeOwnership);
    FSNode(const FSNode& node);
    FSNode(FSNode&& node);
    virtual ~FSNode();

    virtual bool FDChanged(int newFileDescriptor, const struct ::stat& statBuffer);
    
    bool            Open(const Path& path, int openFlags = O_RDONLY) { return Open(path.GetPath(), openFlags); }
    virtual bool    Open(const String& path, int openFlags = O_RDONLY);
    virtual bool    Open(const Directory& directory, const String& path, int openFlags = O_RDONLY);
    virtual bool    Open(const FileReference& reference, int openFlags = O_RDONLY);
    virtual bool    SetTo(int fileDescriptor, bool takeOwnership);
    virtual bool    SetTo(const FSNode& node);
    virtual bool    SetTo(FSNode&& node);
    virtual void    Close();
    
    virtual bool    IsValid() const;
    
    virtual bool    GetStat(struct ::stat* statBuffer, bool updateCache = true) const;
    virtual bool    SetStats(const struct stat& value, uint32_t mask) const;

    virtual ino_t   GetInode() const;
    virtual dev_t   GetDev() const;
    virtual int     GetMode(bool updateCache = false) const;
    virtual off64_t GetSize(bool updateCache = true) const;
    virtual bool    SetSize(off64_t size) const;

    virtual time_t  GetCTime(bool updateCache = true) const;
    virtual time_t  GetMTime(bool updateCache = true) const;
    virtual time_t  GetATime(bool updateCache = true) const;

    bool    IsDir() const       { return S_ISDIR(GetMode()); }
    bool    IsLink() const      { return S_ISLNK(GetMode()); }
    bool    IsFile() const      { return S_ISREG(GetMode()); }
    bool    IsCharDev() const   { return S_ISCHR(GetMode()); }
    bool    IsBlockDev() const  { return S_ISBLK(GetMode()); }
    bool    IsFIFO() const      { return S_ISFIFO(GetMode()); }
    
//    virtual status_t GetNextAttrName( String* pcName );
//    virtual status_t RewindAttrdir();
//
//    virtual ssize_t  WriteAttr( const String& cAttrName, int nFlags, int nType, const void* pBuffer, off_t nPos, size_t nLen );
//    virtual ssize_t  ReadAttr( const String& cAttrName, int nType, void* pBuffer, off_t nPos, size_t nLen );
//
//    virtual status_t RemoveAttr( const String& cName );
//    virtual status_t StatAttr( const String& cName, struct ::attr_info* psBuffer );

    virtual int GetFileDescriptor() const;
    
    FSNode& operator=(const FSNode& rhs);
    FSNode& operator=(FSNode&& rhs);
private:
    friend class Directory;
    
    int  m_FileDescriptor = -1;
//    DIR* m_hAttrDir = nullptr;
    mutable struct ::stat m_StatCache;
};

} // namespace os
