// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <fcntl.h>

#include <Storage/DirectoryEntry.h>
#include <Storage/FSNode.h>
#include <Storage/DirIterator.h>

class PFileReference;
class PFile;
class PSymLink;


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

class PDirectory : public PFSNode, public PDirIterator
{
public:
    PDirectory();
    PDirectory(const PString& path, int openFlags = O_RDONLY);
    PDirectory(const PDirectory& directory, const PString& name, int openFlags = O_RDONLY);
    PDirectory(const PFileReference& fileReference, int openFlags = O_RDONLY);
    PDirectory(const PFSNode& node);
    PDirectory(const PDirectory& directory);
    PDirectory(int fileDescriptor, bool takeOwnership);
    PDirectory(PDirectory&& directory) = default;
    virtual ~PDirectory();

    virtual bool FDChanged(int newFileDescriptor, const struct ::stat& statBuffer) override;

    ///////////////////////////////////////////////////////////////////////////////
    /// Open the directory pointed to by \p path. The path must
    /// be valid and it must point to a directory.
    /// \param path The directory to open.
    /// \param openFlags Flags describing how to open the directory. Only O_RDONLY,
    ///     O_WRONLY, and O_RDWR are relevant to directories. Take a look
    ///     at the PFSNode documentation for a more detailed description of open modes.
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////
    using PFSNode::Open;

    virtual bool GetNextEntry(PString& outName) override;
    virtual bool GetNextEntry(PFileReference& outReference) override;
    virtual bool Rewind() override;

    bool CreateFile(const PString& name, PFile& outFile, int accessMode = S_IRWXU);
    bool CreateDirectory(const PString& name, PDirectory& outDirectory, int accessMode = S_IRWXU);
    bool CreatePath(const PString& path, bool includeLeaf = true, PDirectory* outLeafDirectory = nullptr, int accessMode = S_IRWXU);
    bool CreateSymlink(const PString& name, const PString& destination, PSymLink& outLink);
    bool Unlink(const PString& name);
    bool GetPath(PString& outPath) const;

    PDirectory& operator=(const PDirectory& rhs) = default;
    PDirectory& operator=(PDirectory&& rhs) = default;

private:
    PDirEntryBuffer m_DirEntryBuffer;
    size_t m_DirEntryBufferSize = 0;
    size_t m_DirEntryBufferOffset = 0;
};
