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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <limits.h>

#include <new>

#include <Storage/File.h>
#include <Storage/FileReference.h>
#include <Storage/Path.h>
#include <System/System.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// Default constructor
/// \par Description:
///     Initialize the instance to a known but invalid state. The instance
///     must be successfully initialized with one of the SetTo() members
///     before it can be used.
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

File::File()
{
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a file from a regular path.
/// \par Description:
///     Open the file pointed to by \p path. The node must be a regular file
///     or a symlink pointing at a regular file. Anything else will cause and
///     errno_exception(EINVAL) exception to be thrown.
///
/// \param path
///     The file to open. The path can either be absolute (starting
///     with "/") or relative to the current working directory.
/// \param openFlags
///
///     Flags controlling how the file should be opened. The value
///     should be one of the O_RDONLY, O_WRONLY, or O_RDWR. In addition
///     the following flags can be bitwise or'd in to further control
///     the operation:
///
///     - O_TRUNC       truncate the size to 0.
///     - O_APPEND      automatically move the file-pointer to the end
///                     of the file before each write (only valid for files)
///     - O_NONBLOCK    open the file in non-blocking mode. This is only
///                     relevant for named pipes and PTY's.
///     - O_NOCTTY      Don't make the file controlling TTY even if \p path
///                     points to a PTY.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

File::File(const String& path, int openFlags) : FSNode(path, openFlags & ~O_NOFOLLOW)
{
    if (IsDir())
    {
        Close();
        errno = EINVAL;
    }
}

///////////////////////////////////////////////////////////////////////////////
///* Open a file addressed as a name inside a specified directory.
/// \par Description:
///     Look at File( const String& path, int openFlags ) for a more
///     detailed description.
///
/// \param directory
///     The directory to use as a base for \p path
/// \param path
///     Path relative to \p path.
/// \param openFlags
///     Flags controlling how to open the file. See
///     File( const String& path, int openFlags )
///     for a full description.
///
/// \sa File( const String& path, int openFlags )
/// \author Kurt Skauen (kurt@atheos.cx)
 ///////////////////////////////////////////////////////////////////////////////


File::File(const Directory& directory, const String& path, int openFlags) : FSNode(directory, path, openFlags & ~O_NOFOLLOW)
{
    if (IsDir())
    {
        Close();
        errno = EINVAL;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Open a file referred to by a os::FileReference.
/// \par Description:
///     Look at File(const String& path, int openFlags) for a more
///     detailed description.
///
/// \param reference
///     Reference to the file to open.
/// \param openFlags
///     Flags controlling how to open the file. See
///     File( const String& path, int openFlags )
///     for a full description.
///
/// \sa File( const String& path, int openFlags )
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

File::File(const FileReference& reference, int openFlags) : FSNode(reference, openFlags & ~O_NOFOLLOW)
{
    if (IsDir())
    {
        Close();
        errno = EINVAL;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a file from a FSNode.
/// \par Description:
///     This constructor can be used to "downcast" an os::FSNode to a
///     os::File. The FSNode must represent a regular file, pipe, or PTY.
///     Attempts to convert symlinks and directories will cause an
///     errno_exception(EINVAL) exception to be thrown.
///
/// \param node
///     The FSNode to downcast.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

File::File(const FSNode& node) : FSNode(node)
{
    if (IsDir())
    {
        Close();
        errno = EINVAL;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a file object from a open filedescriptor.
/// \par Description:
///     Construct a file object from a open filedescriptor.
/// \note
///     The file descriptor will be close when the object is deleted.
/// \par Warning:
/// \param
/// \return
/// \par Error codes:
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

File::File(int fileDescriptor, bool takeOwnership) : FSNode(fileDescriptor, takeOwnership)
{
    if (IsDir())
    {
        Close();
        errno = EINVAL;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Copy constructor.
/// \par Description:
///     This constructor will make an independent copy of \p file.
///     The copy and original will have their own file pointers and
///     they will have their own attribute directory iterators (see note).
/// \par Note:
///     The attribute directory iterator will not be cloned so when
///     FSNode::GetNextAttrName() is called for the first time it will
///     return the first attribute name even if the iterator was not
///     located at the beginning of the originals attribute directory.
///
/// \param file
///     The file object to copy.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

File::File(const File& file) : FSNode(file)
{
    m_Position   = file.m_Position;
    m_BufferSize = file.m_BufferSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

File::~File()
{
    if (m_Dirty) {
        Flush();
    }
    delete[] m_Buffer;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool File::FDChanged(int newFileDescriptor, const struct ::stat& statBuffer)
{
    if (m_Dirty && IsValid()) {
        Flush();
    }
    m_ValidBufferSize = 0;
    m_Position = 0;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool File::FillBuffer(off64_t position) const
{
    if (!IsValid()) {
        set_last_error(EINVAL);
        return false;
    }

    if (m_Dirty)
    {
        if (!Flush()) {
            return false;
        }
    }
    m_BufferPosition = position;

    const int fileDescriptor = GetFileDescriptor();
    for(;;)
    {
        ssize_t length = pread(fileDescriptor, m_Buffer, m_BufferSize, position);
        if (length < 0)
        {
            if (errno == EINTR) {
                continue;
            } else {
                return false;
            }
        }
        m_ValidBufferSize = length;
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Set the size of the files caching buffer.
/// \par Description:
///     By default the os::File class use a 32KB internal memory
///     buffer to cache recently read/written data. Normally you the
///     buffer can greatly increase the performance since it reduce
///     the number of kernel-calls when doing multiple small reads or
///     writes on the file. There is cases however where it is
///     beneficial to increase the buffer size or even to disabling
///     buffering entirely.
///
///     If you read/write large chunks of data at the time the buffer
///     might impose more overhead than gain and it could be a good
///     idea to disable the buffering entirely by setting the buffer
///     size to 0. When for example streaming video the amount of data
///     read are probably going to be much larger than the buffer
///     anyway and each byte is only read once by the application and
///     if the application read the file in reasonably sized chunks
///     the extra copy imposed by reading the data into the internal
///     buffer and then copy it to the callers buffer will only
///     decrease the performance.
///
/// \param bufferSize
///     The buffer size in bytes. If the size is set to 0 the file
///     will be unbuffered and each call to Read()/Write() and
///     ReadPos()/WritePos() will translate directly to kernel calls.
///
/// \return
///     On success 0 is returned. If there was not enough memory for
///     the new buffer -1 will be returned and the global variable
///     "errno" will be set to ENOMEM.
/// \sa GetBufferSize()
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool File::SetBufferSize(size_t bufferSize)
{
    if (bufferSize == m_BufferSize) {
        return true;
    }
    if (bufferSize == 0)
    {
        if (m_Dirty)
        {
            if (!Flush()) {
                return false;
            }
        }
        m_ValidBufferSize = 0;
        m_BufferSize = 0;
        delete[] m_Buffer;
        m_Buffer = nullptr;
    }
    else
    {
        try
        {
            if (m_Buffer != nullptr)
            {
                uint8_t* pNewBuffer = new uint8_t[bufferSize];
                if (m_ValidBufferSize > bufferSize) {
                    m_ValidBufferSize = bufferSize;
                }
                if (m_ValidBufferSize > 0) {
                    memcpy(pNewBuffer, m_Buffer, m_ValidBufferSize);
                }
                delete[] m_Buffer;
                m_Buffer = pNewBuffer;
            }
            m_BufferSize = bufferSize;
        }
        catch (std::bad_alloc& cExc)
        {
            errno = ENOMEM;
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Obtain the files buffer size.
// \return
//      The files buffer size in bytes. A value of 0 means the file is
//      unbuffered.
// \sa SetBufferSize()
// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

size_t File::GetBufferSize() const
{
    return m_BufferSize;
}

///////////////////////////////////////////////////////////////////////////////
/// Write unwritten data to the underlying file.
/// \par Note:
///     Flush() will be called automatically by the destructor if there is
///     unwritten data in the buffer.
/// \return
///     On success 0 will be returned. On failure -1 will be returned and
///     a error code will be stored in the global variable "errno".
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool File::Flush() const
{
    if (!IsValid()) {
        set_last_error(EINVAL);
        return false;
    }
    if (m_Dirty)
    {
        ssize_t     size = m_ValidBufferSize;
        size_t      offset = 0;
        const int   fileDescriptor = GetFileDescriptor();

        while (size > 0)
        {
            ssize_t bytesWritten = pwrite(fileDescriptor, m_Buffer + offset, size, m_BufferPosition + offset);
            if (bytesWritten < 0)
            {
                if (errno == EINTR) {
                    continue;
                } else {
                    return false;
                }
            }
            size -= bytesWritten;
            offset += bytesWritten;
        }
        m_Dirty = false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t File::Read(void* buffer, ssize_t size)
{
    ssize_t bytesRead = ReadPos(m_Position, buffer, size);
    if (bytesRead > 0) {
        m_Position += bytesRead;
    }
    return bytesRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool File::Read(String& buffer, ssize_t size)
{
    if (size < 0 || !IsValid()) {
        set_last_error(EINVAL);
        return false;
    }
    try
    {
        buffer.resize(size);
        ssize_t bytesRead = Read(buffer.data(), size);
        if (bytesRead < 0)
        {
            buffer.clear();
            return false;
        }
        else
        {
            if (bytesRead < size) {
                buffer.resize(bytesRead);
            }
            return true;
        }
    }
    catch (const std::bad_alloc&)
    {
        set_last_error(ENOMEM);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool File::Read(String& buffer)
{
    return Read(buffer, ssize_t(GetSize()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t File::Write(const void* buffer, ssize_t size)
{
    ssize_t bytesWritten = WritePos(m_Position, buffer, size);
    if (bytesWritten > 0) {
        m_Position += bytesWritten;
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool File::Write(const String& buffer, ssize_t size)
{
    if (size < 0 || !IsValid()) {
        set_last_error(EINVAL);
        return false;
    }
    return Write(buffer.c_str(), size) == size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool File::Write(const String& buffer)
{
    return Write(buffer, buffer.size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t File::ReadPos(off64_t position, void* buffer, ssize_t size) const
{
    if (!IsValid()) {
        set_last_error(EINVAL);
        return -1;
    }
    const int fileDescriptor = GetFileDescriptor();

    if (fileDescriptor == -1) 
    if (m_BufferSize == 0) {
        return pread(fileDescriptor, buffer, size, position);
    }
    if (m_Buffer == nullptr)
    {
        try {
            m_Buffer = new uint8_t[m_BufferSize];
        }
        catch (std::bad_alloc& cExc) {
            errno = ENOMEM;
            return -1;
        }
    }
    ssize_t bytesRead = 0;

    if (size > m_BufferSize * 2)
    {
        ssize_t preSize = size - m_BufferSize;
        if (m_ValidBufferSize == 0 || position + preSize <= m_BufferPosition || position > m_BufferPosition + m_ValidBufferSize)
        {
            ssize_t currentBytesRead = pread(fileDescriptor, buffer, preSize, position);
            if (currentBytesRead < 0) {
                return currentBytesRead;
            }
            bytesRead += currentBytesRead;
            position  += currentBytesRead;
            size      -= currentBytesRead;
            buffer     = ((uint8_t*)buffer) + currentBytesRead;
        }
    }

    while (size > 0)
    {
        if (position < m_BufferPosition || position >= m_BufferPosition + m_ValidBufferSize)
        {
            if (!FillBuffer(position)) {
                return (bytesRead > 0) ? bytesRead : -1;
            } else if (m_ValidBufferSize == 0) {
                return bytesRead;
            }
        }
        off64_t bufferOffset = position - m_BufferPosition;
        ssize_t currentLength = ssize_t(m_ValidBufferSize - bufferOffset);
        if (currentLength > size) {
            currentLength = size;
        }
        memcpy(buffer, m_Buffer + bufferOffset, currentLength);
        bytesRead   += currentLength;
        position    += currentLength;
        size        -= currentLength;
        buffer      = ((uint8_t*)buffer) + currentLength;
    }
    return bytesRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t File::WritePos(off64_t position, const void* buffer, ssize_t size)
{
    if (!IsValid()) {
        set_last_error(EINVAL);
        return -1;
    }
    const int fileDescriptor = GetFileDescriptor();

    if (m_BufferSize == 0) {
        return pwrite(fileDescriptor, buffer, size, position);
    }
    if (m_Buffer == nullptr)
    {
        try {
            m_Buffer = new uint8_t[m_BufferSize];
        }
        catch (std::bad_alloc& cExc) {
            errno = ENOMEM;
            return -1;
        }
    }
    ssize_t bytesWritten = 0;

    if (size > m_BufferSize * 2)
    {
        ssize_t preSize = size - m_BufferSize;
        if (m_ValidBufferSize == 0 || position + preSize <= m_BufferPosition || position > m_BufferPosition + m_ValidBufferSize)
        {
            ssize_t currentBytesWritten = pwrite(fileDescriptor, buffer, preSize, position);
            if (currentBytesWritten < 0) {
                return currentBytesWritten;
            }
            bytesWritten    += currentBytesWritten;
            position        += currentBytesWritten;
            size            -= currentBytesWritten;
            buffer          = ((const uint8_t*)buffer) + currentBytesWritten;
        }
    }
    if (position < m_BufferPosition || position > m_BufferPosition + m_ValidBufferSize)
    {
        if (m_Dirty)
        {
            if (!Flush()) {
                return -1;
            }
        }
        m_BufferPosition = position;
        m_ValidBufferSize = 0;
    }
    while (size > 0)
    {
        if (position >= m_BufferPosition + m_BufferSize)
        {
            if (m_Dirty)
            {
                if (!Flush()) {
                    return -1;
                }
            }
            m_BufferPosition = position;
            m_ValidBufferSize = 0;
        }
        off64_t bufferOffset = position - m_BufferPosition;
        ssize_t currentLength = ssize_t(m_BufferSize - bufferOffset);
        if (currentLength > size) {
            currentLength = size;
        }
        memcpy(m_Buffer + bufferOffset, buffer, currentLength);
        if (bufferOffset + currentLength > m_ValidBufferSize) {
            m_ValidBufferSize = size_t(bufferOffset + currentLength);
        }
        bytesWritten += currentLength;
        position += currentLength;
        size -= currentLength;
        buffer = ((const uint8_t*)buffer) + currentLength;
        m_Dirty = true;
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off64_t File::GetSize(bool updateCache) const
{
    off64_t size = FSNode::GetSize(updateCache);

    if (size != -1 && m_ValidBufferSize > 0 && m_BufferPosition + m_ValidBufferSize > size) {
        size = m_BufferPosition + m_ValidBufferSize;
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// Move the file pointer.
/// \par Description:
/// \par Note:
/// \par Warning:
/// \param
/// \return
/// \par Error codes:
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

off64_t File::Seek(off64_t position, int mode)
{
    switch (mode)
    {
        case SEEK_SET:
            if (position < 0) {
                errno = EINVAL;
                return -1;
            }
            m_Position = position;
            return(m_Position);
        case SEEK_CUR:
            if (m_Position + position < 0) {
                errno = EINVAL;
                return -1;
            }
            m_Position += position;
            return m_Position;
        case SEEK_END:
        {
            off64_t size = GetSize();
            if (size + position < 0) {
                errno = EINVAL;
                return -1;
            }
            m_Position = size + position;
            return m_Position;
        }
        default:
            errno = EINVAL;
            return -1;
    }
}
