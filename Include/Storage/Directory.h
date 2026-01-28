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

#include <fcntl.h>

#include <Storage/FSNode.h>
#include <Storage/DirIterator.h>

namespace os
{

class FileReference;
class File;
class SymLink;

///////////////////////////////////////////////////////////////////////////////
/// Filesystem directory class
/// \ingroup storage
/// \par Description:
///     This class let you iterate over the content of a directory.
///
///     Unlink other FSNode derivated classes it is possible to ask a
///     os::Directory to retrieve it's own path.
///
/// \sa FSNode
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class Directory : public FSNode, public DirIterator
{
public:
    Directory();
    Directory(const PString& path, int openFlags = O_RDONLY);
    Directory(const Directory& directory, const PString& name, int openFlags = O_RDONLY);
    Directory(const FileReference& fileReference, int openFlags = O_RDONLY);
    Directory(const FSNode& node);
    Directory(int fileDescriptor, bool takeOwnership);
    Directory(const Directory& directory);
    Directory(Directory&& directory) = default;
    virtual ~Directory();

    virtual bool FDChanged(int newFileDescriptor, const struct ::stat& statBuffer) override;
    
    virtual bool GetNextEntry(PString& outName) override;
    virtual bool GetNextEntry(FileReference& outReference) override;
    virtual bool Rewind() override;

    bool CreateFile(const PString& name, File& outFile, int accessMode = S_IRWXU);
    bool CreateDirectory(const PString& name, Directory& outDirectory, int accessMode = S_IRWXU);
    bool CreatePath(const PString& path, bool includeLeaf = true, Directory* outLeafDirectory = nullptr, int accessMode = S_IRWXU);
    bool CreateSymlink(const PString& name, const PString& destination, SymLink& outLink);
    bool Unlink(const PString& name);
    bool GetPath(PString& outPath) const;

    Directory& operator=(const Directory& rhs) = default;
    Directory& operator=(Directory&& rhs) = default;
};

} // namespace os
