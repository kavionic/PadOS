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

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string_view>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include <sys/pados_types.h>

#include <PadOS/Filesystem.h>
#include <System/AppDefinition.h>
#include <System/ExceptionHandling.h>
#include <Utils/String.h>
#include <Utils/TerminalLineEditor.h>
#include <Utils/UTF8Utils.h>


namespace shutil_less
{

constexpr std::string_view ANSI_CLEAR_SCREEN           = "\033[2J";
constexpr std::string_view ANSI_CLEAR_LINE             = "\033[2K";
constexpr std::string_view ANSI_CURSOR_TOP_LEFT        = "\033[H";
constexpr std::string_view ANSI_HIDE_CURSOR            = "\033[?25l";
constexpr std::string_view ANSI_SHOW_CURSOR            = "\033[?25h";
constexpr std::string_view ANSI_ENABLE_ALT_SCR_BUFFER  = "\033[?1049h";
constexpr std::string_view ANSI_DISABLE_ALT_SCR_BUFFER = "\033[?1049l";
constexpr std::string_view ANSI_REVERSE_VIDEO          = "\033[7m";
constexpr std::string_view ANSI_DISABLE_REVERSE_VIDEO  = "\033[27m";
constexpr std::string_view ANSI_RESET_RENDERING        = "\033[0m";
constexpr std::string_view ANSI_SCROLL_UP              = "\033[S";
constexpr std::string_view ANSI_SCROLL_DOWN            = "\033[T";
constexpr std::string_view ANSI_RESET_SCROLL_REGION    = "\033[r";
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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

CmdLess::CmdLess(const char* fileName)
    : m_FileName((fileName == nullptr || strcmp(fileName, "-") == 0) ? "standard input" : fileName)
    , m_TerminalInputFD((fileName == nullptr || strcmp(fileName, "-") == 0) ? STDOUT_FILENO : STDIN_FILENO)
    , m_ReadFromStandardInput(fileName == nullptr || strcmp(fileName, "-") == 0)
{
    m_SearchLineEditor.SetSubmissionMode(PTerminalLineEditor::SubmissionMode::AlwaysSubmit);
    m_SearchLineEditor.SetDisplayMode(PTerminalLineEditor::DisplayMode::HorizontalScroll);
    m_SearchLineEditor.SetDisconnectCharacter(PTerminalLineEditor::DISABLED_CONTROL_CHARACTER);
    m_SearchLineEditor.SetCancelCharacter(0x07);
    m_SearchLineEditor.SetBackspaceAtStartAction(PTerminalLineEditor::BackspaceAtStartAction::CancelInput);
    m_SearchLineEditor.SetAutoRestart(false);
    m_SearchLineEditor.SetEchoNewline(false);
    m_SearchLineEditor.SetEchoControlCharacters(false);
    m_SearchLineEditor.VFLineSubmitted.Connect(this, &CmdLess::HandleSearchSubmitted);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int CmdLess::Run()
{
    if (!LoadFile()) {
        return 1;
    }
    if (!EnterTerminal()) {
        return 1;
    }

    m_SearchLineEditor.Initialize(-1, STDOUT_FILENO);

    WriteAll(STDOUT_FILENO, ANSI_ENABLE_ALT_SCR_BUFFER);
    WriteAll(STDOUT_FILENO, ANSI_HIDE_CURSOR);

    PScopeExit restoreTerminal([this]
        {
            WriteAll(STDOUT_FILENO, ANSI_SHOW_CURSOR);
            WriteAll(STDOUT_FILENO, ANSI_DISABLE_ALT_SCR_BUFFER);
            LeaveTerminal();
        }
    );

    bool running = true;
    Redraw();

    for (;;)
    {
        int scrollDirection = 0;
        bool needsRedraw = false;
        const InputKey input = ReadInput();

        switch (input.Code)
        {
            case LessKey::Up:
                m_Message.clear();
                scrollDirection = -1;
                break;

            case LessKey::Down:
                m_Message.clear();
                scrollDirection = 1;
                break;

            case LessKey::Left:
                m_Message.clear();
                if (m_HorizontalOffset >= 8) {
                    m_HorizontalOffset -= 8;
                } else {
                    m_HorizontalOffset = 0;
                }
                needsRedraw = true;
                break;

            case LessKey::Right:
                m_Message.clear();
                m_HorizontalOffset += 8;
                needsRedraw = true;
                break;

            case LessKey::Home:
                m_Message.clear();
                m_FirstLine = 0;
                needsRedraw = true;
                break;

            case LessKey::End:
                m_Message.clear();
                MoveToEnd();
                needsRedraw = true;
                break;

            case LessKey::PageUp:
                m_Message.clear();
                MovePages(-1);
                needsRedraw = true;
                break;

            case LessKey::PageDown:
                m_Message.clear();
                MovePages(1);
                needsRedraw = true;
                break;

            case LessKey::EndOfInput:
                running = false;
                break;

            case LessKey::Character:
                switch (input.Character)
                {
                    case 'q':
                    case 'Q':
                    case 0x03:
                        running = false;
                        break;

                    case 'j':
                    case '\r':
                    case '\n':
                        m_Message.clear();
                        scrollDirection = 1;
                        break;

                    case 'k':
                        m_Message.clear();
                        scrollDirection = -1;
                        break;

                    case ' ':
                    case 'f':
                        m_Message.clear();
                        MovePages(1);
                        needsRedraw = true;
                        break;

                    case 'b':
                        m_Message.clear();
                        MovePages(-1);
                        needsRedraw = true;
                        break;

                    case 'g':
                        m_Message.clear();
                        m_FirstLine = 0;
                        needsRedraw = true;
                        break;

                    case 'G':
                        m_Message.clear();
                        MoveToEnd();
                        needsRedraw = true;
                        break;

                    case '0':
                        m_Message.clear();
                        m_HorizontalOffset = 0;
                        needsRedraw = true;
                        break;

                    case '/':
                        PromptSearch(false);
                        needsRedraw = true;
                        break;

                    case '?':
                        PromptSearch(true);
                        needsRedraw = true;
                        break;

                    case 'n':
                        FindNextMatch(m_SearchReverse);
                        needsRedraw = true;
                        break;

                    case 'N':
                        FindNextMatch(!m_SearchReverse);
                        needsRedraw = true;
                        break;

                    case 'h':
                    case 'H':
                        DrawHelp();
                        needsRedraw = true;
                        break;

                    default:
                        break;
                }
                break;

            case LessKey::Escape:
            case LessKey::Unknown:
                break;
        }

        if (!running) {
            break;
        }
        if (scrollDirection != 0) {
            ScrollOneLine(scrollDirection);
        } else if (needsRedraw) {
            Redraw();
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdLess::LoadFile()
{
    const int fileDescriptor = m_ReadFromStandardInput ? STDIN_FILENO : open(m_FileName.c_str(), O_RDONLY);

    if (fileDescriptor == -1)
    {
        PrintError(PString::format_string(
            "less: cannot open '{}': {}\n", m_FileName, strerror(errno)));
        return false;
    }

    PScopeExit closeFile([fileDescriptor]
        {
            if (fileDescriptor != STDIN_FILENO) {
                close(fileDescriptor);
            }
        }
    );

    PString currentLine;
    char buffer[2048];

    for (;;)
    {
        const ssize_t bytesRead = read(fileDescriptor, buffer, sizeof(buffer));

        if (bytesRead < 0)
        {
            if (errno == EINTR) {
                continue;
            }

            PrintError(PString::format_string(
                "less: failed to read '{}': {}\n", m_FileName, strerror(errno)));
            return false;
        }
        if (bytesRead == 0) {
            break;
        }

        for (size_t byteIndex = 0; byteIndex < size_t(bytesRead); ++byteIndex)
        {
            if (buffer[byteIndex] == '\n')
            {
                if (!currentLine.empty() && currentLine.back() == '\r') {
                    currentLine.pop_back();
                }

                m_Lines.push_back(std::move(currentLine));
                currentLine.clear();
            }
            else
            {
                currentLine.push_back(buffer[byteIndex]);
            }
        }
    }

    if (!currentLine.empty() || m_Lines.empty()) {
        m_Lines.push_back(std::move(currentLine));
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdLess::EnterTerminal()
{
    const PErrorCode getResult = device_control(
        m_TerminalInputFD,
        TCGETA,
        nullptr,
        0,
        &m_OriginalTermios,
        sizeof(m_OriginalTermios));

    if (getResult != PErrorCode::Success)
    {
        PrintError("less: standard input is not an interactive terminal\n");
        return false;
    }

    struct termios rawTerminal = m_OriginalTermios;

    rawTerminal.c_iflag &= ~(ICRNL | IXON);
    rawTerminal.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    rawTerminal.c_cc[VMIN] = 1;
    rawTerminal.c_cc[VTIME] = 0;

    const PErrorCode setResult = device_control(
        m_TerminalInputFD,
        TCSETA,
        &rawTerminal,
        sizeof(rawTerminal),
        nullptr,
        0);

    if (setResult != PErrorCode::Success)
    {
        PrintError("less: failed to configure the terminal\n");
        return false;
    }

    m_HasTerminal = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::LeaveTerminal()
{
    if (m_HasTerminal)
    {
        device_control(
            m_TerminalInputFD,
            TCSETA,
            &m_OriginalTermios,
            sizeof(m_OriginalTermios),
            nullptr,
            0);

        m_HasTerminal = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

InputKey CmdLess::ReadInput() const
{
    char character;

    if (!ReadByte(character)) {
        return {LessKey::EndOfInput, '\0'};
    }
    if (character != '\033') {
        return {LessKey::Character, character};
    }

    char sequenceType;
    char keyCode;

    if (!ReadByte(sequenceType)) {
        return {LessKey::Escape, '\0'};
    }
    if (sequenceType != '[' && sequenceType != 'O') {
        return {LessKey::Unknown, '\0'};
    }
    if (!ReadByte(keyCode)) {
        return {LessKey::EndOfInput, '\0'};
    }

    switch (keyCode)
    {
        case 'A': return {LessKey::Up, '\0'};
        case 'B': return {LessKey::Down, '\0'};
        case 'C': return {LessKey::Right, '\0'};
        case 'D': return {LessKey::Left, '\0'};
        case 'H': return {LessKey::Home, '\0'};
        case 'F': return {LessKey::End, '\0'};
        default:
            break;
    }

    if (sequenceType == '[' && keyCode >= '0' && keyCode <= '9')
    {
        char suffix;

        if (!ReadByte(suffix)) {
            return {LessKey::EndOfInput, '\0'};
        }
        if (suffix != '~') {
            return {LessKey::Unknown, '\0'};
        }

        switch (keyCode)
        {
            case '1':
            case '7':
                return {LessKey::Home, '\0'};

            case '4':
            case '8':
                return {LessKey::End, '\0'};

            case '5':
                return {LessKey::PageUp, '\0'};

            case '6':
                return {LessKey::PageDown, '\0'};

            default:
                break;
        }
    }

    return {LessKey::Unknown, '\0'};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdLess::ReadByte(char& outCharacter) const
{
    for (;;)
    {
        const ssize_t bytesRead = read(m_TerminalInputFD, &outCharacter, 1);

        if (bytesRead == 1) {
            return true;
        }
        if (bytesRead == 0) {
            return false;
        }
        if (errno != EINTR) {
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::Redraw()
{
    const struct winsize terminalSize = GetTerminalSize();
    const size_t pageLineCount = GetPageLineCount(terminalSize);

    PString output;
    output.reserve(size_t(terminalSize.ws_col) * size_t(terminalSize.ws_row) + 128);

    for (size_t screenLine = 0; screenLine < pageLineCount; ++screenLine)
    {
        AppendFileLine(
            output,
            screenLine + 1,
            m_FirstLine + screenLine,
            terminalSize);
    }

    AppendStatus(output, pageLineCount, terminalSize);
    WriteAll(STDOUT_FILENO, output);

    m_RenderedColumnCount = terminalSize.ws_col;
    m_RenderedRowCount = terminalSize.ws_row;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::ScrollOneLine(int direction)
{
    const size_t previousFirstLine = m_FirstLine;
    MoveLines(direction);

    if (m_FirstLine == previousFirstLine) {
        return;
    }

    const struct winsize terminalSize = GetTerminalSize();
    const size_t pageLineCount = GetPageLineCount(terminalSize);

    if (pageLineCount < 2 ||
        terminalSize.ws_col != m_RenderedColumnCount ||
        terminalSize.ws_row != m_RenderedRowCount)
    {
        Redraw();
        return;
    }

    PString output;
    output.reserve(size_t(terminalSize.ws_col) * 2 + 128);

    output += PString::format_string(
        "\033[1;{}r", pageLineCount);
    output += (direction > 0) ? ANSI_SCROLL_UP : ANSI_SCROLL_DOWN;
    output += ANSI_RESET_SCROLL_REGION;

    const size_t screenLine = (direction > 0) ? pageLineCount : 1;
    const size_t fileLine = (direction > 0)
        ? m_FirstLine + pageLineCount - 1
        : m_FirstLine;

    AppendFileLine(output, screenLine, fileLine, terminalSize);
    AppendStatus(output, pageLineCount, terminalSize);

    WriteAll(STDOUT_FILENO, output);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::AppendFileLine(
    PString& output,
    size_t screenLine,
    size_t fileLine,
    const struct winsize& terminalSize) const
{
    output += PString::format_string(
        "\033[{};1H", screenLine);
    output += ANSI_CLEAR_LINE;

    if (fileLine < m_Lines.size())
    {
        output += FormatText(
            m_Lines[fileLine],
            m_HorizontalOffset,
            terminalSize.ws_col,
            m_SearchText);
    }
    else
    {
        output.push_back('~');
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::AppendStatus(
    PString& output,
    size_t pageLineCount,
    const struct winsize& terminalSize) const
{
    output += PString::format_string(
        "\033[{};1H", pageLineCount + 1);
    output += ANSI_REVERSE_VIDEO;
    output += ANSI_CLEAR_LINE;
    output += FormatText(MakeStatus(pageLineCount), 0, terminalSize.ws_col);
    output += ANSI_RESET_RENDERING;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::DrawHelp() const
{
    PString output;

    output += ANSI_CLEAR_SCREEN;
    output += ANSI_CURSOR_TOP_LEFT;
    output +=
        "less command help\n"
        "\n"
        "  q, Q, Ctrl-C       Quit\n"
        "  Down, j, Enter     Forward one line\n"
        "  Up, k              Backward one line\n"
        "  Space, f, PgDn     Forward one page\n"
        "  b, PgUp            Backward one page\n"
        "  g, Home            Go to the first line\n"
        "  G, End             Go to the last line\n"
        "  Left, Right        Scroll horizontally\n"
        "  0                  Return to the first column\n"
        "  /text              Search forward\n"
        "  ?text              Search backward\n"
        "  n, N               Repeat search\n"
        "  h                  Show this help\n"
        "\n"
        "Press any key to return.";

    WriteAll(STDOUT_FILENO, output);
    ReadInput();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdLess::FormatText(
    const PString& text,
    size_t horizontalOffset,
    size_t width,
    std::string_view highlightText) const
{
    PString output;
    size_t displayColumn = 0;
    size_t visibleColumns = 0;
    size_t nextMatchOffset = highlightText.empty()
        ? PString::npos
        : text.find(highlightText);
    size_t matchEndOffset = PString::npos;
    bool isHighlighted = false;

    for (size_t byteOffset = 0;
         byteOffset < text.size() && visibleColumns < width;)
    {
        if (isHighlighted && byteOffset >= matchEndOffset)
        {
            output += ANSI_DISABLE_REVERSE_VIDEO;
            isHighlighted = false;
            nextMatchOffset = text.find(highlightText, matchEndOffset);
        }
        if (!isHighlighted && byteOffset == nextMatchOffset)
        {
            output += ANSI_REVERSE_VIDEO;
            isHighlighted = true;
            matchEndOffset = nextMatchOffset + highlightText.size();
        }

        const uint8_t byte = uint8_t(text[byteOffset]);

        if (byte == '\t')
        {
            const size_t spaceCount = 8 - displayColumn % 8;

            for (size_t spaceIndex = 0; spaceIndex < spaceCount; ++spaceIndex)
            {
                AppendDisplayGlyph(
                    output,
                    " ",
                    1,
                    displayColumn,
                    visibleColumns,
                    horizontalOffset,
                    width);
            }

            ++byteOffset;
        }
        else if (byte < 0x20 || byte == 0x7f)
        {
            const char controlGlyph[2] = {
                '^',
                (byte == 0x7f) ? '?' : char(byte + '@')
            };

            AppendDisplayGlyph(
                output,
                &controlGlyph[0],
                1,
                displayColumn,
                visibleColumns,
                horizontalOffset,
                width);

            AppendDisplayGlyph(
                output,
                &controlGlyph[1],
                1,
                displayColumn,
                visibleColumns,
                horizontalOffset,
                width);

            ++byteOffset;
        }
        else
        {
            const size_t glyphLength = std::min<size_t>(
                utf8_char_length(byte),
                text.size() - byteOffset);

            AppendDisplayGlyph(
                output,
                text.data() + byteOffset,
                glyphLength,
                displayColumn,
                visibleColumns,
                horizontalOffset,
                width);

            byteOffset += glyphLength;
        }
    }

    if (isHighlighted) {
        output += ANSI_DISABLE_REVERSE_VIDEO;
    }

    return output;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::AppendDisplayGlyph(
    PString& output,
    const char* glyph,
    size_t glyphLength,
    size_t& displayColumn,
    size_t& visibleColumns,
    size_t horizontalOffset,
    size_t width) const
{
    if (displayColumn >= horizontalOffset && visibleColumns < width)
    {
        output.append(glyph, glyphLength);
        ++visibleColumns;
    }

    ++displayColumn;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString CmdLess::MakeStatus(size_t pageLineCount) const
{
    if (!m_Message.empty()) {
        return m_Message;
    }

    const bool isAtEnd = m_FirstLine + pageLineCount >= m_Lines.size();
    const size_t percentage = isAtEnd
        ? 100
        : (m_FirstLine + 1) * 100 / m_Lines.size();

    PString status = PString::format_string(
        "{}  line {}/{}  {}%",
        m_FileName,
        m_FirstLine + 1,
        m_Lines.size(),
        percentage);

    if (isAtEnd) {
        status += "  (END)";
    }

    return status;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct winsize CmdLess::GetTerminalSize() const
{
    struct winsize terminalSize = {};

    const PErrorCode result = device_control(
        STDOUT_FILENO,
        TIOCGWINSZ,
        nullptr,
        0,
        &terminalSize,
        sizeof(terminalSize));

    if (result != PErrorCode::Success || terminalSize.ws_col == 0) {
        terminalSize.ws_col = 80;
    }
    if (result != PErrorCode::Success || terminalSize.ws_row < 2) {
        terminalSize.ws_row = 24;
    }

    return terminalSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t CmdLess::GetPageLineCount(const struct winsize& terminalSize) const
{
    return std::max<size_t>(1, size_t(terminalSize.ws_row) - 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t CmdLess::GetMaximumFirstLine() const
{
    const struct winsize terminalSize = GetTerminalSize();
    const size_t pageLineCount = GetPageLineCount(terminalSize);

    return (m_Lines.size() > pageLineCount)
        ? m_Lines.size() - pageLineCount
        : 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::MoveToEnd()
{
    m_FirstLine = GetMaximumFirstLine();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::MoveLines(int64_t delta)
{
    if (delta < 0)
    {
        const size_t distance = size_t(-delta);

        if (distance > m_FirstLine) {
            m_FirstLine = 0;
        } else {
            m_FirstLine -= distance;
        }
    }
    else
    {
        m_FirstLine = std::min(
            m_FirstLine + size_t(delta),
            GetMaximumFirstLine());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::MovePages(int direction)
{
    const struct winsize terminalSize = GetTerminalSize();
    const int64_t distance = int64_t(GetPageLineCount(terminalSize));
    MoveLines((direction < 0) ? -distance : distance);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdLess::PromptSearch(bool reverse)
{
    m_SearchSubmitted = false;
    m_SubmittedSearchText.clear();
    m_SearchLineEditor.SetPrimaryPrompt(reverse ? "?" : "/", 1);
    WriteAll(STDOUT_FILENO, ANSI_SHOW_CURSOR);
    m_SearchLineEditor.BeginInput();

    while (m_SearchLineEditor.IsInputActive())
    {
        char character;
        if (!ReadByte(character))
        {
            m_SearchLineEditor.ResetInput();
            WriteAll(STDOUT_FILENO, ANSI_HIDE_CURSOR);
            return false;
        }
        m_SearchLineEditor.AddInput(&character, 1);
    }

    WriteAll(STDOUT_FILENO, ANSI_HIDE_CURSOR);

    if (!m_SearchSubmitted)
    {
        m_Message = "Search canceled";
        return false;
    }
    if (!m_SubmittedSearchText.empty()) {
        m_SearchText = std::move(m_SubmittedSearchText);
    }
    if (m_SearchText.empty())
    {
        m_Message = "No previous search";
        return false;
    }

    m_SearchReverse = reverse;
    return FindNextMatch(reverse);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::HandleSearchSubmitted(const PString& query)
{
    m_SubmittedSearchText = query;
    m_SearchSubmitted = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CmdLess::FindNextMatch(bool reverse)
{
    if (m_SearchText.empty())
    {
        m_Message = "No previous search";
        return false;
    }

    for (size_t distance = 1; distance <= m_Lines.size(); ++distance)
    {
        const size_t lineIndex = reverse
            ? (m_FirstLine + m_Lines.size() - distance) % m_Lines.size()
            : (m_FirstLine + distance) % m_Lines.size();

        if (m_Lines[lineIndex].find(m_SearchText) != PString::npos)
        {
            m_FirstLine = lineIndex;
            m_Message = PString::format_string(
                "{} found at line {}",
                reverse ? "Backward match" : "Match",
                lineIndex + 1);
            return true;
        }
    }

    m_Message = PString::format_string(
        "Pattern not found: {}", m_SearchText);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::WriteAll(int fileDescriptor, std::string_view text)
{
    size_t bytesWritten = 0;

    while (bytesWritten < text.size())
    {
        const ssize_t result = write(
            fileDescriptor,
            text.data() + bytesWritten,
            text.size() - bytesWritten);

        if (result > 0)
        {
            bytesWritten += size_t(result);
        }
        else if (result < 0 && errno == EINTR)
        {
            continue;
        }
        else
        {
            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::PrintUsage(int fileDescriptor, const char* commandName)
{
    const PString usage = PString::format_string(
        "Usage: {} [FILE]\n"
        "View FILE, or standard input, one screen at a time.\n"
        "\n"
        "  --help    display this help and exit\n",
        commandName);

    WriteAll(fileDescriptor, usage);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CmdLess::PrintError(const PString& text)
{
    WriteAll(STDERR_FILENO, text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int less_main(int argc, char* argv[])
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        CmdLess::PrintUsage(STDOUT_FILENO, argv[0]);
        return 0;
    }
    if (argc > 2)
    {
        CmdLess::PrintUsage(STDERR_FILENO, argv[0]);
        return 1;
    }

    CmdLess less((argc == 2) ? argv[1] : nullptr);
    return less.Run();
}


static PAppDefinition g_LessAppDef(
    "less",
    "View a text file one screen at a time.",
    less_main);

} // namespace shutil_less
