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
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <sys/syslimits.h>
#include <PadOS/Filesystem.h>

#include <System/ExceptionHandling.h>
#include <Storage/Directory.h>
#include <Storage/FileReference.h>
#include <Storage/File.h>
#include <Storage/Symlink.h>
#include <Storage/Path.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFilesystem.h>


///////////////////////////////////////////////////////////////////////////////
/// Default contructor.
/// \par Description:
///     Initiate the instance to a know but invalid state.
///     The instance must be successfully initialized with one
///     of the Open() or SetTo() members before it can be used.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

PDirectory::PDirectory()
{
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a directory from a path.
/// \par Description:
///     See: Open(const PString& path, int openFlags)
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

PDirectory::PDirectory(const PString& path, int openFlags) : PFSNode(path, openFlags)
{
    if (IsValid() && !IsDir())
    {
        Close();
        errno = ENOTDIR;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a directory from a path.
/// \par Description:
///     See: Open(const Directory& directory, const PString& path, int openFlags)
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

PDirectory::PDirectory(const PDirectory& directory, const PString& path, int openFlags) : PFSNode(directory, path, openFlags)
{
    if (IsValid() && !IsDir())
    {
        Close();
        errno = ENOTDIR;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a directory from a path
/// \par Description:
///     See: Open( const FileReference& fileReference, int openFlags )
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

PDirectory::PDirectory(const PFileReference& fileReference, int openFlags) : PFSNode(fileReference, openFlags)
{
    if (IsValid() && !IsDir())
    {
        Close();
        errno = ENOTDIR;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a directory from a path
/// \par Description:
///     See: SetTo(const FSNode& node)
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

PDirectory::PDirectory(const PFSNode& node) : PFSNode(node)
{
    if (IsValid() && !IsDir())
    {
        Close();
        errno = ENOTDIR;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Copy constructor
/// \par Description:
///  Make a independent copy of another directory.
///
///  The new directory will consume a new file descriptor so the
///  copy might fail (throwing an errno_exception exception) if the
///  process run out of file descriptors.
///
/// \param cDir
///  The directory to copy
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

PDirectory::PDirectory(const PDirectory& directory) : PFSNode(directory)
{
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a directory object from a open file descriptor.
/// \par Description:
///     Construct a directory object from a open file descriptor.
///     The file descriptor must be referencing a directory or
///     an errno_exception with the ENOTDIR error code will be thrown.
/// \param nFD
///     An open file descriptor referencing a directory.
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

PDirectory::PDirectory(int fileDescriptor, bool takeOwnership) : PFSNode(fileDescriptor, takeOwnership)
{
    if (IsValid() && !IsDir())
    {
        Close();
        errno = ENOTDIR;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDirectory::~PDirectory()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::FDChanged(int newFileDescriptor, const struct ::stat& statBuffer)
{
    if (newFileDescriptor >= 0 && !S_ISDIR(statBuffer.st_mode))
    {
        errno = ENOTDIR;
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// Open a directory using a path.
/// \f status_t Directory::Open(const PString& path, int openFlags = O_RDONLY)
/// \par Description:
///     Open the directory pointed to by \p path. The path must
///     be valid and it must point to a directory.
///
/// \param path
///     The directory to open.
/// \param openFlags
///     Flags describing how to open the directory. Only O_RDONLY,
///     O_WRONLY, and O_RDWR are relevant to directories. Take a look
///     at the os::FSNode documentation for a more detailed
///     description of open modes.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Get the absolute path of the directory
/// \par Description:
/// \par Note:
/// \par Warning:
/// \param
/// \return
/// \par Error codes:
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::GetPath(PString& outPath) const
{
    try
    {
        std::vector<char> buffer;
        buffer.resize(PATH_MAX);

        const PErrorCode result = get_directory_path(GetFileDescriptor(), buffer.data(), buffer.size());
        if (result != PErrorCode::Success)
        {
            set_last_error(result);
            return false;
        }
        outPath = buffer.data();
        return true;
    }
    PERROR_CATCH_SET_ERRNO(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::GetNextEntry(PString& outName)
{
    dirent_t entry;
    if (posix_getdents(GetFileDescriptor(), &entry, sizeof(entry), 0) != sizeof(entry)) {
        return false;
    }
    outName = entry.d_name;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::GetNextEntry(PFileReference& outReference)
{
    PString name;
    if (!GetNextEntry(name)) {
        return false;
    }
    return outReference.SetTo(*this, name, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::Rewind()
{
    return rewind_directory(GetFileDescriptor()) == PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::CreateFile(const PString& name, PFile& outFile, int accessMode)
{
    int file = openat(GetFileDescriptor(), name.c_str(), O_WRONLY | O_CREAT, accessMode);
    if (file < 0) {
        return false;
    }
    return outFile.SetTo(file, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::CreateDirectory(const PString& name, PDirectory& outDirectory, int accessMode)
{
    if (mkdirat(GetFileDescriptor(), name.c_str(), accessMode) < 0) {
        return false;
    }
    return outDirectory.Open(*this, name, O_RDONLY);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::CreatePath(const PString& path, bool includeLeaf, PDirectory* outLeafDirectory, int accessMode)
{
    if (path.empty() || path[0] == '/')
    {
        set_last_error(EINVAL);
        return false;
    }
    const char* nameStart = path.c_str();

    PDirectory parent = *this;
    for (;;)
    {
        const char* nameEnd = strchr(nameStart, '/');
        PString name;
        if (nameEnd != nullptr)
        {
            name.assign(nameStart, nameEnd);
            nameStart = nameEnd + 1;
            while (*nameStart == '/') nameStart++;
        }
        else
        {
            name = nameStart;
        }
        if (!includeLeaf && nameEnd == nullptr)
        {
            if (outLeafDirectory != nullptr) {
                *outLeafDirectory = std::move(parent);
            }
            return true;
        }
        PDirectory directory(parent, name);
        if (!directory.IsValid())
        {
            if (errno != ENOENT) {
                return false;
            }
            if (!parent.CreateDirectory(name, directory, accessMode)) {
                return false;
            }
        }
        if (nameEnd == nullptr)
        {
            if (outLeafDirectory != nullptr) {
                *outLeafDirectory = std::move(directory);
            }
            return true;
        }
        parent = std::move(directory);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::CreateSymlink(const PString& name, const PString& destinationPath, PSymLink& outLink)
{
    status_t result = symlinkat(destinationPath.c_str(), GetFileDescriptor(), name.c_str());
    if (result != 0) {
        return false;
    }
    return outLink.Open(*this, name, O_RDONLY);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDirectory::Unlink(const PString& name)
{
    return unlinkat(GetFileDescriptor(), name.c_str(), 0) != -1;
}
