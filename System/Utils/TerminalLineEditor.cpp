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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::Initialize(int inputFD, int outputFD)
{
    m_InputFD = inputFD;
    m_OutputFD = outputFD;

    UpdateTerminalSize();

    m_Prompt = m_PrimaryPrompt;
    m_PromptVisibleLength = m_PrimaryPromptVisibleLength;

    WriteText(m_Prompt);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::SetPrimaryPrompt(const PString& text, size_t visibleLength)
{
    m_PrimaryPrompt = text;
    m_PrimaryPromptVisibleLength = visibleLength;
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
    char buffer[32];

    ssize_t length;
    do {
        length = read(m_InputFD, buffer, sizeof(buffer));
    } while (length < 0 && errno == EINTR);

    if (length > 0)
    {
        UpdateTerminalSize();
        ProcessInput(buffer, size_t(length));
        return ReadResult::DataRead;
    }
    if (length == 0) {
        return ReadResult::EndOfInput;
    }
    return ReadResult::Error;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::ResetInput()
{
    m_PendingExpansionAlternatives.clear();

    const PString lineBuffer = m_InputBuffer + m_EditBuffer;

    if (!lineBuffer.empty()) {
        m_HistoryBuffers.push_back(lineBuffer);
    }

    m_InputBuffer.clear();
    m_EditBuffer.clear();
    m_CursorPosition = 0;
    m_HistoryLocation = m_HistoryBuffers.size();

    m_Prompt = m_PrimaryPrompt;
    m_PromptVisibleLength = m_PrimaryPromptVisibleLength;

    WriteNewline();
    WriteText(m_Prompt);
    SendANSICode(PANSI_ControlCode::EraseDisplay);
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

    for (size_t commonStartLength = 0; ; ++commonStartLength)
    {
        for (size_t candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex)
        {
            const PString& text = candidates[candidateIndex].Text;

            if (text.size() <= commonStartLength) {
                return commonStartLength;
            }
            if (candidateIndex != 0 && text[commonStartLength] != candidates[0].Text[commonStartLength]) {
                return commonStartLength;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::ProcessInput(const char* text, size_t length)
{
    size_t start = 0;
    for (size_t characterIndex = 0; characterIndex <= length; ++characterIndex)
    {
        if (characterIndex != length)
        {
            const PANSI_ControlCode controlCharacter = m_ANSICodeParser.ProcessCharacter(text[characterIndex]);
            if (controlCharacter != PANSI_ControlCode::None)
            {
                const size_t bytesAdded = characterIndex - start;
                if (bytesAdded > 0) {
                    AddInputText(&text[start], bytesAdded);
                }
                start = characterIndex + 1;
                if (controlCharacter != PANSI_ControlCode::Pending) {
                    ProcessControlCharacter(controlCharacter, m_ANSICodeParser.GetArguments());
                }
                continue;
            }
        }

        if (characterIndex == length || text[characterIndex] == '\r' || text[characterIndex] == '\n')
        {
            const size_t bytesAdded = characterIndex - start;

            if (bytesAdded > 0) {
                AddInputText(&text[start], bytesAdded);
            }

            start = characterIndex + 1;
            if (characterIndex != length && text[characterIndex] == '\r') {
                SubmitLine();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::AddInputText(const char* text, size_t length)
{
    m_PendingExpansionAlternatives.clear();

    m_EditBuffer.insert(m_EditBuffer.begin() + m_CursorPosition, text, text + length);

    m_CursorPosition += length;
    if (m_CursorPosition == m_EditBuffer.size())
    {
        WriteText(text, length);

        const PIPoint screenPosition = GetScreenPosition(m_CursorPosition);
        if (screenPosition.x == 0) {
            WriteText(" \010", 2);
        }
    }
    else
    {
        RefreshText(m_CursorPosition - length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::SubmitLine()
{
    WriteNewline();

    m_PendingExpansionAlternatives.clear();

    const PString lineBuffer = m_InputBuffer + m_EditBuffer;

    if (m_InputMode == PTerminalLineEditor::InputMode::POSIXCommand)
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
            return;
        }
    }

    if (!lineBuffer.empty()) {
        m_HistoryBuffers.push_back(lineBuffer);
    }

    m_InputBuffer.clear();
    m_EditBuffer.clear();
    m_CursorPosition = 0;
    m_HistoryLocation = m_HistoryBuffers.size();

    VFLineSubmitted(lineBuffer);

    m_Prompt = m_PrimaryPrompt;
    m_PromptVisibleLength = m_PrimaryPromptVisibleLength;
    WriteText(m_Prompt);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::WriteText(const char* text, size_t length) const
{
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

PIPoint PTerminalLineEditor::GetScreenPosition(size_t cursorPosition) const
{
    PIPoint position(m_PromptVisibleLength, 0);

    for (size_t characterIndex = 0; characterIndex < cursorPosition; ++characterIndex)
    {
        if (m_EditBuffer[characterIndex] == '\n' || ((m_PromptVisibleLength + characterIndex) % size_t(m_TerminalSize.x)) == 0)
        {
            position.x = 0;
            position.y++;
        }
        else
        {
            position.x++;
        }
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

    if (distance < 0)
    {
        if (size_t(-distance) > m_CursorPosition) {
            distance = -ptrdiff_t(m_CursorPosition);
        }
    }
    else
    {
        const size_t availableDistance = m_EditBuffer.size() - m_CursorPosition;
        if (size_t(distance) > availableDistance) {
            distance = ptrdiff_t(availableDistance);
        }
    }

    if (distance != 0)
    {
        const PIPoint previousScreenPosition = GetScreenPosition(m_CursorPosition);
        m_CursorPosition = size_t(ptrdiff_t(m_CursorPosition) + distance);
        const PIPoint newScreenPosition = GetScreenPosition(m_CursorPosition);

        MoveScreenCursor(previousScreenPosition, newScreenPosition);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::MoveInHistory(ptrdiff_t distance)
{
    m_PendingExpansionAlternatives.clear();

    if (!m_InputBuffer.empty()) {
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
        MoveCursor(-ptrdiff_t(m_CursorPosition));
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
        m_EditBuffer.erase(m_EditBuffer.begin() + m_CursorPosition);
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
        MoveCursor(-1);
        m_EditBuffer.erase(m_EditBuffer.begin() + m_CursorPosition);
        RefreshText(m_CursorPosition);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTerminalLineEditor::RefreshText(size_t startPosition)
{
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

    MoveCursor(ptrdiff_t(m_EditBuffer.size() - m_CursorPosition));
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
    MoveCursor(ptrdiff_t(cursorPosition));

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

        MoveCursor(ptrdiff_t(replaceStart) - ptrdiff_t(m_CursorPosition));

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

void PTerminalLineEditor::ProcessControlCharacter(PANSI_ControlCode controlCharacter, const std::vector<int>& arguments)
{
    switch (controlCharacter)
    {
        case PANSI_ControlCode::Break:
            WriteText("^C", 2);
            ResetInput();
            VFBreak();
            break;

        case PANSI_ControlCode::Disconnect:
            if (m_DisconnectOnEmpty && m_EditBuffer.empty() && m_InputBuffer.empty())
            {
                WriteText("^D\n", 3);
                VFDisconnect();
            }
            else
            {
                WriteText("^D", 2);
                ResetInput();
            }
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

        case PANSI_ControlCode::Backspace:
            BackspaceChar();
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
}
