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
#include <string_view>
#include <utility>
#include <vector>

#include <Math/point.h>
#include <Signals/VFConnector.h>
#include <Utils/ANSIEscapeCodeParser.h>
#include <Utils/String.h>


class PTerminalLineEditor
{
public:
    enum class SubmissionMode : uint8_t
    {
        AlwaysSubmit,
        ContinueIncomplete
    };

    enum class DisplayMode : uint8_t
    {
        Wrap,
        HorizontalScroll
    };

    enum class BackspaceAtStartAction : uint8_t
    {
        Ignore,
        CancelInput
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
    static constexpr int DISABLED_CONTROL_CHARACTER = -1;

    PTerminalLineEditor() = default;

    void Initialize(int inputFD, int outputFD);

    void BeginInput(const PString& initialText = {});
    void ResetInput();

    size_t AddInput(const char* text, size_t length);

    bool IsInputActive() const { return m_InputActive; }

    int  GetInputFileDescriptor() const { return m_InputFD; }
    int  GetOutputFileDescriptor() const { return m_OutputFD; }

    void           SetSubmissionMode(SubmissionMode submissionMode) { m_SubmissionMode = submissionMode; }
    SubmissionMode GetSubmissionMode() const { return m_SubmissionMode; }

    void        SetDisplayMode(DisplayMode displayMode) { m_DisplayMode = displayMode; }
    DisplayMode GetDisplayMode() const { return m_DisplayMode; }

    void SetDisconnectOnEmpty(bool disconnectOnEmpty) { m_DisconnectOnEmpty = disconnectOnEmpty; }
    bool GetDisconnectOnEmpty() const { return m_DisconnectOnEmpty; }

    void SetBreakCharacter(int character) { m_BreakCharacter = character; }
    int  GetBreakCharacter() const { return m_BreakCharacter; }

    void SetDisconnectCharacter(int character) { m_DisconnectCharacter = character; }
    int  GetDisconnectCharacter() const { return m_DisconnectCharacter; }

    void SetCancelCharacter(int character) {
        m_CancelCharacter = character;
    }
    int GetCancelCharacter() const {
        return m_CancelCharacter;
    }

    void SetBackspaceAtStartAction(BackspaceAtStartAction action) {
        m_BackspaceAtStartAction = action;
    }
    BackspaceAtStartAction GetBackspaceAtStartAction() const {
        return m_BackspaceAtStartAction;
    }

    void SetAutoRestart(bool autoRestart) { m_AutoRestart = autoRestart; }
    bool GetAutoRestart() const { return m_AutoRestart; }

    void SetEchoNewline(bool echoNewline) { m_EchoNewline = echoNewline; }
    bool GetEchoNewline() const { return m_EchoNewline; }

    void SetEchoControlCharacters(bool echoControlCharacters) { m_EchoControlCharacters = echoControlCharacters; }
    bool GetEchoControlCharacters() const { return m_EchoControlCharacters; }

    void SetHistoryEnabled(bool enabled) { m_HistoryEnabled = enabled; }
    bool GetHistoryEnabled() const { return m_HistoryEnabled; }

    void SetPrimaryPrompt(const PString& text, size_t visibleLength);
    void SetContinuationPrompt(const PString& text, size_t visibleLength);

    ReadResult ReadInput();

public:
    VFConnector<void (const PString& line)> VFLineSubmitted;
    VFConnector<void ()>                    VFBreak;
    VFConnector<void ()>                    VFDisconnect;
    VFConnector<CompletionResult(const CompletionContext& context)> VFGetExpansionList;

private:
    static size_t GetCommonStartLength(const std::vector<CompletionCandidate>& candidates);
    static size_t GetUTF8CharacterLength(std::string_view text, size_t offset);
    static size_t GetPreviousUTF8CharacterOffset(std::string_view text, size_t offset);
    static size_t GetUTF8CharacterCount(std::string_view text, size_t startOffset, size_t endOffset);

    void AddInputText(const char* text, size_t length);
    void InsertInputText(const char* text, size_t length);
    void FlushIncompleteUTF8Text();
    bool SubmitLine();

    void ClearInput();
    void CancelInput();
    void RestartInput();

    void WriteText(const char* text, size_t length) const;
    void WriteText(const PString& text) const { WriteText(text.c_str(), text.size()); }
    void WriteNewline() const { WriteText("\n", 1); }

    template<typename... TArgTypes>
    void SendANSICode(PANSI_ControlCode code, TArgTypes ...args) const
    {
        WriteText(PANSIEscapeCodeParser::FormatANSICode(code, std::forward<TArgTypes>(args)...));
    }

    void    UpdateTerminalSize();
    void    ShowTerminalCursor(bool show);
    size_t  GetSingleLineEditWidth() const;
    size_t  GetSingleLineCursorColumn() const;
    size_t  GetSingleLineVisibleEnd() const;
    bool    UpdateSingleLineHorizontalOffset();
    void    RefreshSingleLine(size_t startPosition);
    void    RenderSingleLine();

    PIPoint GetScreenPosition(size_t cursorPosition) const;
    void    MoveScreenCursor(const PIPoint& startPosition, const PIPoint& endPosition);

    void MoveCursor(ptrdiff_t distance);
    void MoveCursorTo(size_t position);
    void MoveToHome() { MoveCursorTo(0); }
    void MoveToEnd() { MoveCursorTo(m_EditBuffer.size()); }

    void MoveInHistory(ptrdiff_t distance);
    void MoveToHistoryStart() { MoveInHistory(-ptrdiff_t(m_HistoryLocation)); }
    void MoveToHistoryEnd() { MoveInHistory(ptrdiff_t(m_HistoryBuffers.size() - m_HistoryLocation)); }

    void DeleteChar();
    void BackspaceChar();

    void RefreshText(size_t startPosition);

    void PrintPendingExpansionAlternatives();
    void ExpandArgument();
    bool ProcessControlCharacter(PANSI_ControlCode controlCharacter, const std::vector<int>& arguments);

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
    PString m_IncompleteUTF8Text;
    PString m_PendingReadText;
    std::vector<PString> m_HistoryBuffers;

    size_t m_HistoryLocation = 0;
    size_t m_CursorPosition = 0;
    size_t m_HorizontalOffset = 0;

    std::vector<CompletionCandidate> m_PendingExpansionAlternatives;

    SubmissionMode m_SubmissionMode = SubmissionMode::AlwaysSubmit;
    DisplayMode m_DisplayMode = DisplayMode::Wrap;
    BackspaceAtStartAction m_BackspaceAtStartAction = BackspaceAtStartAction::Ignore;

    int m_BreakCharacter = 0x03;
    int m_DisconnectCharacter = 0x04;
    int m_CancelCharacter = DISABLED_CONTROL_CHARACTER;

    bool m_DisconnectOnEmpty = true;
    bool m_AutoRestart = true;
    bool m_EchoNewline = true;
    bool m_EchoControlCharacters = true;
    bool m_HistoryEnabled = true;
    bool m_InputActive = false;
    bool m_SkipNextLineFeed = false;
};
