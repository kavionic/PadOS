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

#include <argparse/argparse.hpp>

#include <Kernel/DebugConsole/KConsoleCommand.h>

namespace kernel
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

class CCmdLS : public KConsoleCommand
{
public:
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        argparse::ArgumentParser program(args[0], "1.0", argparse::default_arguments::none);

        program.add_argument("--help")
            .help("Print argument help.")
            .flag();

        program.add_argument("-a", "--all")
            .help("Do not ignore entries starting with .")
            .flag();

        program.add_argument("-A", "--almost-all")
            .help("Do not list implied . and ..")
            .flag();

        program.add_argument("--block-size")
            .help("With -l, scale sizes by SIZE when printing them; e.g., '--block-size=M'; see SIZE format below.");

        program.add_argument("-C", "--color")
            .help("Colorize the output; WHEN can be 'always' (default if omitted), 'auto', or 'never'.");

        program.add_argument("-d", "--directory")
            .help("List directories themselves, not their contents.")
            .flag();

        program.add_argument("-F", "--classify")
            .help("Append indicator (one of */=>@|) to entries.")
            .flag();

        program.add_argument("--file-type")
            .help("like --classify, except do not append '*'.")
            .flag();

        program.add_argument("-h", "--human-readable")
            .help("With -l and -s, print sizes like 1K 234M 2G etc.")
            .flag();

        program.add_argument("--si")
            .help("Like --human-readable, but use powers of 1000 not 1024.")
            .flag();

        program.add_argument("-l")
            .help("Use a long listing format.")
            .flag();

        program.add_argument("files").remaining();

        try
        {
            program.parse_args(args);
        }
        catch (const std::exception& exc)
        {
            Print("{}\n", exc.what());
            Print("{}", program.help().str());
            return 1;
        }

        if (program.get<bool>("--help"))
        {
            Print("{}", program.help().str());
            return 0;
        }

        if (program.is_used("--block-size"))
        {
            const std::string& valueStr = program.get("--block-size");
            std::optional<int64_t> value = PString::parse_byte_size(valueStr, PUnitSystem::Auto);
            if (value.has_value())
            {
                m_BlockSize = value.value();
            }
            else
            {
                Print("{}: invalid --block-size argument '{}'\n", args[0], valueStr);
                return 1;
            }
        }

        m_ColorMode = EColorMode::Always;

        if (program.is_used("--color"))
        {
            const std::string& value = program.get("--color");

            if (value.empty() || value == "always" || value == "yes" || value == "force") {
                m_ColorMode = EColorMode::Always;
            } else if (value == "auto" || value == "tty" || value == "if-tty") {
                m_ColorMode = EColorMode::Auto;
            } else if (value == "never" || value == "no" || value == "none") {
                m_ColorMode = EColorMode::Never;
            }
            else
            {
                Print("{0}: invalid argument ‘{1}’ for ‘--color’\n"
                      "Valid arguments are:\n"
                      "  - ‘always’, ‘yes’, ‘force’\n"
                      "  - ‘never’, ‘no’, ‘none’\n"
                      "  - ‘auto’, ‘tty’, ‘if-tty’\n"
                      "Try '{0} --help' for more information.\n", args[0], value);

                return 1;
            }
        }

        m_FilesToShow = EFilesToShow::Normal;

        if (program.get<bool>("--all")) {
            m_FilesToShow = EFilesToShow::All;
        } else if(program.get<bool>("--almost-all")) {
            m_FilesToShow = EFilesToShow::AlmostAll;
        }
        m_ListDirectories   = program.get<bool>("--directory");
        m_UseLongFormat     = program.get<bool>("-l");
        m_UnitSystem        = program.get<bool>("--si") ? PUnitSystem::SI : PUnitSystem::IEC;
        m_HumanReadable     = m_UnitSystem == PUnitSystem::SI || program.get<bool>("--human-readable");
        m_Classify          = program.get<bool>("--classify");
        m_FileType          = program.get<bool>("--file-type");

        std::vector<std::string> files = program.is_used("files") ? program.get<std::vector<std::string>>("files") : std::vector<std::string>({ "." });

        for (size_t i = 0; i < files.size(); ++i)
        {
            const std::string& path = files[i];

            if (!m_ListDirectories && i != 0) {
                Print("\n");
            }
            ListDirectory(path);
        }

        return 0;
    }

    virtual PString GetDescription() const override { return "Print information about running threads."; }

