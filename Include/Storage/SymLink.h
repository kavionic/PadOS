// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
