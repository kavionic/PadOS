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
// Created: 09.01.2026 22:00

#include <vector>
#include <cctype>
#include <stdexcept>

#include <PadOS/Time.h>
#include <Utils/Logging.h>
#include <Kernel/KLogging.h>
#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/VFS/FileIO.h>

namespace kernel
{
KDebugConsole KDebugConsole::s_Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDebugConsole::KDebugConsole() : KThread("debug_console")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDebugConsole& KDebugConsole::Get()
{
    return s_Instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::Setup()
{
    Start_trw();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KDebugConsole::Run()
{
    SendText(m_Prompt);

    TimeValNanos nextSizeQueryTime;

    for(;;)
    {
        try
        {
            char buffer[32];
            const size_t length = kread_trw(0, buffer, sizeof(buffer));

            const TimeValNanos curTime = get_monotonic_time();

            if (curTime >= nextSizeQueryTime)
            {
                nextSizeQueryTime = curTime + TimeValNanos::FromMilliseconds(250);
                SendANSICode(PANSIControlCode::XTerm_XTWINOPS, 18);
            }

            size_t start = 0;
            for (size_t i = 0; i <= length; ++i)
            {
                if (i != length)
                {
                    PANSIControlCode controlChar = m_ANSICodeParser.ProcessCharacter(buffer[i]);
                    if (controlChar != PANSIControlCode::None)
                    {
                        start = i + 1;
                        if (controlChar != PANSIControlCode::Pending) {
                            ProcessControlChar(controlChar, m_ANSICodeParser.GetArguments());
                        }
                        continue;
                    }
                }

                if (i == length || buffer[i] == '\r' || buffer[i] == '\n')
                {
                    const size_t bytesAdded = i - start;

                    if (bytesAdded > 0) {
                        AddInputText(&buffer[start], bytesAdded);
                    }

                    if (i != length)
                    {
                        start = i + 1;
                        if (buffer[i] != '\n') {
                            EnterPressed();
                        }
                    }
                }
            }
        }
        catch(std::exception& exc)
        {
            kprintf("ERROR: %s\n", exc.what());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::AddInputText(const char* text, size_t length)
{
    m_EditBuffer.insert(m_EditBuffer.begin() + m_CursorPosition, text, text + length);

    m_CursorPosition += length;
    if (m_CursorPosition == m_EditBuffer.size())
    {
        SendText(text, length);

        const PIPoint screenPos = GetScreenPosition(m_CursorPosition);
        if (screenPos.x == 0) {
            SendText(" \010", 2); // Punch through "pending wrap".
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

void KDebugConsole::EnterPressed()
{
    write(1, "\n", 1);
    if (!m_EditBuffer.empty())
    {
        ProcessCmdLine(m_EditBuffer);
        m_HistoryBuffers.push_back(std::move(m_EditBuffer));
        m_EditBuffer.clear();

        m_CursorPosition = 0;
    }
    m_HistoryLocation = m_HistoryBuffers.size();
    SendText(m_Prompt);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::SendText(const char* text, size_t length)
{
    write(1, text, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIPoint KDebugConsole::GetScreenPosition(size_t cursorPosition) const
{
    return PIPoint((m_Prompt.size() + cursorPosition) % m_TerminalSize.x, (m_Prompt.size() + cursorPosition) / m_TerminalSize.x);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::MoveScreenCursor(const PIPoint& startPos, const PIPoint& endPos)
{
    const PIPoint delta = endPos - startPos;

    if (delta.y < 0) {
        SendANSICode(PANSIControlCode::XTerm_Up, -delta.y);
    } else if (delta.y > 0) {
        SendANSICode(PANSIControlCode::XTerm_Down, delta.y);
    }
    if (delta.x < 0) {
        SendANSICode(PANSIControlCode::XTerm_Left, -delta.x);
    } else if (delta.x > 0) {
        SendANSICode(PANSIControlCode::XTerm_Right, delta.x);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::MoveCursor(int distance)
{
    if (distance < 0) {
        if (-distance > m_CursorPosition) distance = -m_CursorPosition;
    } else {
        if (m_CursorPosition + distance > m_EditBuffer.size()) distance = m_EditBuffer.size() - m_CursorPosition;
    }

    if (distance != 0)
    {
        const PIPoint preScreenPos = GetScreenPosition(m_CursorPosition);
        m_CursorPosition += distance;
        const PIPoint postScreenPos = GetScreenPosition(m_CursorPosition);

        MoveScreenCursor(preScreenPos, postScreenPos);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::MoveInHistory(int distance)
{
    if (distance < 0) {
        if (-distance > m_HistoryLocation) distance = -m_HistoryLocation;
    } else {
        if (m_HistoryLocation + distance > m_HistoryBuffers.size()) distance = m_HistoryBuffers.size() - m_HistoryLocation;
    }
    if (distance != 0)
    {
        MoveCursor(-m_CursorPosition);
        if (m_HistoryLocation < m_HistoryBuffers.size())
        {
            m_HistoryBuffers[m_HistoryLocation] = std::move(m_EditBuffer);
            m_EditBuffer.clear();
        }
        m_HistoryLocation += distance;
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

void KDebugConsole::DeleteChar()
{
    if (m_CursorPosition < m_EditBuffer.size())
    {
        m_EditBuffer.erase(m_EditBuffer.begin() + m_CursorPosition);
        RefreshText(m_CursorPosition);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::BackspaceChar()
{
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

void KDebugConsole::RefreshText(size_t startPosition)
{
    const size_t length = m_EditBuffer.size() - startPosition;

    SendText(&m_EditBuffer[startPosition], length);

    const PIPoint postScreenPos = GetScreenPosition(m_CursorPosition);
    const PIPoint preScreenPos = GetScreenPosition(m_EditBuffer.size());

    if (preScreenPos.x == 0) {
        SendText(" \010", 2); // Punch through "pending wrap".
    }
    SendANSICode(PANSIControlCode::EraseDisplay);

    MoveScreenCursor(preScreenPos, postScreenPos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::ProcessCmdLine(const PString& lineBuffer)
{
    std::vector<std::string> tokens = Tokenize(lineBuffer);

    if (!tokens.empty())
    {
        auto cmdIt = m_Commands.find(tokens[0]);

        if (cmdIt != m_Commands.end())
        {
            Ptr<KConsoleCommand>& cmd = cmdIt->second;

            try {
                cmd->Invoke(std::move(tokens));
            } catch(const std::exception& exc) {
                kprintf("%s\n", exc.what());
            }
        }
        else
        {
            kprintf("Unknown command: %s\n", tokens[0].c_str());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::ProcessControlChar(PANSIControlCode controlChar, const std::vector<int>& args)
{
    switch(controlChar)
    {
        case PANSIControlCode::XTerm_Left:
            MoveCursor(-1);
            break;
        case PANSIControlCode::XTerm_Right:
            MoveCursor(1);
            break;
        case PANSIControlCode::XTerm_Up:
            MoveInHistory(-1);
            break;
        case PANSIControlCode::XTerm_Down:
            MoveInHistory(1);
            break;
        case PANSIControlCode::XTerm_End:
            MoveToEnd();
            break;
        case PANSIControlCode::XTerm_Home:
            MoveToHome();
            break;
        case PANSIControlCode::Backspace:
            BackspaceChar();
            break;
        case PANSIControlCode::VT_Keycode:
            if (args.size() >= 1)
            {
                switch(PANSI_VT_KeyCodes(args[0]))
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
                        break;
                    default:
                        break;
                }
            }
            break;
        case PANSIControlCode::XTerm_XTWINOPS:
            if (args.size() >= 3 && args[0] == 8) {
                m_TerminalSize = PIPoint(args[2], args[1]);
            }
            break;
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<std::string> KDebugConsole::Tokenize(const PString& text)
{
    enum class QuoteMode { None, InSingle, InDouble };

    QuoteMode                quouteMode = QuoteMode::None;
    std::vector<std::string> tokenList;
    std::string              currentToken;

    bool forceTokenSave = false;

    auto push_token = [&tokenList, &currentToken, &forceTokenSave]()
        {
            if (forceTokenSave || !currentToken.empty())
            {
                tokenList.push_back(std::move(currentToken));
                currentToken.clear();
                forceTokenSave = false;
            }
        };

    auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\n'; };

    for (size_t i = 0; i < text.size(); ++i)
    {
        char character = text[i];

        switch (quouteMode)
        {
            case QuoteMode::None:
                if (is_space(character))
                {
                    push_token();
                }
                else if (character == '\'')
                {
                    quouteMode = QuoteMode::InSingle;
                    forceTokenSave = true; // Allow '' to produce empty token.
                }
                else if (character == '"')
                {
                    quouteMode = QuoteMode::InDouble;
                    forceTokenSave = true; // Allow "" to produce empty token.
                }
                else if (character == '\\')
                {
                    if (i + 1 >= text.size()) {
                        throw std::runtime_error("tokenize: trailing backslash");
                    }
                    const char nextChar = text[i + 1];
                    if (nextChar == '\n')
                    {
                        ++i; // Remove both \ and newline (line continuation).
                    }
                    else
                    {
                        currentToken.push_back(nextChar);
                        ++i;
                    }
                }
                else
                {
                    currentToken.push_back(character);
                }
                break;

            case QuoteMode::InSingle:
                if (character == '\'') {
                    quouteMode = QuoteMode::None;
                } else {
                    currentToken.push_back(character);
                }
                break;

            case QuoteMode::InDouble:
                if (character == '"')
                {
                    quouteMode = QuoteMode::None;
                }
                else if (character == '\\')
                {
                    if (i + 1 >= text.size()) throw std::runtime_error("tokenize: trailing backslash in double quotes");
                    const char nextChar = text[i + 1];
                    // POSIX: backslash only special before \, ", $, `, or newline.
                    if (nextChar == '\\' || nextChar == '"' || nextChar == '$' || nextChar == '`')
                    {
                        currentToken.push_back(nextChar);
                        ++i;
                    }
                    else if (nextChar == '\n')
                    {
                        ++i; // Remove both \ and newline (line continuation).
                    }
                    else
                    {
                        currentToken.push_back('\\'); // Backslash preserved literally.
                    }
                }
                else
                {
                    currentToken.push_back(character);
                }
                break;
        }
    }

    if (quouteMode == QuoteMode::InSingle) throw std::runtime_error("tokenize: unmatched single quote");
    if (quouteMode == QuoteMode::InDouble) throw std::runtime_error("tokenize: unmatched double quote");

    push_token();
    return tokenList;
}


} // namespace kernel