private:
    void ListDirectory(const std::string& path)
    {
        std::vector<std::pair<PString, struct stat>> files;

        int directory = open(path.c_str(), O_RDONLY);
        if (directory != -1)
        {
            struct stat nodeStats;

            if (fstat(directory, &nodeStats) != -1)
            {
                bool printHeader = false;
                if (m_ListDirectories || !S_ISDIR(nodeStats.st_mode))
                {
                    files.push_back(std::make_pair(path, nodeStats));
                }
                else
                {
                    printHeader = path != ".";
                    dirent_t entry;

                    while (kread_directory(directory, &entry, sizeof(entry)) == sizeof(entry))
                    {
                        switch (m_FilesToShow)
                        {
                            case EFilesToShow::Normal:
                                if (entry.d_name[0] == '.') {
                                    continue;
                                }
                                break;
                            case EFilesToShow::AlmostAll:
                                if (PString::is_dot_or_dot_dot(entry.d_name, entry.d_namlen)) {
                                    continue;
                                }
                            case EFilesToShow::All:
                                break;
                        }
                        int error = 0;
                        struct stat fileStat;
                        int fd = openat(directory, entry.d_name, O_RDONLY);
                        if (fd != -1)
                        {
                            if (fstat(fd, &fileStat) == -1)
                            {
                                error = errno;
                            }
                            close(fd);
                        }
                        else
                        {
                            error = errno;
                        }
                        if (error == 0) {
                            files.push_back(std::make_pair(entry.d_name, fileStat));
                        } else {
                            Print("Error: {} - {}", path, strerror(error));
                        }
                    }
                    std::sort(files.begin(), files.end(), [](const std::pair<PString, struct stat>& lhs, const std::pair<PString, struct stat>& rhs) { return lhs.first < rhs.first; });
                }
                if (printHeader)
                {
                    Print("{}:\n", path);
                }

                off_t totalSize = 0;
                for (size_t i = 0; i < files.size(); ++i)
                {
                    const struct stat& fileStat = files[i].second;
                    totalSize += fileStat.st_size;
                }

                if (m_UseLongFormat) {
                    Print("total {}\n", FormatFileSize(totalSize, true));
                }
                PrintFileList(files);
                close(directory);
            }
            else
            {
                Print("Error: {} - {}", path, strerror(errno));
            }
        }
    }

    void PrintFileList(const std::vector<std::pair<PString, struct stat>>& files)
    {
        for (size_t i = 0; i < files.size(); ++i)
        {
            const PString& fileName = files[i].first;
            const struct stat& fileStat = files[i].second;
            if (m_UseLongFormat)
            {
                char timeStr[64];
                std::strftime(timeStr, sizeof(timeStr), "%b %d %Y", std::localtime(&fileStat.st_mtime));

                Print("{} {:>8} {:12} {}\n", PString::format_file_permissions(fileStat.st_mode), FormatFileSize(fileStat.st_size, false), timeStr, FormatFilename(fileName, fileStat.st_mode));
            }
            else
            {
                if (i == 0) {
                    Print("{}", FormatFilename(fileName, fileStat.st_mode));
                } else {
                    Print(" {}", FormatFilename(fileName, fileStat.st_mode));
                }
            }
        }
        if (!m_UseLongFormat) {
            Print("\n");
        }
    }

    int GetFileColor(mode_t mode) const
    {
        switch (mode & S_IFMT)
        {
            case S_IFBLK: [[fallthrough]];
            case S_IFCHR: [[fallthrough]];
            case S_IFIFO:
                return int(PANSI_RenderProperty::FgColor_BrightYellow);
            case S_IFDIR:
                return int(PANSI_RenderProperty::FgColor_BrightBlue);
            case S_IFLNK:
                return int(PANSI_RenderProperty::FgColor_BrightCyan);
            case S_IFREG:
                if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                    return int(PANSI_RenderProperty::FgColor_BrightGreen);
                } else {
                    return int(PANSI_RenderProperty::Reset);
                }
            default:
                return int(PANSI_RenderProperty::Reset);
        }
    }

    PString FormatFilename(const PString& name, mode_t mode) const
    {
        PString prefix;
        if (m_Classify || m_FileType)
        {
            switch (mode & S_IFMT)
            {
                case S_IFIFO:   prefix = "|"; break;
                case S_IFDIR:   prefix = "/"; break;
                case S_IFLNK:   prefix = "@"; break;
                case S_IFSOCK:  prefix = "="; break;
                case S_IFREG:
                    if (!m_FileType && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
                        prefix = "*"; break;
                    }
                    break;
                default:
                    break;
            }
        }
        if (m_ColorMode != EColorMode::Never) // TODO: implement 'auto' when getting support for TTY's
        {
            int colorCode = GetFileColor(mode);
            if (PANSI_RenderProperty(colorCode) != PANSI_RenderProperty::Reset) {
                return PANSIEscapeCodeParser::FormatANSICode(PANSI_ControlCode::SetRenderProperty, colorCode) + prefix + name + PANSIEscapeCodeParser::FormatANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::Reset));
            }
        }
        return prefix + name;
    }
    
    PString FormatFileSize(off_t size, bool useBlocks) const
    {
        return m_HumanReadable ? PString::format_byte_size(size, -2, m_UnitSystem) : PString::format_string("{}", useBlocks ? (size / m_BlockSize) : size);
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

static KConsoleCommandRegistrator<CCmdLS> g_RegisterCCmdLS("ls");

} // namespace kernel
