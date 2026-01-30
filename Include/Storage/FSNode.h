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
#include <System/System.h>
#include <System/Types.h>
#include <System/ErrorCodes.h>
#include <Storage/Path.h>

class PFileReference;
class PDirectory;


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


class PFSNode
{
public:
    PFSNode();
    PFSNode(const PString& path, int openFlags = O_RDONLY);
    PFSNode(const PDirectory& directory, const PString& path, int openFlags = O_RDONLY);
    PFSNode(const PFileReference& reference, int openFlags = O_RDONLY);
    PFSNode(int fileDescriptor, bool takeOwnership);
    PFSNode(const PFSNode& node);
    PFSNode(PFSNode&& node);
    virtual ~PFSNode();

    virtual bool FDChanged(int newFileDescriptor, const struct ::stat& statBuffer);
    
    bool            Open(const PPath& path, int openFlags = O_RDONLY) { return Open(path.GetPath(), openFlags); }
    virtual bool    Open(const PString& path, int openFlags = O_RDONLY);
    virtual bool    Open(const PDirectory& directory, const PString& path, int openFlags = O_RDONLY);
    virtual bool    Open(const PFileReference& reference, int openFlags = O_RDONLY);
    virtual bool    SetTo(int fileDescriptor, bool takeOwnership);
    virtual bool    SetTo(const PFSNode& node);
    virtual bool    SetTo(PFSNode&& node);
    virtual void    Close();
    
    virtual bool    IsValid() const;
    
    virtual bool    GetStat(struct ::stat* statBuffer, bool updateCache = true) const;
    virtual bool    SetStats(const struct stat& value, uint32_t mask) const;

    virtual ino_t   GetInode() const;
    virtual dev_t   GetDev() const;
    virtual int     GetMode(bool updateCache = false) const;
    virtual off64_t GetSize(bool updateCache = true) const;
    virtual bool    SetSize(off64_t size) const;

    virtual PErrorCode GetCTime(TimeValNanos& outTime, bool updateCache = true) const;
    virtual PErrorCode GetMTime(TimeValNanos& outTime, bool updateCache = true) const;
    virtual PErrorCode GetATime(TimeValNanos& outTime, bool updateCache = true) const;

    PErrorCode GetCTime(time_t& outTime, bool updateCache = true) const;
    PErrorCode GetMTime(time_t& outTime, bool updateCache = true) const;
    PErrorCode GetATime(time_t& outTime, bool updateCache = true) const;

    bool    IsDir() const       { return S_ISDIR(GetMode()); }
    bool    IsLink() const      { return S_ISLNK(GetMode()); }
    bool    IsFile() const      { return S_ISREG(GetMode()); }
    bool    IsCharDev() const   { return S_ISCHR(GetMode()); }
    bool    IsBlockDev() const  { return S_ISBLK(GetMode()); }
    bool    IsFIFO() const      { return S_ISFIFO(GetMode()); }
    
//    virtual status_t GetNextAttrName( PString* pcName );
//    virtual status_t RewindAttrdir();
//
//    virtual ssize_t  WriteAttr( const PString& cAttrName, int nFlags, int nType, const void* pBuffer, off_t nPos, size_t nLen );
//    virtual ssize_t  ReadAttr( const PString& cAttrName, int nType, void* pBuffer, off_t nPos, size_t nLen );
//
//    virtual status_t RemoveAttr( const PString& cName );
//    virtual status_t StatAttr( const PString& cName, struct ::attr_info* psBuffer );

    virtual int GetFileDescriptor() const;
    
    PFSNode& operator=(const PFSNode& rhs);
    PFSNode& operator=(PFSNode&& rhs);

protected:
    bool ParseResult(PErrorCode result) const
    {
        if (result == PErrorCode::Success)
        {
            return true;
        }
        else
        {
            set_last_error(result);
            return false;
        }
    }

private:
    friend class PDirectory;
    
    int  m_FileDescriptor = -1;
//    DIR* m_hAttrDir = nullptr;
    mutable struct ::stat m_StatCache;
};
