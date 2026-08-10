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

#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <Math/point.h>
#include <Signals/VFConnector.h>
#include <Utils/ANSIEscapeCodeParser.h>
#include <Utils/String.h>


class PTerminalLineEditor
{
public:
    enum class InputMode : uint8_t
    {
        SingleLine,
        POSIXCommand
    };

    enum class ReadResult : uint8_t
    {
        DataRead,
        EndOfInput,
        Error
    };

    struct CompletionCandidate
    {
        PString Text;
        bool    IsFinal = true;
    };

    struct CompletionContext
    {
        const PString& Line;
        const PString& TokenText;
        size_t         CursorPosition = 0;
        size_t         TokenIndex = 0;
        size_t         TokenOffset = 0;
    };

    struct CompletionResult
    {
        std::vector<CompletionCandidate> Candidates;
        size_t ReplaceStartInToken = INVALID_INDEX;
        size_t ReplaceEndInToken = INVALID_INDEX;
    };

public:
    PTerminalLineEditor() = default;

    void Initialize(int inputFD, int outputFD);

    int  GetInputFileDescriptor() const { return m_InputFD; }
    int  GetOutputFileDescriptor() const { return m_OutputFD; }

    void        SetInputMode(InputMode inputMode) { m_InputMode = inputMode; }
    InputMode   GetInputMode() const { return m_InputMode; }

    void SetDisconnectOnEmpty(bool disconnectOnEmpty) { m_DisconnectOnEmpty = disconnectOnEmpty; }
    bool GetDisconnectOnEmpty() const { return m_DisconnectOnEmpty; }

    void SetPrimaryPrompt(const PString& text, size_t visibleLength);
    void SetContinuationPrompt(const PString& text, size_t visibleLength);

    ReadResult ReadInput();

    void ResetInput();

public:
    VFConnector<void (const PString& line)> VFLineSubmitted;
    VFConnector<void ()>                    VFBreak;
    VFConnector<void ()>                    VFDisconnect;
    VFConnector<CompletionResult(const CompletionContext& context)> VFGetExpansionList;

private:
    static size_t GetCommonStartLength(const std::vector<CompletionCandidate>& candidates);

    void ProcessInput(const char* text, size_t length);
    void AddInputText(const char* text, size_t length);
    void SubmitLine();

    void WriteText(const char* text, size_t length) const;
    void WriteText(const PString& text) const { WriteText(text.c_str(), text.size()); }
    void WriteNewline() const { WriteText("\n", 1); }

    template<typename... TArgTypes>
    void SendANSICode(PANSI_ControlCode code, TArgTypes ...args) const
    {
        WriteText(PANSIEscapeCodeParser::FormatANSICode(code, std::forward<TArgTypes>(args)...));
    }

    void UpdateTerminalSize();
    void ShowTerminalCursor(bool show);

    PIPoint GetScreenPosition(size_t cursorPosition) const;
    void    MoveScreenCursor(const PIPoint& startPosition, const PIPoint& endPosition);

    void MoveCursor(ptrdiff_t distance);
    void MoveToHome() { MoveCursor(-ptrdiff_t(m_CursorPosition)); }
    void MoveToEnd() { MoveCursor(ptrdiff_t(m_EditBuffer.size() - m_CursorPosition)); }

    void MoveInHistory(ptrdiff_t distance);
    void MoveToHistoryStart() { MoveInHistory(-ptrdiff_t(m_HistoryLocation)); }
    void MoveToHistoryEnd() { MoveInHistory(ptrdiff_t(m_HistoryBuffers.size() - m_HistoryLocation)); }

    void DeleteChar();
    void BackspaceChar();

    void RefreshText(size_t startPosition);

    void PrintPendingExpansionAlternatives();
    void ExpandArgument();

    void ProcessControlCharacter(PANSI_ControlCode controlCharacter, const std::vector<int>& arguments);

    PANSIEscapeCodeParser m_ANSICodeParser;

    int m_InputFD = -1;
    int m_OutputFD = -1;

    PString m_PrimaryPrompt = "$ ";
    PString m_ContinuationPrompt = "> ";
    PString m_Prompt = m_PrimaryPrompt;

    size_t m_PrimaryPromptVisibleLength = m_PrimaryPrompt.size();
    size_t m_ContinuationPromptVisibleLength = m_ContinuationPrompt.size();
    size_t m_PromptVisibleLength = m_Prompt.size();

    PIPoint m_TerminalSize = PIPoint(80, 24);

    PString m_InputBuffer;
    PString m_EditBuffer;
    std::vector<PString> m_HistoryBuffers;

    size_t m_HistoryLocation = 0;
    size_t m_CursorPosition = 0;

    std::vector<CompletionCandidate> m_PendingExpansionAlternatives;

    InputMode m_InputMode = InputMode::SingleLine;
    bool m_DisconnectOnEmpty = true;
};
