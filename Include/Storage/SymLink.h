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

class PPath;


///////////////////////////////////////////////////////////////////////////////
/// Symbolic link handling class.
/// \ingroup storage
/// \par Description:
///
///  \sa FSNode, FileReference
///  \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PSymLink : public PFSNode
{
public:
    PSymLink();
    PSymLink(const PString& path, int openFlags = O_RDONLY);
    PSymLink(const PDirectory& directory, const PString& name, int openFlags = O_RDONLY);
    PSymLink(const PFileReference& reference, int openFlags = O_RDONLY);
    PSymLink(const PFSNode& node);
    PSymLink(const PSymLink& node);
    virtual ~PSymLink();

    virtual bool Open(const PString& path, int openFlags = O_RDONLY) override;
    virtual bool Open(const PDirectory& directory, const PString& path, int openFlags = O_RDONLY) override;
    virtual bool Open(const PFileReference& reference, int openFlags = O_RDONLY) override;
    virtual bool SetTo(int fileDescriptor, bool takeOwnership) override { return PFSNode::SetTo(fileDescriptor, takeOwnership); }
    virtual bool SetTo(const PFSNode& node) override;
    virtual bool SetTo(PFSNode&& node) override;
    virtual bool SetTo(const PSymLink& link);

    bool    ReadLink(PString& buffer);
    PString ReadLink();
    bool    ConstructPath(const PDirectory& parent, PPath& outPath);
    bool    ConstructPath(const PString& parent, PPath& outPath);

};
