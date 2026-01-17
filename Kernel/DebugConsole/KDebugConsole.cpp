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
#include <Utils/POSIXTokenizer.h>
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
                    const PANSIControlCode controlChar = m_ANSICodeParser.ProcessCharacter(buffer[i]);
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

                    start = i + 1;
                    if (i != length && buffer[i] == '\r')
                    {
                        EnterPressed();
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

    const PString& lineBuffer = m_InputBuffer.empty() ? m_EditBuffer : (m_InputBuffer + m_EditBuffer);

    if (!lineBuffer.empty())
    {
        PPOSIXTokenizer tokenizer(lineBuffer);

        if (tokenizer.GetTermination() == PPOSIXTokenizer::Termination::Normal)
        {
            m_InputBuffer.clear();

            m_HistoryBuffers.push_back(lineBuffer);
            m_HistoryLocation = m_HistoryBuffers.size();

            m_EditBuffer.clear();
            m_CursorPosition = 0;

            m_Prompt = m_CmdPrompt;
            ProcessCmdLine(std::move(tokenizer));
        }
        else
        {
            m_InputBuffer += m_EditBuffer + "\n";

            m_EditBuffer.clear();
            m_CursorPosition = 0;

            m_Prompt = m_EditPrompt;
        }
    }
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

void KDebugConsole::ShowTerminalCursor(bool show)
{
    SendText(show ? "\033[?25h" : "\033[?25l");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIPoint KDebugConsole::GetScreenPosition(size_t cursorPosition) const
{
    const size_t promptLength = m_Prompt.size();
    PIPoint      position(promptLength, 0);

    for (size_t i = 0; i < cursorPosition; ++i)
    {
        if (m_EditBuffer[i] == '\n' || ((promptLength + i) % m_TerminalSize.x) == 0)
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

void KDebugConsole::MoveScreenCursor(const PIPoint& startPos, const PIPoint& endPos)
{
    const PIPoint delta = endPos - startPos;
    ShowTerminalCursor(false);
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
    ShowTerminalCursor(true);
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
    if (!m_InputBuffer.empty()) {
        return; // No navigation during multi-line input.
    }
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

void KDebugConsole::ResetInput()
{
    const PString& lineBuffer = m_InputBuffer.empty() ? m_EditBuffer : (m_InputBuffer + m_EditBuffer);

    if (!lineBuffer.empty()) {
        m_HistoryBuffers.push_back(lineBuffer);
    }

    m_InputBuffer.clear();
    m_EditBuffer.clear();
    m_CursorPosition = 0;
    m_HistoryLocation = m_HistoryBuffers.size();
    
    m_Prompt = m_CmdPrompt;

    SendText("\n", 1);
    SendText(m_Prompt);
    SendANSICode(PANSIControlCode::EraseDisplay);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::ProcessCmdLine(PPOSIXTokenizer&& tokenizer)
{
    std::vector<std::string> tokens;

    for (const PPOSIXTokenizer::Token& token : tokenizer.GetTokens()) {
        tokens.push_back(tokenizer.GetTokenText(token));
    }

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
        case PANSIControlCode::Break:
            SendText("^C", 2);
            ResetInput();
            break;
        case PANSIControlCode::Disconnect:
            SendText("^D", 2);
            ResetInput();
            break;
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


} // namespace kernel
