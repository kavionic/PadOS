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
// Created: 09.08.2026 23:00

#include <algorithm>
#include <cerrno>
#include <string_view>
#include <termios.h>
#include <unistd.h>

#include <sys/pados_types.h>

#include <Utils/POSIXTokenizer.h>
#include <Utils/TerminalLineEditor.h>
#include <Utils/UTF8Utils.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::Initialize(int inputFD, int outputFD)
{
    m_InputFD = inputFD;
    m_OutputFD = outputFD;

    UpdateTerminalSize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::BeginInput(const PString& initialText)
{
    UpdateTerminalSize();
    ClearInput();

    m_Prompt = m_PrimaryPrompt;
    m_PromptVisibleLength = m_PrimaryPromptVisibleLength;
    m_InputActive = true;

    if (m_DisplayMode == DisplayMode::HorizontalScroll)
    {
        m_EditBuffer = initialText;
        m_CursorPosition = m_EditBuffer.size();
        RenderSingleLine();
    }
    else
    {
        WriteText(m_Prompt);
        AddInputText(initialText.c_str(), initialText.size());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::ResetInput()
{
    ClearInput();
    m_InputActive = false;

    if (m_EchoNewline) {
        WriteNewline();
    }
    RestartInput();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::AddInput(const char* text, size_t length)
{
    if (!m_InputActive || text == nullptr) {
        return 0;
    }

    size_t textStart = 0;

    for (size_t characterIndex = 0; characterIndex < length; ++characterIndex)
    {
        const uint8_t character = uint8_t(text[characterIndex]);

        if (m_SkipNextLineFeed)
        {
            m_SkipNextLineFeed = false;
            if (character == '\n')
            {
                textStart = characterIndex + 1;
                continue;
            }
        }

        PANSI_ControlCode controlCharacter = PANSI_ControlCode::None;

        if (m_BreakCharacter != DISABLED_CONTROL_CHARACTER && int(character) == m_BreakCharacter) {
            controlCharacter = PANSI_ControlCode::Break;
        } else if (m_DisconnectCharacter != DISABLED_CONTROL_CHARACTER && int(character) == m_DisconnectCharacter) {
            controlCharacter = PANSI_ControlCode::Disconnect;
        } else if (m_CancelCharacter != DISABLED_CONTROL_CHARACTER && int(character) == m_CancelCharacter) {
            controlCharacter = PANSI_ControlCode::Cancel;
        } else if (character != 0x03 && character != 0x04) {
            controlCharacter = m_ANSICodeParser.ProcessCharacter(text[characterIndex]);
        }

        if (controlCharacter != PANSI_ControlCode::None)
        {
            AddInputText(text + textStart, characterIndex - textStart);
            textStart = characterIndex + 1;

            if (controlCharacter != PANSI_ControlCode::Pending && ProcessControlCharacter(controlCharacter, m_ANSICodeParser.GetArguments())) {
                return characterIndex + 1;
            }
            continue;
        }

        if (character == '\r' || character == '\n')
        {
            AddInputText(text + textStart, characterIndex - textStart);
            FlushIncompleteUTF8Text();
            textStart = characterIndex + 1;
            m_SkipNextLineFeed = character == '\r';

            if (SubmitLine()) {
                return characterIndex + 1;
            }
        }
    }

    AddInputText(text + textStart, length - textStart);
    return length;
}

void PTerminalLineEditor::SetPrimaryPrompt(const PString& text, size_t visibleLength)
{
    const bool primaryPromptActive = m_Prompt == m_PrimaryPrompt;

    m_PrimaryPrompt = text;
    m_PrimaryPromptVisibleLength = visibleLength;

    if (m_InputActive && primaryPromptActive)
    {
        m_Prompt = m_PrimaryPrompt;
        m_PromptVisibleLength = m_PrimaryPromptVisibleLength;
        if (m_DisplayMode == DisplayMode::HorizontalScroll) {
            RenderSingleLine();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::SetContinuationPrompt(const PString& text, size_t visibleLength)
{
    m_ContinuationPrompt = text;
    m_ContinuationPromptVisibleLength = visibleLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTerminalLineEditor::ReadResult PTerminalLineEditor::ReadInput()
{
    if (m_InputFD == -1) {
        return ReadResult::Error;
    }

    if (m_PendingReadText.empty())
    {
        char buffer[32];

        ssize_t length;
        do {
            length = read(m_InputFD, buffer, sizeof(buffer));
        } while (length < 0 && errno == EINTR);

        if (length > 0) {
            m_PendingReadText.append(buffer, size_t(length));
        } else if (length == 0) {
            return ReadResult::EndOfInput;
        } else {
            return ReadResult::Error;
        }
    }

    UpdateTerminalSize();
    size_t bytesConsumed = 0;

    while (bytesConsumed < m_PendingReadText.size() && m_InputActive)
    {
        const size_t consumed = AddInput(
            m_PendingReadText.data() + bytesConsumed,
            m_PendingReadText.size() - bytesConsumed);

        if (consumed == 0) {
            break;
        }
        bytesConsumed += consumed;
    }

    m_PendingReadText.erase(0, bytesConsumed);
    return ReadResult::DataRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::GetCommonStartLength(const std::vector<CompletionCandidate>& candidates)
{
    if (candidates.empty()) {
        return 0;
    }
    if (candidates.size() == 1) {
        return candidates[0].Text.size();
    }

    size_t commonStartLength = 0;
    for (;;)
    {
        bool reachedEnd = false;
        for (size_t candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex)
        {
            const PString& text = candidates[candidateIndex].Text;

            if (text.size() <= commonStartLength ||
                (candidateIndex != 0 && text[commonStartLength] != candidates[0].Text[commonStartLength]))
            {
                reachedEnd = true;
                break;
            }
        }
        if (reachedEnd) {
            break;
        }
        ++commonStartLength;
    }

    while (commonStartLength > 0 &&
           commonStartLength < candidates[0].Text.size() &&
           !is_first_utf8_byte(uint8_t(candidates[0].Text[commonStartLength])))
    {
        --commonStartLength;
    }
    return commonStartLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::GetUTF8CharacterLength(std::string_view text, size_t offset)
{
    if (offset >= text.size()) {
        return 0;
    }

    const size_t characterLength = size_t(utf8_char_length(uint8_t(text[offset])));
    return (characterLength <= text.size() - offset) ? characterLength : 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::GetPreviousUTF8CharacterOffset(std::string_view text, size_t offset)
{
    if (offset == 0) {
        return 0;
    }

    size_t characterOffset = offset - 1;

    while (characterOffset > 0 && !is_first_utf8_byte(uint8_t(text[characterOffset]))) {
        --characterOffset;
    }
    return characterOffset;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::GetUTF8CharacterCount(std::string_view text, size_t startOffset, size_t endOffset)
{
    size_t characterCount = 0;

    for (size_t characterOffset = startOffset; characterOffset < endOffset; ++characterCount)
    {
        size_t characterLength = GetUTF8CharacterLength(text, characterOffset);

        if (characterLength == 0) {
            characterLength = 1;
        }
        characterOffset += characterLength;
    }
    return characterCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::AddInputText(const char* text, size_t length)
{
    if (length == 0) {
        return;
    }

    m_IncompleteUTF8Text.append(text, length);

    size_t completeLength = 0;
    while (completeLength < m_IncompleteUTF8Text.size())
    {
        const size_t characterLength = GetUTF8CharacterLength(m_IncompleteUTF8Text, completeLength);

        if (characterLength == 0) {
            break;
        }
        completeLength += characterLength;
    }

    if (completeLength != 0)
    {
        InsertInputText(m_IncompleteUTF8Text.data(), completeLength);
        m_IncompleteUTF8Text.erase(0, completeLength);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::InsertInputText(const char* text, size_t length)
{
    m_PendingExpansionAlternatives.clear();
    const size_t insertionPosition = m_CursorPosition;

    m_EditBuffer.insert(m_EditBuffer.begin() + insertionPosition, text, text + length);
    m_CursorPosition += length;

    if (m_DisplayMode == DisplayMode::HorizontalScroll) {
        RefreshSingleLine(insertionPosition);
    } else if (m_CursorPosition == m_EditBuffer.size()) {
        WriteText(text, length);

        const PIPoint screenPosition = GetScreenPosition(m_CursorPosition);
        if (screenPosition.x == 0) {
            WriteText(" \010", 2);
        }
    } else {
        RefreshText(insertionPosition);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::FlushIncompleteUTF8Text()
{
    if (!m_IncompleteUTF8Text.empty())
    {
        InsertInputText(m_IncompleteUTF8Text.data(), m_IncompleteUTF8Text.size());
        m_IncompleteUTF8Text.clear();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTerminalLineEditor::SubmitLine()
{
    if (m_EchoNewline) {
        WriteNewline();
    }

    m_PendingExpansionAlternatives.clear();

    const PString lineBuffer = m_InputBuffer + m_EditBuffer;

    if (m_SubmissionMode == SubmissionMode::ContinueIncomplete)
    {
        const PPOSIXTokenizer tokenizer(lineBuffer);
        if (tokenizer.GetTermination() != PPOSIXTokenizer::Termination::Normal)
        {
            m_InputBuffer += m_EditBuffer + "\n";
            m_EditBuffer.clear();
            m_CursorPosition = 0;

            m_Prompt = m_ContinuationPrompt;
            m_PromptVisibleLength = m_ContinuationPromptVisibleLength;
            WriteText(m_Prompt);
            return false;
        }
    }

    if (m_HistoryEnabled && !lineBuffer.empty()) {
        m_HistoryBuffers.push_back(lineBuffer);
    }

    ClearInput();
    m_InputActive = false;

    VFLineSubmitted(lineBuffer);
    RestartInput();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::ClearInput()
{
    m_ANSICodeParser.Reset();
    m_PendingExpansionAlternatives.clear();
    m_InputBuffer.clear();
    m_EditBuffer.clear();
    m_IncompleteUTF8Text.clear();
    m_CursorPosition = 0;
    m_HorizontalOffset = 0;
    m_HistoryLocation = m_HistoryBuffers.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::CancelInput()
{
    ClearInput();
    m_InputActive = false;
    RestartInput();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::RestartInput()
{
    if (m_AutoRestart) {
        BeginInput();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::WriteText(const char* text, size_t length) const
{
    if (m_OutputFD == -1) {
        return;
    }

    size_t bytesWritten = 0;
    while (bytesWritten < length)
    {
        const ssize_t result = write(m_OutputFD, text + bytesWritten, length - bytesWritten);
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

void PTerminalLineEditor::UpdateTerminalSize()
{
    if (m_OutputFD == -1) {
        return;
    }

    struct winsize terminalSize = {};

    const PErrorCode result = device_control(m_OutputFD, TIOCGWINSZ, nullptr, 0, &terminalSize, sizeof(terminalSize));

    if (result == PErrorCode::Success && terminalSize.ws_col > 0)
    {
        m_TerminalSize.x = terminalSize.ws_col;
        m_TerminalSize.y = terminalSize.ws_row;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::ShowTerminalCursor(bool show)
{
    WriteText(show ? "\033[?25h" : "\033[?25l");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::GetSingleLineEditWidth() const
{
    const size_t terminalWidth = std::max(size_t(m_TerminalSize.x), size_t(1));
    const size_t promptWidth = (m_PromptVisibleLength < terminalWidth) ? m_PromptVisibleLength : 0;
    return terminalWidth - promptWidth - 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::GetSingleLineCursorColumn() const
{
    return GetUTF8CharacterCount(m_EditBuffer, m_HorizontalOffset, m_CursorPosition);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTerminalLineEditor::GetSingleLineVisibleEnd() const
{
    const size_t editWidth = GetSingleLineEditWidth();
    size_t visibleEnd = m_HorizontalOffset;
    size_t visibleColumns = 0;

    while (visibleEnd < m_EditBuffer.size() && visibleColumns < editWidth)
    {
        size_t characterLength = GetUTF8CharacterLength(m_EditBuffer, visibleEnd);

        if (characterLength == 0) {
            characterLength = 1;
        }
        visibleEnd += characterLength;
        ++visibleColumns;
    }
    return visibleEnd;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTerminalLineEditor::UpdateSingleLineHorizontalOffset()
{
    const size_t previousHorizontalOffset = m_HorizontalOffset;

    if (m_CursorPosition < m_HorizontalOffset) {
        m_HorizontalOffset = m_CursorPosition;
    }

    size_t cursorColumn = GetSingleLineCursorColumn();
    const size_t editWidth = GetSingleLineEditWidth();

    while (cursorColumn > editWidth && m_HorizontalOffset < m_CursorPosition)
    {
        size_t characterLength = GetUTF8CharacterLength(m_EditBuffer, m_HorizontalOffset);

        if (characterLength == 0) {
            characterLength = 1;
        }
        m_HorizontalOffset += characterLength;
        --cursorColumn;
    }
    return m_HorizontalOffset != previousHorizontalOffset;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::RefreshSingleLine(size_t startPosition)
{
    if (UpdateSingleLineHorizontalOffset() || startPosition < m_HorizontalOffset)
    {
        RenderSingleLine();
        return;
    }

    const size_t visibleEnd = GetSingleLineVisibleEnd();
    if (startPosition > visibleEnd)
    {
        RenderSingleLine();
        return;
    }

    const size_t endColumn = GetUTF8CharacterCount(m_EditBuffer, m_HorizontalOffset, visibleEnd);
    const size_t cursorColumn = GetSingleLineCursorColumn();
    const bool repositionCursor = cursorColumn != endColumn;

    if (repositionCursor) {
        ShowTerminalCursor(false);
    }
    WriteText(m_EditBuffer.data() + startPosition, visibleEnd - startPosition);
    SendANSICode(PANSI_ControlCode::EraseInLine);

    if (cursorColumn < endColumn) {
        SendANSICode(PANSI_ControlCode::XTerm_Left, int(endColumn - cursorColumn));
    } else if (cursorColumn > endColumn) {
        SendANSICode(PANSI_ControlCode::XTerm_Right, int(cursorColumn - endColumn));
    }
    if (repositionCursor) {
        ShowTerminalCursor(true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::RenderSingleLine()
{
    const size_t terminalWidth = std::max(size_t(m_TerminalSize.x), size_t(1));
    const bool promptFits = m_PromptVisibleLength < terminalWidth;
    UpdateSingleLineHorizontalOffset();

    const size_t visibleEnd = GetSingleLineVisibleEnd();
    const size_t endColumn = GetUTF8CharacterCount(m_EditBuffer, m_HorizontalOffset, visibleEnd);
    const size_t cursorColumn = GetSingleLineCursorColumn();

    ShowTerminalCursor(false);
    WriteText("\r", 1);
    if (promptFits) {
        WriteText(m_Prompt);
    }
    WriteText(m_EditBuffer.data() + m_HorizontalOffset, visibleEnd - m_HorizontalOffset);
    SendANSICode(PANSI_ControlCode::EraseInLine);

    if (cursorColumn < endColumn) {
        SendANSICode(PANSI_ControlCode::XTerm_Left, int(endColumn - cursorColumn));
    }
    ShowTerminalCursor(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIPoint PTerminalLineEditor::GetScreenPosition(size_t cursorPosition) const
{
    const size_t terminalWidth = std::max(size_t(m_TerminalSize.x), size_t(1));
    PIPoint position(
        int(m_PromptVisibleLength % terminalWidth),
        int(m_PromptVisibleLength / terminalWidth));

    for (size_t characterOffset = 0; characterOffset < cursorPosition; )
    {
        size_t characterLength = GetUTF8CharacterLength(m_EditBuffer, characterOffset);

        if (characterLength == 0) {
            characterLength = 1;
        }

        if (m_EditBuffer[characterOffset] == '\n')
        {
            position.x = 0;
            position.y++;
        }
        else
        {
            position.x++;
            if (size_t(position.x) == terminalWidth)
            {
                position.x = 0;
                position.y++;
            }
        }
        characterOffset += characterLength;
    }
    return position;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::MoveScreenCursor(const PIPoint& startPosition, const PIPoint& endPosition)
{
    const PIPoint delta = endPosition - startPosition;
    ShowTerminalCursor(false);
    if (delta.y < 0) {
        SendANSICode(PANSI_ControlCode::XTerm_Up, -delta.y);
    } else if (delta.y > 0) {
        SendANSICode(PANSI_ControlCode::XTerm_Down, delta.y);
    }
    if (delta.x < 0) {
        SendANSICode(PANSI_ControlCode::XTerm_Left, -delta.x);
    } else if (delta.x > 0) {
        SendANSICode(PANSI_ControlCode::XTerm_Right, delta.x);
    }
    ShowTerminalCursor(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::MoveCursor(ptrdiff_t distance)
{
    m_PendingExpansionAlternatives.clear();
    size_t newPosition = m_CursorPosition;

    if (distance < 0)
    {
        while (distance < 0 && newPosition != 0)
        {
            newPosition = GetPreviousUTF8CharacterOffset(m_EditBuffer, newPosition);
            ++distance;
        }
    }
    else
    {
        while (distance > 0 && newPosition < m_EditBuffer.size())
        {
            size_t characterLength = GetUTF8CharacterLength(m_EditBuffer, newPosition);

            if (characterLength == 0) {
                characterLength = 1;
            }
            newPosition = std::min(newPosition + characterLength, m_EditBuffer.size());
            --distance;
        }
    }
    MoveCursorTo(newPosition);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::MoveCursorTo(size_t position)
{
    position = std::min(position, m_EditBuffer.size());
    if (position == m_CursorPosition) {
        return;
    }

    if (m_DisplayMode == DisplayMode::HorizontalScroll)
    {
        const size_t previousCursorColumn = GetSingleLineCursorColumn();
        m_CursorPosition = position;

        if (UpdateSingleLineHorizontalOffset()) {
            RenderSingleLine();
        } else {
            const size_t cursorColumn = GetSingleLineCursorColumn();
            if (cursorColumn < previousCursorColumn) {
                SendANSICode(PANSI_ControlCode::XTerm_Left, int(previousCursorColumn - cursorColumn));
            } else if (cursorColumn > previousCursorColumn) {
                SendANSICode(PANSI_ControlCode::XTerm_Right, int(cursorColumn - previousCursorColumn));
            }
        }
        return;
    }

    const PIPoint previousScreenPosition = GetScreenPosition(m_CursorPosition);
    m_CursorPosition = position;
    MoveScreenCursor(previousScreenPosition, GetScreenPosition(m_CursorPosition));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::MoveInHistory(ptrdiff_t distance)
{
    m_PendingExpansionAlternatives.clear();

    if (!m_HistoryEnabled || !m_InputBuffer.empty()) {
        return;
    }
    if (distance < 0)
    {
        if (size_t(-distance) > m_HistoryLocation) {
            distance = -ptrdiff_t(m_HistoryLocation);
        }
    }
    else
    {
        const size_t availableDistance = m_HistoryBuffers.size() - m_HistoryLocation;
        if (size_t(distance) > availableDistance) {
            distance = ptrdiff_t(availableDistance);
        }
    }
    if (distance != 0)
    {
        MoveCursorTo(0);
        const bool hideCursorDuringUpdate = m_DisplayMode == DisplayMode::Wrap;
        if (hideCursorDuringUpdate)
        {
            ShowTerminalCursor(false);
            SendANSICode(PANSI_ControlCode::EraseDisplay);
        }
        if (m_HistoryLocation < m_HistoryBuffers.size())
        {
            m_HistoryBuffers[m_HistoryLocation] = std::move(m_EditBuffer);
            m_EditBuffer.clear();
        }
        m_HistoryLocation = size_t(ptrdiff_t(m_HistoryLocation) + distance);
        if (m_HistoryLocation < m_HistoryBuffers.size()) {
            m_EditBuffer = m_HistoryBuffers[m_HistoryLocation];
        }
        m_CursorPosition = m_EditBuffer.size();
        RefreshText(0);
        if (hideCursorDuringUpdate) {
            ShowTerminalCursor(true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::DeleteChar()
{
    m_PendingExpansionAlternatives.clear();

    if (m_CursorPosition < m_EditBuffer.size())
    {
        size_t characterLength = GetUTF8CharacterLength(m_EditBuffer, m_CursorPosition);

        if (characterLength == 0) {
            characterLength = 1;
        }
        m_EditBuffer.erase(m_CursorPosition, characterLength);
        RefreshText(m_CursorPosition);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::BackspaceChar()
{
    m_PendingExpansionAlternatives.clear();

    if (m_CursorPosition > 0)
    {
        const size_t characterStart = GetPreviousUTF8CharacterOffset(m_EditBuffer, m_CursorPosition);
        const size_t characterLength = m_CursorPosition - characterStart;

        MoveCursorTo(characterStart);
        m_EditBuffer.erase(characterStart, characterLength);
        RefreshText(m_CursorPosition);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::RefreshText(size_t startPosition)
{
    if (m_DisplayMode == DisplayMode::HorizontalScroll)
    {
        RefreshSingleLine(startPosition);
        return;
    }

    WriteText(m_EditBuffer.data() + startPosition, m_EditBuffer.size() - startPosition);

    const PIPoint cursorScreenPosition = GetScreenPosition(m_CursorPosition);
    const PIPoint endScreenPosition = GetScreenPosition(m_EditBuffer.size());

    if (endScreenPosition.x == 0) {
        WriteText(" \010", 2);
    }
    SendANSICode(PANSI_ControlCode::EraseDisplay);

    MoveScreenCursor(endScreenPosition, cursorScreenPosition);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::PrintPendingExpansionAlternatives()
{
    std::vector<CompletionCandidate> alternatives = std::move(m_PendingExpansionAlternatives);

    const size_t cursorPosition = m_CursorPosition;

    MoveCursorTo(m_EditBuffer.size());
    WriteNewline();

    SendANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::FgColor_BrightGreen));

    for (size_t alternativeIndex = 0; alternativeIndex < alternatives.size(); ++alternativeIndex)
    {
        if (alternativeIndex != 0) {
            WriteText(" ", 1);
        }
        WriteText(alternatives[alternativeIndex].Text);
    }

    SendANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::Reset));

    WriteNewline();
    WriteText(m_Prompt);
    RefreshText(0);
    MoveCursorTo(cursorPosition);

    m_PendingExpansionAlternatives = std::move(alternatives);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::ExpandArgument()
{
    if (!m_PendingExpansionAlternatives.empty())
    {
        PrintPendingExpansionAlternatives();
        return;
    }

    const PString lineBuffer = m_InputBuffer + m_EditBuffer;
    const size_t inputPrefixLength = m_InputBuffer.size();
    const size_t cursorPosition = inputPrefixLength + m_CursorPosition;

    if (VFGetExpansionList.Empty()) {
        return;
    }

    PPOSIXTokenizer tokenizer(lineBuffer);

    size_t tokenIndex = 0;
    size_t tokenOffset = 0;
    PString tokenText;

    if (!lineBuffer.empty())
    {
        tokenIndex = tokenizer.GetTokenByPosition(cursorPosition, tokenOffset);
        if (tokenIndex == INVALID_INDEX) {
            return;
        }
        tokenText = tokenizer.GetTokenText(tokenizer.GetTokens()[tokenIndex]);
    }

    const CompletionContext context = {
        .Line           = lineBuffer,
        .TokenText      = tokenText,
        .CursorPosition = cursorPosition,
        .TokenIndex     = tokenIndex,
        .TokenOffset    = tokenOffset
    };
    CompletionResult result = VFGetExpansionList(context);

    if (result.Candidates.empty()) {
        return;
    }
    if (result.ReplaceStartInToken == INVALID_INDEX || result.ReplaceEndInToken == INVALID_INDEX || result.ReplaceStartInToken > result.ReplaceEndInToken) {
        return;
    }

    size_t replaceStart;
    size_t replaceEnd;
    if (lineBuffer.empty())
    {
        replaceStart = result.ReplaceStartInToken;
        replaceEnd = result.ReplaceEndInToken;
    }
    else
    {
        const PPOSIXTokenizer::Token& token = tokenizer.GetTokens()[tokenIndex];
        replaceStart = tokenizer.TokenToGlobalOffset(token, result.ReplaceStartInToken);
        replaceEnd = tokenizer.TokenToGlobalOffset(token, result.ReplaceEndInToken);
    }

    if (replaceStart < inputPrefixLength || replaceEnd < replaceStart || replaceEnd > lineBuffer.size()) {
        return;
    }
    replaceStart -= inputPrefixLength;
    replaceEnd -= inputPrefixLength;

    const size_t replacementLength = replaceEnd - replaceStart;
    PString replacementString;

    bool finalExpansion = true;
    if (result.Candidates.size() == 1)
    {
        replacementString = result.Candidates[0].Text;
        finalExpansion = result.Candidates[0].IsFinal;
    }
    else
    {
        finalExpansion = false;
        const size_t commonStartLength = GetCommonStartLength(result.Candidates);
        if (commonStartLength > replacementLength ||
            (commonStartLength == replacementLength &&
             std::string_view(m_EditBuffer.data() + replaceStart, commonStartLength) != std::string_view(result.Candidates[0].Text.data(), commonStartLength)))
        {
            replacementString = PString(result.Candidates[0].Text.data(), commonStartLength);
        }
    }

    if (!replacementString.empty())
    {
        for (size_t characterIndex = 0; characterIndex < replacementString.size(); ++characterIndex)
        {
            if (replacementString[characterIndex] == ' ')
            {
                replacementString.insert(replacementString.begin() + characterIndex, '\\');
                ++characterIndex;
            }
        }

        MoveCursorTo(replaceStart);

        if (finalExpansion && replaceEnd == m_EditBuffer.size()) {
            replacementString += " ";
        }
        m_CursorPosition = replaceStart + replacementString.size();

        m_EditBuffer.erase(m_EditBuffer.begin() + replaceStart, m_EditBuffer.begin() + replaceEnd);
        m_EditBuffer.insert(m_EditBuffer.begin() + replaceStart, replacementString.begin(), replacementString.end());

        RefreshText(replaceStart);
    }
    else
    {
        m_PendingExpansionAlternatives = std::move(result.Candidates);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTerminalLineEditor::ProcessControlCharacter(PANSI_ControlCode controlCharacter, const std::vector<int>& arguments)
{
    switch (controlCharacter)
    {
        case PANSI_ControlCode::Cancel:
            CancelInput();
            return true;

        case PANSI_ControlCode::Break:
            if (m_EchoControlCharacters) {
                WriteText("^C", 2);
            }
            ClearInput();
            m_InputActive = false;
            VFBreak();
            if (m_EchoNewline) {
                WriteNewline();
            }
            RestartInput();
            return true;

        case PANSI_ControlCode::Disconnect:
            if (m_DisconnectOnEmpty && m_EditBuffer.empty() && m_InputBuffer.empty())
            {
                if (m_EchoControlCharacters) {
                    WriteText("^D", 2);
                }
                if (m_EchoNewline) {
                    WriteNewline();
                }
                m_InputActive = false;
                VFDisconnect();
            }
            else
            {
                if (m_EchoControlCharacters) {
                    WriteText("^D", 2);
                }
                ClearInput();
                m_InputActive = false;
                if (m_EchoNewline) {
                    WriteNewline();
                }
                RestartInput();
            }
            return true;

        case PANSI_ControlCode::Backspace:
            if (m_CursorPosition == 0 &&
                m_BackspaceAtStartAction == BackspaceAtStartAction::CancelInput)
            {
                CancelInput();
                return true;
            }
            BackspaceChar();
            break;

        case PANSI_ControlCode::XTerm_Left:
            MoveCursor(-1);
            break;

        case PANSI_ControlCode::XTerm_Right:
            MoveCursor(1);
            break;

        case PANSI_ControlCode::XTerm_Up:
            MoveInHistory(-1);
            break;

        case PANSI_ControlCode::XTerm_Down:
            MoveInHistory(1);
            break;

        case PANSI_ControlCode::XTerm_End:
            MoveToEnd();
            break;

        case PANSI_ControlCode::XTerm_Home:
            MoveToHome();
            break;

        case PANSI_ControlCode::Tab:
            ExpandArgument();
            break;

        case PANSI_ControlCode::VT_Keycode:
            if (!arguments.empty())
            {
                switch (PANSI_VT_KeyCodes(arguments[0]))
                {
                    case PANSI_VT_KeyCodes::Home_1:
                    case PANSI_VT_KeyCodes::Home_7:
                        MoveToHome();
                        break;

                    case PANSI_VT_KeyCodes::End_4:
                    case PANSI_VT_KeyCodes::End_8:
                        MoveToEnd();
                        break;

                    case PANSI_VT_KeyCodes::Delete:
                        DeleteChar();
                        break;

                    case PANSI_VT_KeyCodes::PgUp:
                        MoveToHistoryStart();
                        break;

                    case PANSI_VT_KeyCodes::PgDn:
                        MoveToHistoryEnd();
                        break;

                    case PANSI_VT_KeyCodes::Insert:
                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }
    return false;
}
