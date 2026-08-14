// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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

#include "FileUtilityHelpers.h"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <unistd.h>

#include <dirent.h>

#include <Storage/Path.h>


namespace shutil
{

bool WriteAll(int fileDescriptor, std::string_view text)
{
    size_t bytesWritten = 0;

    while (bytesWritten < text.size())
    {
        const ssize_t result = write(
            fileDescriptor,
            text.data() + bytesWritten,
            text.size() - bytesWritten);

        if (result < 0)
        {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (result == 0) {
            return false;
        }
        bytesWritten += static_cast<size_t>(result);
    }
    return true;
}

PString TrimTrailingSlashes(PString path)
{
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

PString GetBaseName(const PString& inputPath)
{
    const PString path = TrimTrailingSlashes(inputPath);
    const size_t separator = path.find_last_of('/');

    if (separator == PString::npos) {
        return path;
    }
    return path.substr(separator + 1);
}

PString GetParentPath(const PString& inputPath)
{
    const PString path = TrimTrailingSlashes(inputPath);
    const size_t separator = path.find_last_of('/');

    if (separator == PString::npos) {
        return PString::zero;
    }
    if (separator == 0) {
        return "/";
    }
    return path.substr(0, separator);
}

PString MakeChildPath(const PString& parentPath, const PString& childName)
{
    if (parentPath == "/") {
        return parentPath + childName;
    }
    return TrimTrailingSlashes(parentPath) + "/" + childName;
}

PString GetAbsolutePath(const PString& path)
{
    return PPath(path).GetPath();
}

bool IsPathWithin(const PString& possibleParent, const PString& possibleChild)
{
    const PString parent = GetAbsolutePath(possibleParent);
    const PString child = GetAbsolutePath(possibleChild);

    if (parent == child) {
        return true;
    }
    if (parent == "/") {
        return !child.empty() && child.front() == '/';
    }
    return child.size() > parent.size() &&
           child.compare(0, parent.size(), parent) == 0 &&
           child[parent.size()] == '/';
}

bool ReadNodeStat(const PString& path, stat_t& statBuffer, bool followSymlinks)
{
    const int openFlags = O_PATH | (followSymlinks ? 0 : O_NOFOLLOW);
    const int fileDescriptor = open(path.c_str(), openFlags);

    if (fileDescriptor == -1) {
        return false;
    }

    const int result = fstat(fileDescriptor, &statBuffer);
    const int errorCode = errno;
    close(fileDescriptor);
    errno = errorCode;
    return result == 0;
}

bool PathExists(const PString& path, stat_t* outStatBuffer)
{
    stat_t statBuffer;
    const bool exists = ReadNodeStat(path, statBuffer, false);

    if (exists && outStatBuffer != nullptr) {
        *outStatBuffer = statBuffer;
    }
    return exists;
}

bool IsDirectory(const PString& path, bool followSymlinks)
{
    stat_t statBuffer;
    return ReadNodeStat(path, statBuffer, followSymlinks) &&
           S_ISDIR(statBuffer.st_mode);
}

bool ReadDirectoryEntries(
    const PString& path,
    std::vector<PString>& entries,
    int& outErrorCode,
    bool followSymlink)
{
    const int directoryHandle = open(
        path.c_str(),
        O_RDONLY | O_DIRECTORY | (followSymlink ? 0 : O_NOFOLLOW));

    if (directoryHandle == -1)
    {
        outErrorCode = errno;
        return false;
    }

    dirent_t directoryEntry;

    for (;;)
    {
        const ssize_t readResult = posix_getdents(
            directoryHandle,
            &directoryEntry,
            sizeof(directoryEntry),
            0);

        if (readResult == 0) {
            break;
        }
        if (readResult != static_cast<ssize_t>(sizeof(directoryEntry)))
        {
            outErrorCode = (readResult < 0) ? errno : EIO;
            close(directoryHandle);
            return false;
        }
        if (PString::is_dot_or_dot_dot(
            directoryEntry.d_name,
            directoryEntry.d_namlen)) {
            continue;
        }

        entries.emplace_back(
            directoryEntry.d_name,
            directoryEntry.d_namlen);
    }

    close(directoryHandle);
    outErrorCode = 0;
    return true;
}

bool ReadSymlinkTarget(
    const PString& path,
    const stat_t& statBuffer,
    PString& target,
    int& outErrorCode)
{
    const int fileDescriptor = open(path.c_str(), O_PATH | O_NOFOLLOW);

    if (fileDescriptor == -1)
    {
        outErrorCode = errno;
        return false;
    }

    const size_t bufferSize = std::max<size_t>(
        static_cast<size_t>(statBuffer.st_size) + 1,
        64);
    std::vector<char> buffer(bufferSize);
    const ssize_t result = readlinkat(
        fileDescriptor,
        "",
        buffer.data(),
        buffer.size());
    outErrorCode = errno;
    close(fileDescriptor);

    if (result < 0) {
        return false;
    }
    if (static_cast<size_t>(result) == buffer.size())
    {
        outErrorCode = ENAMETOOLONG;
        return false;
    }

    target.assign(buffer.data(), static_cast<size_t>(result));
    outErrorCode = 0;
    return true;
}

bool Confirm(const PString& prompt)
{
    WriteAll(STDERR_FILENO, prompt);

    char response = '\0';
    char character = '\0';

    for (;;)
    {
        const ssize_t result = read(STDIN_FILENO, &character, 1);

        if (result < 0)
        {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (result == 0 || character == '\n') {
            break;
        }
        if (response == '\0') {
            response = character;
        }
    }

    return response == 'y' || response == 'Y';
}

bool ParseOctalMode(const std::string& text, mode_t& mode)
{
    if (text.empty()) {
        return false;
    }

    mode_t result = 0;

    for (const char character : text)
    {
        if (character < '0' || character > '7') {
            return false;
        }
        if (result > (std::numeric_limits<mode_t>::max() >> 3)) {
            return false;
        }
        result = static_cast<mode_t>((result << 3) | (character - '0'));
    }

    if (result > 07777) {
        return false;
    }
    mode = result;
    return true;
}

} // namespace shutil
