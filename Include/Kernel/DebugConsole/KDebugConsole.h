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

class PPOSIXTokenizer;

namespace kernel
{

class KDebugConsole : public KThread
{
public:
    KDebugConsole(int stdInFD, int stdOutFD, int stdErrFD);
    KDebugConsole(const PString& portPath);

    void Setup();

    virtual void* Run() override;

    int GetStdInFD() const  { return m_StdInFD; }
    int GetStdOutFD() const { return m_StdOutFD; }
    int GetStdErrFD() const { return m_StdErrFD; }

    void AddInputText(const char* text, size_t length);
    void EnterPressed();

    void SendText(const char* text, size_t length);
    void SendText(const PString& text) { SendText(text.c_str(), text.size()); }
    void SendNewline() { SendText("\n", 1); }

    template<typename... TArgTypes>
    void SendANSICode(PANSI_ControlCode code, TArgTypes ...args)
    {
        SendText(m_ANSICodeParser.FormatANSICode(code, std::forward<TArgTypes>(args)...));
    }
    
    void ShowTerminalCursor(bool show);

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
    void ResetInput();

    static const std::map<PString, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>>& GetCommands() { return s_Commands; }

    static void RegisterCommand(const PString& name, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>&& commandCreator) { s_Commands[name] = std::move(commandCreator); }

    bool SetPrompt(const PString& text);

    void UpdatePrompt();
private:
    static size_t GetCommonStartLength(const std::vector<PString>& alternatives);

    void PrintPendingExpansionAlternatives();

    void                    ExpandArgument();
    std::vector<PString>    ExpandCommandName(size_t argumentIndex, const PString& argumentText, size_t argumentOffset, size_t& outReplaceStart, size_t& outReplaceEnd);
    std::vector<PString>    ExpandFilePath(size_t argumentIndex, const PString& argumentText, size_t argumentOffset, size_t& outReplaceStart, size_t& outReplaceEnd);

    void ProcessCmdLine(PPOSIXTokenizer&& tokenizer);
    void ProcessControlChar(PANSI_ControlCode controlChar, const std::vector<int>& args);


    PANSIEscapeCodeParser m_ANSICodeParser;

    PString m_PortPath;
    int m_StdInFD = -1;
    int m_StdOutFD = -1;
    int m_StdErrFD = -1;

    PString m_CmdPrompt = "$ ";
    PString m_EditPrompt = "> ";
    PString m_Prompt = m_CmdPrompt;

    PIPoint m_TerminalSize = PIPoint(80, 24);

    PString m_InputBuffer;
    PString m_EditBuffer;
    std::vector<PString> m_HistoryBuffers;

    size_t  m_HistoryLocation = 0;
    size_t  m_CursorPosition = 0;

    std::vector<PString> m_PendingExpansionAlternatives;

    static std::map<PString, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>> s_Commands;
};

} // namespace kernel
