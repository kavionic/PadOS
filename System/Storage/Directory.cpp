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
#include <limits.h>
#include <sys/syslimits.h>
#include <Storage/Directory.h>
#include <Storage/FileReference.h>
#include <Storage/File.h>
#include <Storage/Symlink.h>
#include <Storage/Path.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFilesystem.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// Default contructor.
/// \par Description:
///     Initiate the instance to a know but invalid state.
///     The instance must be successfully initialized with one
///     of the Open() or SetTo() members before it can be used.
///
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

Directory::Directory()
{
}

///////////////////////////////////////////////////////////////////////////////
/// Construct a directory from a path.
/// \par Description:
///     See: Open( const String& path, int openFlags )
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

Directory::Directory(const String& path, int openFlags) : FSNode(path, openFlags)
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
///     See: Open( const Directory& directory, const String& path, int openFlags )
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

Directory::Directory(const Directory& directory, const String& path, int openFlags) : FSNode(directory, path, openFlags)
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

Directory::Directory(const FileReference& fileReference, int openFlags) : FSNode(fileReference, openFlags)
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

Directory::Directory(const FSNode& node) : FSNode(node)
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

Directory::Directory(const Directory& directory) : FSNode(directory)
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

Directory::Directory(int fileDescriptor, bool takeOwnership) : FSNode(fileDescriptor, takeOwnership)
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

Directory::~Directory()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::FDChanged(int newFileDescriptor, const struct ::stat& statBuffer)
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
/// \f status_t Directory::Open( const String& path, int openFlags = O_RDONLY )
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

bool Directory::GetPath(String& outPath) const
{
    std::vector<char> buffer;
    buffer.resize(PATH_MAX);

    int pathLength = FileIO::GetDirectoryPath(GetFileDescriptor(), buffer.data(), buffer.size());
    if (pathLength == PATH_MAX) {
        errno = ENAMETOOLONG;
        return false;
    }
    if (pathLength < 0) {
        return(pathLength);
    }
    outPath.assign(buffer.data(), pathLength);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::GetNextEntry(String& outName)
{
    dirent_t entry;
    if (FileIO::ReadDirectory(GetFileDescriptor(), &entry, sizeof(entry)) != 1) {
        return false;
    }
    outName = entry.d_name;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::GetNextEntry(FileReference& outReference)
{
    String name;
    if (!GetNextEntry(name)) {
        return false;
    }
    return outReference.SetTo(*this, name, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::Rewind()
{
    return FileIO::RewindDirectory(GetFileDescriptor()) != -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::CreateFile(const String& name, File& outFile, int accessMode)
{
    int file = FileIO::Open(GetFileDescriptor(), name.c_str(), O_WRONLY | O_CREAT, accessMode);
    if (file < 0) {
        return false;
    }
    return outFile.SetTo(file, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::CreateDirectory(const String& name, Directory& outDirectory, int accessMode)
{
    if (FileIO::CreateDirectory(GetFileDescriptor(), name.c_str(), accessMode) < 0) {
        return false;
    }
    return outDirectory.Open(*this, name, O_RDONLY);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::CreatePath(const String& path, bool includeLeaf, Directory* outLeafDirectory, int accessMode)
{
    if (path.empty() || path[0] == '/')
    {
        set_last_error(EINVAL);
        return false;
    }
    const char* nameStart = path.c_str();

    Directory parent = *this;
    for (;;)
    {
        const char* nameEnd = strchr(nameStart, '/');
        String name;
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
        Directory directory(parent, name);
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

bool Directory::CreateSymlink(const String& name, const String& destinationPath, SymLink& outLink)
{
    PErrorCode result = FileIO::Symlink(destinationPath.c_str(), GetFileDescriptor(), name.c_str());
    if (result != PErrorCode::Success) {
        set_last_error(result);
        return false;
    }
    return outLink.Open(*this, name, O_RDONLY);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Directory::Unlink(const String& name)
{
    return FileIO::Unlink(GetFileDescriptor(), name.c_str()) != -1;
}
