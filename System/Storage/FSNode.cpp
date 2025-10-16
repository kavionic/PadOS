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

#include <unistd.h>
#include <assert.h>
#include <sys/pados_syscalls.h>

#include <Storage/FSNode.h>
#include <Storage/FileReference.h>
#include <Storage/Directory.h>
#include <Storage/Path.h>
//#include <Kernel/VFS/KFilesystem.h>

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// Default constructor.
/// \par Description:
///     Initiate the FSNode to a known but "invalid" state.
///     The node must be initialize with one of the Open() or SetTo()
///     methods before any other methods can be called.
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

FSNode::FSNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a FSNode from a file path.
/// \par Description:
///     See: Open(const String& path, int openFlags)
/// \par Note:
///     Since constructors can't return error codes it will throw an
///     os::errno_exception in the case of failure. The error code can
///     be retrieved from the exception object.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

FSNode::FSNode(const String& path, int openFlags)
{
    Open(path, openFlags);
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a FSNode from directory and a name inside that directory.
/// \par Description:
///     See: Open( const Directory& directory, const String& path, int openFlags )
/// \par Note:
///     Since constructors can't return error codes it will throw an
///     os::errno_exception in the case of failure. The error code can
///     be retrieved from the exception object.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

FSNode::FSNode(const Directory& directory, const String& path, int openFlags)
{
    Open(directory, path, openFlags);
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a FSNode from a file reference.
/// \par Description:
///  See: Open( const FileReference& reference, int openFlags )
/// \par Note:
///  Since constructors can't return error codes it will throw an
///  os::errno_exception in the case of failure. The error code can
///  be retrieved from the exception object.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

FSNode::FSNode(const FileReference& reference, int openFlags)
{
    Open(reference, openFlags);
}

///////////////////////////////////////////////////////////////////////////////
///* Construct a FSNode from a file descriptor.
/// \par Description:
///     See: Open(int fileDescriptor)
/// \par Note:
///     Since constructors can't return error codes it will throw an
///     os::errno_exception in the case of failure. The error code can
///     be retrieved from the exception object.
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

FSNode::FSNode(int fileDescriptor, bool takeOwnership)
{
    if (!takeOwnership && fileDescriptor != -1) {
        fileDescriptor = dup(fileDescriptor);
    }
    m_FileDescriptor = fileDescriptor;

    if (fstat(m_FileDescriptor, &m_StatCache) < 0)
    {
        close(m_FileDescriptor);
        m_FileDescriptor = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
///* Copy constructor
/// \par Description:
///     Copy an existing node. If the node can't be copied an
///     os::errno_exception will be thrown. Each node consume
///     a file-descriptor so running out of FD's will cause
///     the copy to fail.
/// \par Note:
///     The attribute directory iterator will not be cloned so when
///     FSNode::GetNextAttrName() is called for the first time it will
///     return the first attribute name even if the iterator was not
///     located at the beginning of the originals attribute directory.
/// \param node
///     The node to copy.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

FSNode::FSNode(const FSNode& node)
{
    m_FileDescriptor = dup(node.m_FileDescriptor);
    m_StatCache = node.m_StatCache;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

FSNode::FSNode(FSNode&& node)
{
    m_FileDescriptor = node.m_FileDescriptor;
    m_StatCache = node.m_StatCache;
    node.m_FileDescriptor = -1;
}

///////////////////////////////////////////////////////////////////////////////
/// Destructor
/// \par Description:
///     Will close the file descriptor and release other resources may
///     consumed by the FSNode.
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

FSNode::~FSNode()
{
//    if (m_hAttrDir != nullptr) {
//        close_attrdir(m_hAttrDir);
//    }
    if (m_FileDescriptor != -1) {
        close(m_FileDescriptor);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Check if the node has been properly initialized.
/// \par Description:
///     Return true if the the object actually reference
///     a real FS node. All other access functions will fail
///     if the object is not fully initialized either through
///     one of the non-default constructors or with one of the
///     Open() or SetTo() members.
///
/// \return
///     True if the object is fully initialized false otherwise.
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool FSNode::IsValid() const
{
    return m_FileDescriptor >= 0;
}

///////////////////////////////////////////////////////////////////////////////
///* Open a node using a path.
/// \par Description:
///     Open a node by path. The path must be valid and the
///     process must have access to it but it can point to
///     any kind of FS-node (file, directory, symlink).
///
///     The path can start with "~/" to make it relative to the
///     current users home directory or it can start with "^/"
///     to make it relative to the directory where the executable
///     our application was started from lives in. This path
///     expansion is performed by the os::FSNode class itself
///     and is not part of the low-level file system.
///
///     The \p openFlags should be a combination of any of the O_*
///     flags defined in <fcntl.h>. Their meaning is the same as when
///     opening a file with the open() POSIX function except you can
///     not create a file by setting the O_CREAT flag.
///
///     The following flags are accepted:
///
///     - O_RDONLY  open the node read-only
///     - O_WRONLY  open the node write-only
///     - O_RDWR    open the node for both reading and writing
///     - O_TRUNC   truncate the size to 0 (only valid for files)
///     - O_APPEND  automatically move the file-pointer to the end
///             of the file before each write (only valid for files)
///     - O_NONBLOCK    open the file in non-blocking mode
///     - O_DIRECTORY   fail if \p path don't point at a directory
///     - O_NOFOLLOW  open the symlink it self rather than it's target
///             if \p path points at a symlink
///
/// \par Note:
///     If this call fail the old state of the FSNode will remain
///     unchanged
/// \param path
///     Path pointing at the node to open.
/// \param openFlags
///     Flags controlling how to open the node.
/// \return
///     On success 0 is returned. On error -1 is returned and a
///     error code is assigned to the global variable "errno".
///     The error code can be any of the errors returned by
///     the open() POSIX function.
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool FSNode::Open(const String& path, int openFlags)
{
    int newFileDescriptor = -1;
    if (path.size() > 1 && path[0] == '~' && path[1] == '/')
    {
        const char* home = getenv("HOME");
        if (home == nullptr)
        {
            errno = ENOENT;
            return false;
        }
        String realPath = home;
        realPath.insert(realPath.end(), path.begin() + 1, path.end());
        newFileDescriptor = open(realPath.c_str(), openFlags);
    }
    /*else if (path.size() > 1 && path[0] == '^' && path[1] == '/')
    {
        int nDir = open_image_file(IMGFILE_BIN_DIR);
        if (nDir < 0) {
            return -1;
        }
        newFileDescriptor = based_open(nDir, path.c_str() + 2, openFlags);
        int nOldErr = errno;
        close(nDir);
        if (newFileDescriptor < 0) {
            errno = nOldErr;
        }
    }*/
    else
    {
        newFileDescriptor = open(path.c_str(), openFlags);
    }

    if (newFileDescriptor < 0) {
        return false;
    }
    struct ::stat sStat;
    if (fstat(newFileDescriptor, &sStat) < 0)
    {
        int savedErrno = errno;
        close(newFileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (!FDChanged(newFileDescriptor, sStat))
    {
        int savedErrno = errno;
        close(newFileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (m_FileDescriptor >= 0) {
        close(m_FileDescriptor);
    }
    m_StatCache = sStat;
    m_FileDescriptor = newFileDescriptor;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Open a node using a directory/path pair.
/// \par Description:
///     Open a node by using a directory and a path relative to
///     that directory.
///
///     The path can either be absolute (\p directory will then be
///     ignored) or it can be relative to \p directory. This have much the
///     same semantics as setting the current working directory to \p
///     directory and then open the node by calling Open( const
///     String& path, int openFlags ) with the path. The main
///     advantage with this function is that it is thread-safe. You
///     don't get any races while temporarily changing the current
///     working directory.
///
///     For a more detailed description look at:
///     Open( const String& path, int openFlags )
///
/// \note
///     If this call fail the old state of the FSNode will remain
///     unchanged
/// \param directory
///     A valid directory from which the \p path is relative to.
/// \param path
///     The file path relative to \p directory. The path can either be
///     absolute (in which case \p directory is ignored) or it can
///     be relative to \p directory.
/// \param openFlags
///     Flags controlling how to open the node. See
///     Open( const String& path, int openFlags )
///     for a full description of the various flags.
///
/// \return
///     On success 0 is returned. On error -1 is returned and a
///     error code is assigned to the global variable "errno".
///     The error code can be any of the errors returned by
///     the open() POSIX function.
///
/// \sa FSNode( const String& path, int openFlags )
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool FSNode::Open(const Directory& directory, const String& path, int openFlags)
{
    if (!directory.IsValid())
    {
        errno = EINVAL;
        return false;
    }
    int newFileDescriptor = openat(directory.GetFileDescriptor(), path.c_str(), openFlags);
    if (newFileDescriptor < 0) {
        return false;
    }
    struct ::stat statBuffer;
    if (fstat(newFileDescriptor, &statBuffer) < 0)
    {
        int savedErrno = errno;
        close(newFileDescriptor);
        errno = savedErrno;
        return false;
    }

    if (!FDChanged(newFileDescriptor, statBuffer))
    {
        int savedErrno = errno;
        close(newFileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (m_FileDescriptor >= 0) {
        close(m_FileDescriptor);
    }
    m_StatCache = statBuffer;
    m_FileDescriptor = newFileDescriptor;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Open the node referred to by the given os::FileReference.
/// \par Description:
///     Same semantics Open( const String& path, int openFlags )
///     except that the node to open is targeted by a file reference
///     rather than a regular path.
/// \par Note:
///     If this call fail the old state of the FSNode will remain
///     unchanged
/// \return
///     On success 0 is returned. On error -1 is returned and a
///     error code is assigned to the global variable "errno".
///     The error code can be any of the errors returned by
///     the open() POSIX function.
/// \sa Open( const String& path, int openFlags )
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool FSNode::Open(const FileReference& reference, int openFlags)
{
    if (!reference.IsValid())
    {
        errno = EINVAL;
        return false;
    }
    int newFileDescriptor = openat(reference.GetDirectory().GetFileDescriptor(), reference.GetName().c_str(), openFlags);
    if (newFileDescriptor < 0) {
        return false;
    }
    struct ::stat statBuffer;
    if (fstat(newFileDescriptor, &statBuffer) < 0)
    {
        int savedErrno = errno;
        close(newFileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (!FDChanged(newFileDescriptor, statBuffer))
    {
        int savedErrno = errno;
        close(newFileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (m_FileDescriptor >= 0) {
        close(m_FileDescriptor);
    }
    m_StatCache = statBuffer;
    m_FileDescriptor = newFileDescriptor;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Make the FSNode represent an already open file.
/// \par Description:
///     Make the FSNode represent an already open file.
/// \par Note:
///     If this call fail the old state of the FSNode will remain
///     unchanged
/// \return
///     On success 0 is returned. On error -1 is returned and a
///     error code is assigned to the global variable "errno".
///     The error code can be any of the errors returned by
///     the open() POSIX function.
/// \sa Open(const String& path, int openFlags)
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool FSNode::SetTo(int fileDescriptor, bool takeOwnership)
{
    struct ::stat statBuffer;
    if (fstat(fileDescriptor, &statBuffer) < 0)
    {
        int savedErrno = errno;
        close(fileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (!FDChanged(fileDescriptor, statBuffer))
    {
        int savedErrno = errno;
        close(fileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (m_FileDescriptor >= 0) {
        close(m_FileDescriptor);
    }
    m_StatCache = statBuffer;
    m_FileDescriptor = fileDescriptor;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///* Copy another FSNode
/// \par Description:
///     Make this node a clone of \p node.
/// \par Note:
///     If this call fail the old state of the FSNode will remain
///     unchanged
/// \param node
///     The FSNode to copy.
///
/// \return
///     On success 0 is returned. On error -1 is returned and a
///     error code is assigned to the global variable "errno".
///     The error code can be any of the errors returned by
///     the open() POSIX function.
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool FSNode::SetTo(const FSNode& node)
{
    if (!node.IsValid())
    {
        Close();
        return true;
    }
    int newFileDescriptor = dup(node.m_FileDescriptor);
    if (newFileDescriptor < 0) {
        return false;
    }
    if (!FDChanged(newFileDescriptor, node.m_StatCache))
    {
        int savedErrno = errno;
        close(newFileDescriptor);
        errno = savedErrno;
        return false;
    }
    if (m_FileDescriptor >= 0) {
        close(m_FileDescriptor);
    }
    m_StatCache = node.m_StatCache;
    m_FileDescriptor = newFileDescriptor;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool FSNode::SetTo(FSNode&& node)
{
    if (!node.IsValid())
    {
        Close();
        return true;
    }
    if (!FDChanged(node.m_FileDescriptor, node.m_StatCache)) {
        return false;
    }
    if (m_FileDescriptor >= 0) {
        close(m_FileDescriptor);
    }
    m_StatCache             = node.m_StatCache;
    m_FileDescriptor        = node.m_FileDescriptor;
    node.m_FileDescriptor   = -1;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Reset the FSNode
/// \par Description:
///     Will close the file descriptor and other resources may
///     consumed by the FSNode. The IsValid() member will return false
///     until the node is reinitialized with one of the Open() or SetTo()
///     methods.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

void FSNode::Close()
{
//    if (m_hAttrDir != nullptr) {
//        close_attrdir(m_hAttrDir);
//        m_hAttrDir = nullptr;
//    }
    FDChanged(-1, m_StatCache);
    if (m_FileDescriptor != -1) {
        close(m_FileDescriptor);
        m_FileDescriptor = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FSNode::FDChanged(int newFileDescriptor, const struct ::stat& statBuffer)
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FSNode::GetStat(struct ::stat* statBuffer, bool updateCache) const
{
    if (m_FileDescriptor < 0) {
        errno = EINVAL;
        return false;
    }
    if (updateCache) {
        status_t nError = fstat(m_FileDescriptor, &m_StatCache);
        if (nError < 0) {
            return false;
        }
    }
    if (statBuffer != nullptr) {
        *statBuffer = m_StatCache;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FSNode::SetStats(const struct stat& statBuffer, uint32_t mask) const
{
    if (m_FileDescriptor < 0) {
        errno = EINVAL;
        return false;
    }
    const PErrorCode result = __write_stat(m_FileDescriptor, &statBuffer, mask);
    if (result != PErrorCode::Success)
    {
        set_last_error(result);
        return false;
    }
    if (mask & WSTAT_MODE)  m_StatCache.st_mode  = statBuffer.st_mode;
    if (mask & WSTAT_UID)   m_StatCache.st_uid   = statBuffer.st_uid;
    if (mask & WSTAT_GID)   m_StatCache.st_gid   = statBuffer.st_gid;
    if (mask & WSTAT_SIZE)  m_StatCache.st_size  = statBuffer.st_size;
    if (mask & WSTAT_ATIME) m_StatCache.st_atim  = statBuffer.st_atim;
    if (mask & WSTAT_MTIME) m_StatCache.st_mtim  = statBuffer.st_mtim;
    if (mask & WSTAT_CTIME) m_StatCache.st_ctim  = statBuffer.st_ctim;

    if (mask & ~(WSTAT_MODE  |
                 WSTAT_UID   |
                 WSTAT_GID   |
                 WSTAT_SIZE  |
                 WSTAT_ATIME |
                 WSTAT_MTIME |
                 WSTAT_CTIME)) {
        printf("ERROR: FSNode::SetStats() called with unknown mask bits: %08lx\n", mask);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ino_t FSNode::GetInode() const
{
    if (m_FileDescriptor < 0) {
        errno = EINVAL;
        return -1;
    }
    return m_StatCache.st_ino;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

dev_t FSNode::GetDev() const
{
    if (m_FileDescriptor < 0) {
        errno = EINVAL;
        return -1;
    }
    return m_StatCache.st_dev;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FSNode::GetMode(bool updateCache) const
{
    if (m_FileDescriptor < 0) {
        errno = EINVAL;
        return -1;
    }
    if (updateCache)
    {
        if (fstat(m_FileDescriptor, &m_StatCache) < 0) {
            return -1;
        }
    }
    return m_StatCache.st_mode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off64_t FSNode::GetSize(bool updateCache) const
{
    if (m_FileDescriptor < 0) {
        errno = EINVAL;
        return -1;
    }
    if (updateCache) {
        if (fstat(m_FileDescriptor, &m_StatCache) < 0) {
            return -1;
        }
    }
    return m_StatCache.st_size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FSNode::SetSize(off64_t size) const
{
    if (m_FileDescriptor < 0) {
        errno = EINVAL;
        return false;
    }
    struct stat stats;
    stats.st_size = off_t(size);
    return SetStats(stats, WSTAT_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FSNode::GetCTime(TimeValNanos& outTime, bool updateCache) const
{
    if (m_FileDescriptor < 0) {
        return PErrorCode::InvalidArg;
    }
    if (updateCache) {
        if (fstat(m_FileDescriptor, &m_StatCache) < 0) {
            return PErrorCode(errno);
        }
    }
    outTime = TimeValNanos::FromTimespec(m_StatCache.st_ctim);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FSNode::GetMTime(TimeValNanos& outTime, bool updateCache) const
{
    if (m_FileDescriptor < 0) {
        return PErrorCode::InvalidArg;
    }
    if (updateCache) {
        if (fstat(m_FileDescriptor, &m_StatCache) < 0) {
            return PErrorCode(errno);
        }
    }
    outTime = TimeValNanos::FromTimespec(m_StatCache.st_mtim);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FSNode::GetATime(TimeValNanos& outTime, bool updateCache) const
{
    if (m_FileDescriptor < 0) {
        return PErrorCode::InvalidArg;
    }
    if (updateCache) {
        if (fstat(m_FileDescriptor, &m_StatCache) < 0) {
            return PErrorCode(errno);
        }
    }
    outTime = TimeValNanos::FromTimespec(m_StatCache.st_atim);
    return PErrorCode::Success;
}

PErrorCode FSNode::GetCTime(time_t& outTime, bool updateCache) const
{
    TimeValNanos fileTime;
    PErrorCode result = GetCTime(fileTime, updateCache);
    if (result == PErrorCode::Success) {
        outTime = fileTime.AsSecondsI();
    }
    return result;
}

PErrorCode FSNode::GetMTime(time_t& outTime, bool updateCache /*= true*/) const
{
    TimeValNanos fileTime;
    PErrorCode result = GetMTime(fileTime, updateCache);
    if (result == PErrorCode::Success) {
        outTime = fileTime.AsSecondsI();
    }
    return result;
}

PErrorCode FSNode::GetATime(time_t& outTime, bool updateCache /*= true*/) const
{
    TimeValNanos fileTime;
    PErrorCode result = GetATime(fileTime, updateCache);
    if (result == PErrorCode::Success) {
        outTime = fileTime.AsSecondsI();
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
///* Read the node's attribute directory.
/// \par Description:
///     Iterate over the node's "attribute directory". Call this
///     member in sequence until it return "0" to iterate over all the
///     attributes associated with the node. The attribute iterator
///     can be reset to the first attribute with the RewindAttrdir()
///     member.
///
///     More info about the returned attributes can be obtain with the
///     StatAttr() member and the content of an attribute can be read with
///     the ReadAttr() member.
/// \par Note:
///     Currently only the AtheOS native filesystem (AFS) support
///     attributes so if the the file is not located on an AFS volume
///     this member will fail.
///
/// \param pcName
///     Pointer to an STL string that will receive the name.
/// \return
///     If a new name was successfully obtained 1 will be returned. If
///     we reach the end of the attribute directory 0 will be
///     returned.  Any other errors will cause -1 to be returned and a
///     error code will be stored in the global "errno" variable.
/// \sa StatAttr(), ReadAttr(), WriteAttr()
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

//status_t FSNode::GetNextAttrName(String* pcName)
//{
//    if (m_FileDescriptor < 0) {
//        errno = EINVAL;
//        return(-1);
//    }
//    if (m_hAttrDir == nullptr) {
//        m_hAttrDir = open_attrdir(m_FileDescriptor);
//        if (m_hAttrDir == nullptr) {
//            return(-1);
//        }
//    }
//    struct dirent* psEntry = read_attrdir(m_hAttrDir);
//    if (psEntry == nullptr) {
//        return(0);
//    }
//    else {
//        *pcName = psEntry->d_name;
//        return(1);
//    }
//}

///////////////////////////////////////////////////////////////////////////////
///* Reset the attribute directory iterator
/// \par Description:
///     RewindAttrdir() will cause the next GetNextAttrName() call to
///     return the name of the first attribute associated with this
///     node.
/// \par Note:
///     Currently only the AtheOS native filesystem (AFS) support
///     attributes so if the the file is not located on an AFS volume
///     this member will fail.
/// \return
///     0 on success. On failure -1 is returned and a error code is stored
///     in the global variable "errno".
/// \sa GetNextAttrName()
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

//status_t FSNode::RewindAttrdir()
//{
//    if (m_FileDescriptor < 0) {
//        errno = EINVAL;
//        return(-1);
//    }
//    if (m_hAttrDir != nullptr) {
//        rewind_attrdir(m_hAttrDir);
//    }
//    return(0);
//}

///////////////////////////////////////////////////////////////////////////////
/// Add/update an attribute
/// \par Description:
///     WriteAttr() is used to create new attributes and update
///     existing attributes. A attribute is simply a chunc of data
///     that is associated with the file but that is not part of the
///     files regular data-stream. Attributes can be added to all
///     kind's of FS nodes like regular files, directories, and
///     symlinks.
///
///     A attribute can contain a untyped stream of data of an
///     arbritary length or it can contain typed data like integers,
///     floats, strings, etc etc. The reason for having typed data
///     is to be able to make a search index that can be used to
///     for efficient search for files based on their attributes.
///     The indexing system is not fully implemented yet but will
///     be part of AtheOS in the future.
///
/// \par Note:
///     Currently only the AtheOS native filesystem (AFS) support
///     attributes so if the the file is not located on an AFS volume
///     this member will fail.
/// \param cAttrName
///     The name of the attribute. The name must be unique inside the
///     scope of the node it belongs to. If an attribute already exists
///     with that name it will be overwritten.
/// \param nFlags
///     Currently only O_TRUNC is accepted. If you pass in O_TRUNC and a
///     attribute with the same name already exists it will be truncated
///     to a size of 0 before the new attribute data is written. By passing
///     in 0 you can update parts of or extend an existing attribute.
/// \param nType
///     The data-type of the attribute. This should be one of the ATTR_TYPE_*
///     types defined in <atheos/filesystem.h>.
///
///     - \b ATTR_TYPE_NONE,    Untyped "raw" data of any size.
///     - \b ATTR_TYPE_INT32,   32-bit integer value (the size must be exactly 4 bytes).
///     - \b ATTR_TYPE_INT64,   64-bit integer value often used for time-values (the size must be exactly 8 bytes).
///     - \b ATTR_TYPE_FLOAT,   32-bit floating point value (the size must be exactly 4 bytes).
///     - \b ATTR_TYPE_DOUBLE,  64-bit floating point value (the size must be exactly 8 bytes).
///     - \b ATTR_TYPE_STRING,  UTF8 string. The string should not be NUL terminated.
/// \param pBuffer
///     Pointer to the data to be written.
/// \param nPos
///     The offset into the attribute where the data will be written.
/// \param nLen
///     Number of bytes to be written.
///
/// \return
///     On success the number of bytes actually written is
///     returned. On failure -1 is returned and the error code is
///     stored in the global variable "errno"
///
/// \sa ReadAttr(), StatAttr()
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

//ssize_t FSNode::WriteAttr(const String& cAttrName, int nFlags, int nType, const void* pBuffer, off_t nPos, size_t nLen)
//{
//    if (m_FileDescriptor < 0) {
//        errno = EINVAL;
//        return(-1);
//    }
//    return(write_attr(m_FileDescriptor, cAttrName.c_str(), nFlags, nType, pBuffer, nPos, nLen));
//}

///////////////////////////////////////////////////////////////////////////////
/// Read the data held by an attribute.
/// \par Description:
///     Read an arbitrary chunk of an attributes data. Both the name
///     and the type must match for the operation so succeed. If you
///     don't know the type in advance it can be retrieved with the
///     StatAttr() member.
///
/// \par Note:
///     Currently only the AtheOS native file system (AFS) support
///     attributes so if the the file is not located on an AFS volume
///     this member will fail.
/// \param cAttrName
///     The name of the attribute to read.
/// \param nType
///     The expected attribute type. The attribute must be of this type
///     for the function to succede. See WriteAttr() for a more detailed
///     description of attribute types.
///
/// \return
///     On success the number of bytes actually read is returned. On
///     failure -1 is returned and the error code is stored in the
///     global variable "errno".
///
/// \sa WriteAttr(), StatAttr()
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////


//ssize_t FSNode::ReadAttr(const String& cAttrName, int nType, void* pBuffer, off_t nPos, size_t nLen)
//{
//    if (m_FileDescriptor < 0) {
//        errno = EINVAL;
//        return(-1);
//    }
//    return(read_attr(m_FileDescriptor, cAttrName.c_str(), nType, pBuffer, nPos, nLen));
//}

///////////////////////////////////////////////////////////////////////////////
///* Remove an attribute from an FS node.
/// \par Description:
///  This will remove the named attribute from the node itself and
///  if the attribute has been indexed it will also be removed from
///  the index.
/// \par Note:
///  Currently only the AtheOS native filesystem (AFS) support
///  attributes so if the the file is not located on an AFS volume
///  this member will fail.
/// \param cName
///  Name of the attribute to remove.
/// \return
///  On success 0 is returned. On failure -1 is returned and the
///  error code is stored in the global variable "errno".
/// \sa WriteAttr(), ReadAttr()
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////


//status_t FSNode::RemoveAttr(const String& cName)
//{
//    if (m_FileDescriptor < 0) {
//        errno = EINVAL;
//        return(-1);
//    }
//    return(remove_attr(m_FileDescriptor, cName.c_str()));
//}

///////////////////////////////////////////////////////////////////////////////
///* Get extended info about an attribute.
/// \par Description:
///  Call this function to retrieve the size and type of an
///  attribute.  For a detailed description of the attribute type
///  take a look at WriteAttr().
/// \par Note:
///  Currently only the AtheOS native filesystem (AFS) support
///  attributes so if the the file is not located on an AFS volume
///  this member will fail.
/// \param cName
///  The name of the attribute to examine.
/// \return
///  On success 0 is returned. On failure -1 is returned and the
///  error code is stored in the global variable "errno".
/// \sa WriteAttr(), ReadAttr()
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

//status_t FSNode::StatAttr(const String& cName, struct attr_info* psBuffer)
//{
//    if (m_FileDescriptor < 0) {
//        errno = EINVAL;
//        return(-1);
//    }
//    return(stat_attr(m_FileDescriptor, cName.c_str(), psBuffer));
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FSNode::GetFileDescriptor() const
{
    return m_FileDescriptor;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FSNode& FSNode::operator=(const FSNode& rhs)
{
    Close();
    SetTo(rhs);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FSNode& FSNode::operator=(FSNode&& rhs)
{
    Close();
    SetTo(std::forward<FSNode&&>(rhs));
    return *this;
}
