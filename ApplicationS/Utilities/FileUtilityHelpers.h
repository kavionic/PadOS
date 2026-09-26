// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <sys/pados_types.h>
#include <sys/stat.h>

#include <Utils/String.h>


namespace shutil
{

bool WriteAll(int fileDescriptor, std::string_view text);
PString TrimTrailingSlashes(PString path);
PString GetBaseName(const PString& inputPath);
PString GetParentPath(const PString& inputPath);
PString MakeChildPath(const PString& parentPath, const PString& childName);
PString GetAbsolutePath(const PString& path);
bool IsPathWithin(const PString& possibleParent, const PString& possibleChild);
bool ReadNodeStat(
    const PString& path,
    stat_t& statBuffer,
    bool followSymlinks = false);
bool PathExists(const PString& path, stat_t* outStatBuffer = nullptr);
bool IsDirectory(const PString& path, bool followSymlinks = false);
bool ReadDirectoryEntries(
    const PString& path,
    std::vector<PString>& entries,
    int& outErrorCode,
    bool followSymlink = false);
bool ReadSymlinkTarget(
    const PString& path,
    const stat_t& statBuffer,
    PString& target,
    int& outErrorCode);
bool Confirm(const PString& prompt);
bool ParseOctalMode(const std::string& text, mode_t& mode);

} // namespace shutil
