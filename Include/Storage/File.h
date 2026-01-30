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

#include <Storage/SeekableIO.h>
#include <Storage/FSNode.h>


///////////////////////////////////////////////////////////////////////////////
/// \ingroup storage
/// \par Description:
///
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PFile : public PSeekableIO, public PFSNode
{
public:
    enum { DEFAULT_BUFFER_SIZE=0 };
    PFile();
    PFile(const PString& path, int openFlags = O_RDONLY);
    PFile(const PDirectory& directory, const PString& name, int openFlags = O_RDONLY);
    PFile(const PFileReference& reference, int openFlags = O_RDONLY);
    PFile(const PFSNode& node);
    PFile(int fileDescriptor, bool takeOwnership);
    PFile(const PFile& file);
    virtual ~PFile();
    
      // From FSNode
    virtual bool    FDChanged(int newFileDescriptor, const struct ::stat& statBuffer) override;
    virtual off64_t GetSize(bool updateCache = true) const override;
    
      // From StreamableIO
    virtual ssize_t Read(void* buffer, ssize_t size) override;
    bool            Read(PString& buffer, ssize_t size);
    bool            Read(PString& buffer);
    virtual ssize_t Write(const void* buffer, ssize_t size) override;
    bool            Write(const PString& buffer, ssize_t size);
    bool            Write(const PString& buffer);

      // From seekableIO
    virtual ssize_t ReadPos(off64_t position, void* buffer, ssize_t size) const override;
    virtual ssize_t WritePos(off64_t position, const void* buffer, ssize_t size) override;

    virtual off64_t Seek(off64_t position, int mode) override;

      // From File
    bool    SetBufferSize(size_t size);
    size_t  GetBufferSize() const;
    bool    Flush() const;
    
private:
    bool FillBuffer(off64_t nPos) const;

    mutable uint8_t*    m_Buffer = nullptr;
    size_t              m_BufferSize = DEFAULT_BUFFER_SIZE;
    mutable size_t      m_ValidBufferSize = 0;
    off64_t             m_Position = 0;
    mutable off64_t     m_BufferPosition = 0;
    mutable bool        m_Dirty = false;
};
