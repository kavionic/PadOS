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

#include <storage/fsnode.h>

#include <string>

namespace os
{

class Path;

///////////////////////////////////////////////////////////////////////////////
/// Symbolic link handling class.
/// \ingroup storage
/// \par Description:
///
///  \sa FSNode, FileReference
///  \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class SymLink : public FSNode
{
public:
    SymLink();
    SymLink(const String& path, int openFlags = O_RDONLY);
    SymLink(const Directory& directory, const String& name, int openFlags = O_RDONLY);
    SymLink(const FileReference& reference, int openFlags = O_RDONLY);
    SymLink(const FSNode& node);
    SymLink(const SymLink& node);
    virtual ~SymLink();

    virtual bool Open(const String& path, int openFlags = O_RDONLY) override;
    virtual bool Open(const Directory& directory, const String& path, int openFlags = O_RDONLY) override;
    virtual bool Open(const FileReference& reference, int openFlags = O_RDONLY) override;
    virtual bool SetTo(int fileDescriptor, bool takeOwnership) override { return FSNode::SetTo(fileDescriptor, takeOwnership); }
    virtual bool SetTo(const FSNode& node) override;
    virtual bool SetTo(FSNode&& node) override;
    virtual bool SetTo(const SymLink& link);

    bool    ReadLink(String& buffer);
    String  ReadLink();
    bool    ConstructPath(const Directory& parent, Path& outPath);
    bool    ConstructPath(const String& parent, Path& outPath);

};

} // namespace os
