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
// Created: 10.01.2026 15:30

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <unistd.h>

#include <sys/pados_types.h>
#include <sys/stat.h>

#include <Utils/String.h>


namespace shutil_ls
{

enum class EFilesToShow
{
    Normal,
    AlmostAll,
    All
};

enum class EColorMode
{
    Auto,
    Never,
    Always
};

struct FileEntry
{
    PString Name;
    PString LinkTarget;
    stat_t  StatBuffer;
};

class CCmdLS
{
public:
    int Invoke(int argc, char* argv[]);

private:
    void ListDirectory(const std::string& path);
    void PrintFileList(const std::vector<FileEntry>& files);
    int GetFileColor(mode_t mode) const;
    PString FormatFilename(const PString& name, mode_t mode) const;
    PString FormatFileSize(off_t size, bool useBlocks) const;

    template<typename ...ARGS>
    void Print(PFormatString<ARGS...>&& format, ARGS&&... arguments)
    {
        const PString text = PString::format_string(
            std::forward<PFormatString<ARGS...>>(format),
            std::forward<ARGS>(arguments)...);
        write(STDOUT_FILENO, text.c_str(), text.size());
    }

    EFilesToShow m_FilesToShow = EFilesToShow::Normal;
    EColorMode   m_ColorMode = EColorMode::Always;
    PUnitSystem  m_UnitSystem = PUnitSystem::IEC;
    int64_t      m_BlockSize = 1024;
    bool         m_ListDirectories = false;
    bool         m_UseLongFormat = false;
    bool         m_HumanReadable = false;
    bool         m_Classify = false;
    bool         m_FileType = false;
};

int ls_main(int argc, char* argv[]);

} // namespace shutil_ls
