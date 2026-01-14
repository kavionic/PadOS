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

#pragma once

#include <Math/point.h>
#include <Utils/ANSIEscapeCodeParser.h>

#include <Kernel/KThread.h>
#include <Kernel/DebugConsole/KConsoleCommand.h>


namespace kernel
{

class KDebugConsole : public KThread
{
public:
    KDebugConsole();

    static KDebugConsole& Get();

    void Setup();

    virtual void* Run() override;

    void AddInputText(const char* text, size_t length);
    void EnterPressed();

    void SendText(const char* text, size_t length);
    void SendText(const PString& text) { SendText(text.c_str(), text.size()); }

    template<typename... TArgTypes>
    void SendANSICode(PANSIControlCode code, TArgTypes ...args)
    {
        SendText(m_ANSICodeParser.FormatANSICode(code, std::forward<TArgTypes>(args)...));
    }
    
    PIPoint GetScreenPosition(size_t cursorPosition) const;
    void    MoveScreenCursor(const PIPoint& startPos, const PIPoint& endPos);

    void MoveCursor(int distance);
    void MoveToHome() { MoveCursor(-m_CursorPosition); }
    void MoveToEnd() { MoveCursor(m_EditBuffer.size() - m_CursorPosition); }

    void MoveInHistory(int distance);
    void MoveToHistoryStart() { MoveInHistory(-m_HistoryLocation); }
    void MoveToHistoryEnd() { MoveInHistory(m_HistoryBuffers.size() - m_HistoryLocation); }

    void DeleteChar();
    void BackspaceChar();

    void RefreshText(size_t startPosition);

    const std::map<PString, Ptr<KConsoleCommand>>& GetCommands() const { return m_Commands; }

    void RegisterCommand(const PString& name, Ptr<KConsoleCommand> command) { m_Commands[name] = command; }

private:
    void ProcessCmdLine(const PString& lineBuffer);
    void ProcessControlChar(PANSIControlCode controlChar, const std::vector<int>& args);

    std::vector<std::string> Tokenize(const PString& text);

    static KDebugConsole s_Instance;

    PANSIEscapeCodeParser m_ANSICodeParser;

    PString m_Prompt = "$ ";

    PIPoint m_TerminalSize = PIPoint(80, 24);

    PString m_EditBuffer;
    std::vector<PString> m_HistoryBuffers;

    size_t  m_HistoryLocation = 0;
    size_t  m_CursorPosition = 0;

    std::map<PString, Ptr<KConsoleCommand>> m_Commands;
};

} // namespace kernel
