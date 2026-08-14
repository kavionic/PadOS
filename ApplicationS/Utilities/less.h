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
// Created: 07.08.2026 16:00

#pragma once

#include <string_view>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include <sys/pados_types.h>

#include <System/ExceptionHandling.h>
#include <Utils/String.h>
#include <Utils/TerminalLineEditor.h>


namespace shutil_less
{

enum class LessKey
{
    Character,
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    PageUp,
    PageDown,
    Escape,
    EndOfInput,
    Unknown
};


struct InputKey
{
    LessKey Code = LessKey::Unknown;
    char    Character = '\0';
};


class CmdLess : public SignalTarget
{
public:
    explicit CmdLess(const char* fileName);

    int Run();

    static void PrintUsage(int fileDescriptor, const char* commandName);

private:
    bool LoadFile();
    bool EnterTerminal();
    void LeaveTerminal();

    InputKey ReadInput() const;
    bool ReadByte(char& outCharacter) const;

    void Redraw();
    void ScrollOneLine(int direction);
    void AppendFileLine(PString& output, size_t screenLine, size_t fileLine, const struct winsize& terminalSize) const;
    void AppendStatus(PString& output, size_t pageLineCount, const struct winsize& terminalSize) const;

    void DrawHelp() const;

    PString FormatText(
        const PString& text,
        size_t horizontalOffset,
        size_t width,
        std::string_view highlightText = {}) const;

    void AppendDisplayGlyph(
        PString& output,
        const char* glyph,
        size_t glyphLength,
        size_t& displayColumn,
        size_t& visibleColumns,
        size_t horizontalOffset,
        size_t width) const;

    PString MakeStatus(size_t pageLineCount) const;
    struct winsize GetTerminalSize() const;
    size_t GetPageLineCount(const struct winsize& terminalSize) const;
    size_t GetMaximumFirstLine() const;

    void MoveToEnd();
    void MoveLines(int64_t delta);
    void MovePages(int direction);
    bool PromptSearch(bool reverse);
    void HandleSearchSubmitted(const PString& query);
    bool FindNextMatch(bool reverse);

    static void WriteAll(int fileDescriptor, std::string_view text);
    static void PrintError(const PString& text);

    PString              m_FileName;
    std::vector<PString> m_Lines;
    PString              m_SearchText;
    PString              m_SubmittedSearchText;
    PString              m_Message;
    PTerminalLineEditor  m_SearchLineEditor;
    struct termios       m_OriginalTermios = {};
    size_t               m_FirstLine = 0;
    size_t               m_HorizontalOffset = 0;
    uint16_t             m_RenderedColumnCount = 0;
    uint16_t             m_RenderedRowCount = 0;
    int                  m_TerminalInputFD = STDIN_FILENO;
    bool                 m_ReadFromStandardInput = false;
    bool                 m_SearchReverse = false;
    bool                 m_SearchSubmitted = false;
    bool                 m_HasTerminal = false;
};

int less_main(int argc, char* argv[]);

} // namespace shutil_less
