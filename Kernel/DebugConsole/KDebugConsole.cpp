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
#include <Process/Process.h>
#include <Utils/POSIXTokenizer.h>
#include <Utils/Logging.h>
#include <System/AppDefinition.h>
#include <Kernel/KLogging.h>
#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/VFS/FileIO.h>


namespace kernel
{
std::map<PString, std::function<Ptr<KConsoleCommand>(KDebugConsole* console)>> KDebugConsole::s_Commands;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDebugConsole::KDebugConsole(int stdInFD, int stdOutFD, int stdErrFD)
    : KThread("debug_console")
    , m_StdInFD(stdInFD)
    , m_StdOutFD(stdOutFD)
    , m_StdErrFD(stdErrFD)
{
}


KDebugConsole::KDebugConsole(const PString& portPath)
    : KThread("debug_console")
    , m_PortPath(portPath)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::Setup()
{
    Start_trw(KSpawnThreadFlag::SpawnProcess);
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

            if (m_StdInFD == -1)
            {
                try
                {
                    const int stream = kopen_trw(m_PortPath.c_str(), O_RDWR | O_DIRECT);

                    if (stream != STDIN_FILENO) {
                        dup2(stream, STDIN_FILENO);
                    }
                    if (stream != STDOUT_FILENO) {
                        dup2(stream, STDOUT_FILENO);
                    }
                    if (stream != STDERR_FILENO) {
                        dup2(stream, STDERR_FILENO);
                    }
                    m_StdInFD  = STDIN_FILENO;
                    m_StdOutFD = STDOUT_FILENO;
                    m_StdErrFD = STDERR_FILENO;
                    if (stream > 2) {
                        kclose(stream);
                    }
                }
                catch (std::exception& exc)
                {
                    snooze_ms(100);
                    continue;
                }
            }
            size_t length;
            try
            {
                length = kread_trw(m_StdInFD, buffer, sizeof(buffer));
            }
            catch(std::exception& exc)
            {
                if (!m_PortPath.empty())
                {
                    kclose(m_StdInFD);
                    kclose(m_StdOutFD);
                    kclose(m_StdErrFD);
                    m_StdInFD = m_StdOutFD = m_StdErrFD = -1;
                }
                continue;
            }
            const TimeValNanos curTime = get_monotonic_time();

            if (curTime >= nextSizeQueryTime)
            {
                nextSizeQueryTime = curTime + TimeValNanos::FromMilliseconds(250);
                SendANSICode(PANSI_ControlCode::XTerm_XTWINOPS, 18);
            }

            size_t start = 0;
            for (size_t i = 0; i <= length; ++i)
            {
                if (i != length)
                {
                    const PANSI_ControlCode controlChar = m_ANSICodeParser.ProcessCharacter(buffer[i]);
                    if (controlChar != PANSI_ControlCode::None)
                    {
                        start = i + 1;
                        if (controlChar != PANSI_ControlCode::Pending) {
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
            snooze_ms(100);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::AddInputText(const char* text, size_t length)
{
    m_PendingExpansionAlternatives.clear();

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
    write(m_StdOutFD, "\n", 1);

    m_PendingExpansionAlternatives.clear();

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
    UpdatePrompt();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::SendText(const char* text, size_t length)
{
    write(m_StdOutFD, text, length);
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

void KDebugConsole::MoveCursor(int distance)
{
    m_PendingExpansionAlternatives.clear();

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
    m_PendingExpansionAlternatives.clear();

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

void KDebugConsole::BackspaceChar()
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

void KDebugConsole::RefreshText(size_t startPosition)
{
    const size_t length = m_EditBuffer.size() - startPosition;

    SendText(&m_EditBuffer[startPosition], length);

    const PIPoint postScreenPos = GetScreenPosition(m_CursorPosition);
    const PIPoint preScreenPos  = GetScreenPosition(m_EditBuffer.size());

    if (preScreenPos.x == 0) {
        SendText(" \010", 2); // Punch through "pending wrap".
    }
    SendANSICode(PANSI_ControlCode::EraseDisplay);

    MoveScreenCursor(preScreenPos, postScreenPos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::ResetInput()
{
    m_PendingExpansionAlternatives.clear();

    const PString& lineBuffer = m_InputBuffer.empty() ? m_EditBuffer : (m_InputBuffer + m_EditBuffer);

    if (!lineBuffer.empty()) {
        m_HistoryBuffers.push_back(lineBuffer);
    }

    m_InputBuffer.clear();
    m_EditBuffer.clear();
    m_CursorPosition = 0;
    m_HistoryLocation = m_HistoryBuffers.size();
    
    m_Prompt = m_CmdPrompt;

    SendNewline();
    SendText(m_Prompt);
    SendANSICode(PANSI_ControlCode::EraseDisplay);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KDebugConsole::SetPrompt(const PString& text)
{
    if (text != m_Prompt)
    {
        const size_t prevCursorPos = m_CursorPosition;
        MoveCursor(-m_CursorPosition);

        m_Prompt = text;
        m_CursorPosition = prevCursorPos;

        SendNewline();
        SendText(m_Prompt);
        RefreshText(0);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::UpdatePrompt()
{
    char cwd[PATH_MAX];

    const PString setColor = PANSIEscapeCodeParser::FormatANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::FgColor_Yellow));
    const PString resetColor = PANSIEscapeCodeParser::FormatANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::Reset));

    const char* envPath = getenv("PATH");

    bool pathOK = false;

    if (envPath != nullptr && strlen(envPath) <= PATH_MAX)
    {
        strcpy(cwd, envPath);

        stat_t pathStat;
        stat_t cwdStat;
        pathOK = stat(envPath, &pathStat) == 0 && stat(".", &cwdStat) == 0 && pathStat.st_dev == cwdStat.st_dev && pathStat.st_ino == cwdStat.st_ino;
    }
    if (!pathOK) {
        pathOK = getcwd(cwd, sizeof(cwd)) != nullptr;
    }
    if (!pathOK || !SetPrompt(PString::format_string("[{}{}{}]$ ", setColor, cwd, resetColor))) {
        SendText(m_Prompt);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KDebugConsole::GetCommonStartLength(const std::vector<PString>& alternatives)
{
    if (alternatives.empty()) {
        return 0;
    } else if (alternatives.size() == 1) {
        return alternatives[0].size();
    }
    for (size_t commonStartLength = 0; ; ++commonStartLength)
    {
        for (size_t i = 0; i < alternatives.size(); ++i)
        {
            const PString& text = alternatives[i];
            const size_t   length = (alternatives[i].back() == '\\') ? (alternatives[i].size() - 1) : alternatives[i].size();

            if (length <= commonStartLength)
            {
                return commonStartLength;
            }
            if (i != 0)
            {
                if (text[commonStartLength] != alternatives[0][commonStartLength])
                {
                    return commonStartLength;
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::PrintPendingExpansionAlternatives()
{
    std::vector<PString> alternatives = std::move(m_PendingExpansionAlternatives);

    const size_t cursorPosition = m_CursorPosition;

    MoveCursor(m_EditBuffer.size() - m_CursorPosition);
    SendNewline();

    SendANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::FgColor_BrightGreen));

    for (size_t i = 0; i < alternatives.size(); ++i)
    {
        if (i != 0) SendText(" ", 1);

        const PString& text = alternatives[i];
        const size_t   length = (alternatives[i].back() == '\\') ? (alternatives[i].size() - 1) : alternatives[i].size();

        SendText(text.c_str(), length);
    }

    SendANSICode(PANSI_ControlCode::SetRenderProperty, int(PANSI_RenderProperty::Reset));

    SendNewline();
    SendText(m_Prompt);
    RefreshText(0);
    MoveCursor(cursorPosition);

    m_PendingExpansionAlternatives = std::move(alternatives);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDebugConsole::ExpandArgument()
{
    if (!m_PendingExpansionAlternatives.empty())
    {
        PrintPendingExpansionAlternatives();
        return;
    }
    const PString& lineBuffer = m_InputBuffer.empty() ? m_EditBuffer : (m_InputBuffer + m_EditBuffer);

    std::vector<PString> alternatives;

    size_t replaceStart;
    size_t replaceEnd;

    if (lineBuffer.empty())
    {
        alternatives = ExpandCommandName(0, PString::zero, 0, replaceStart, replaceEnd);
    }
    else
    {
        PPOSIXTokenizer tokenizer(lineBuffer);

        size_t tokenPosition = 0;
        const size_t tokenIndex = tokenizer.GetTokenByPosition(m_CursorPosition, tokenPosition);

        if (tokenIndex != INVALID_INDEX)
        {
            const PPOSIXTokenizer::Token& token = tokenizer.GetTokens()[tokenIndex];

            PString text = tokenizer.GetTokenText(token);

            if (tokenIndex == 0) {
                alternatives = ExpandCommandName(tokenIndex, text, tokenPosition, replaceStart, replaceEnd);
            } else {
                alternatives = ExpandFilePath(tokenIndex, text, tokenPosition, replaceStart, replaceEnd);
            }
            replaceStart = tokenizer.TokenToGlobalOffset(token, replaceStart);
            replaceEnd   = tokenizer.TokenToGlobalOffset(token, replaceEnd);
        }
    }

    if (alternatives.empty()) {
        return;
    }

    const size_t replacementLength = replaceEnd - replaceStart;
    PString replacementString;

    bool finalExpansion = true;

    if (alternatives.size() == 1)
    {
        replacementString = alternatives[0];
        if (replacementString.back() == '\\')
        {
            finalExpansion = false;
            replacementString.resize(replacementString.size() - 1);
        }
    }
    else
    {
        finalExpansion = false;
        const size_t commonStartLength = GetCommonStartLength(alternatives);
        if (commonStartLength > replacementLength ||
            (commonStartLength == replacementLength &&
             std::string_view(m_EditBuffer.c_str() + replaceStart, commonStartLength) != std::string_view(alternatives[0].c_str(), commonStartLength)))
        {
            replacementString = PString(alternatives[0].data(), commonStartLength);
        }
    }

    if (!replacementString.empty())
    {
        assert(replaceStart != INVALID_INDEX);
        assert(replaceEnd != INVALID_INDEX);

        for (size_t i = 0; i < replacementString.size(); ++i)
        {
            if (replacementString[i] == ' ')
            {
                replacementString.insert(replacementString.begin() + i, '\\');
                ++i;
            }
        }

        MoveCursor(replaceStart - m_CursorPosition);

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
        m_PendingExpansionAlternatives = std::move(alternatives);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<PString> KDebugConsole::ExpandCommandName(size_t argumentIndex, const PString& argumentText, size_t argumentOffset, size_t& outReplaceStart, size_t& outReplaceEnd)
{
    std::vector<PString> alternatives;

    for (const auto& i : s_Commands)
    {
        if (i.first.starts_with_nocase(argumentText.c_str()))
        {
            alternatives.push_back(i.first);
        }
    }

    const std::vector<const PAppDefinition*> apps = PAppDefinition::GetApplicationList();

    for (const PAppDefinition* app : apps)
    {
        PString entryName(app->Name);
        if (entryName.starts_with_nocase(argumentText.c_str()))
        {
            alternatives.push_back(std::move(entryName));
        }
    }
    outReplaceStart = 0;
    outReplaceEnd   = argumentText.size();

    std::sort(alternatives.begin(), alternatives.end());

    return alternatives;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<PString> KDebugConsole::ExpandFilePath(size_t argumentIndex, const PString& argumentText, size_t argumentOffset, size_t& outReplaceStart, size_t& outReplaceEnd)
{
    PString folderPath;
    PString filename;

    for (ssize_t i = argumentOffset - 1; i >= 0; --i)
    {
        if (argumentText[i] == '/')
        {
            outReplaceStart = i + 1;
            outReplaceEnd = argumentText.size();
            folderPath.insert(folderPath.begin(), &argumentText[0], &argumentText[i+1]);
            filename.insert(filename.begin(), &argumentText[i+1], &argumentText[argumentText.size()]);

            folderPath += ".";
            break;
        }
    }
    if (folderPath.empty())
    {
        folderPath = ".";
        filename = argumentText;

        outReplaceStart = 0;
        outReplaceEnd = argumentText.size();
    }

    std::vector<PString> alternatives;
    if (!folderPath.empty())
    {
        int directory = open(folderPath.c_str(), O_RDONLY | O_DIRECTORY);
        if (directory != -1)
        {
            dirent_t entry;

            while(kread_directory(directory, &entry, sizeof(entry)) == sizeof(entry))
            {
                PString entryName(entry.d_name, entry.d_namlen);
                if (entryName != "." && entryName != ".." && entryName.starts_with_nocase(filename.c_str()))
                {
                    if (entry.d_type == DT_DIR)
                    {
                        entryName += "/\\";
                    }
                    else if (entry.d_type == DT_LNK)
                    {
                        struct stat targetStat;
                        if (fstatat(directory, entry.d_name, &targetStat, 0) == 0 && S_ISDIR(targetStat.st_mode)) {
                            entryName += "/\\";
                        }
                    }
                    alternatives.push_back(std::move(entryName));
                }
            }
            close(directory);
        }
    }
    std::sort(alternatives.begin(), alternatives.end());
    return alternatives;
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
        const PString path = (tokens[0].empty() || tokens[0][0] == '/') ? tokens[0] : (PString("/bin/") + tokens[0]);

        stat_t statBuf;
        if (stat(path.c_str(), &statBuf) == 0 && (statBuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        {
            std::vector<char*> argv;
            argv.reserve(tokens.size());
            for (auto& token : tokens) {
                argv.push_back(token.data());
            }
            argv.push_back(nullptr);

            pid_t pid;
            PErrorCode result = spawn_execve(&pid, path.c_str(), 0, argv.data(), environ);
            if (result != PErrorCode::Success) {
                kprintf("Failed to execute '%s': %s\n", path.c_str(), p_strerror(result));
            }
            void* retValue = nullptr;
            thread_join(pid, &retValue);
            return;
        }

        auto cmdIt = s_Commands.find(tokens[0]);

        if (cmdIt != s_Commands.end())
        {
            const Ptr<KConsoleCommand>& cmd = cmdIt->second(this);

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

void KDebugConsole::ProcessControlChar(PANSI_ControlCode controlChar, const std::vector<int>& args)
{
    switch(controlChar)
    {
        case PANSI_ControlCode::Break:
            SendText("^C", 2);
            ResetInput();
            break;
        case PANSI_ControlCode::Disconnect:
            SendText("^D", 2);
            ResetInput();
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
        case PANSI_ControlCode::XTerm_XTWINOPS:
            if (args.size() >= 3 && args[0] == 8) {
                m_TerminalSize = PIPoint(args[2], args[1]);
            }
            break;
        default:
            break;
    }
}


} // namespace kernel
