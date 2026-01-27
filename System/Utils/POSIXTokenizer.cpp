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
// Created: 14.01.2026 23:00


#include <Utils/POSIXTokenizer.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPOSIXTokenizer::Termination PPOSIXTokenizer::SetText(const PString& text)
{
    m_Text = text;
    m_Tokens.clear();

    QuoteMode                quouteMode = QuoteMode::None;

    Token currentToken;
    currentToken.Start = 0;

    auto push_token = [this, &currentToken](size_t newStart)
        {
            currentToken.End = newStart;

            const ssize_t length = currentToken.End - currentToken.Start;

            if (currentToken.HasFormatting || length > 0)
            {
                m_Tokens.push_back(currentToken);
            }
            currentToken.Start = currentToken.End + 1;
        };

    auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\n'; };

    for (size_t i = 0; i < text.size(); ++i)
    {
        const char character = text[i];

        switch (quouteMode)
        {
            case QuoteMode::None:
                if (is_space(character))
                {
                    push_token(i);
                }
                else if (character == '\'')
                {
                    quouteMode = QuoteMode::InSingle;
                    currentToken.HasFormatting = true;
                }
                else if (character == '"')
                {
                    quouteMode = QuoteMode::InDouble;
                    currentToken.HasFormatting = true;
                }
                else if (character == '\\')
                {
                    currentToken.HasFormatting = true;
                    if (i + 1 >= text.size())
                    {
                        push_token(text.size());
                        m_Termination = Termination::TrailingSlash;
                        return m_Termination;
                    }
                    ++i; // Skip \ and next character.
                }
                break;

            case QuoteMode::InSingle:
                if (character == '\'') {
                    quouteMode = QuoteMode::None;
                }
                break;

            case QuoteMode::InDouble:
                if (character == '"')
                {
                    quouteMode = QuoteMode::None;
                }
                else if (character == '\\')
                {
                    ++i;
                    if (i >= text.size())
                    {
                        push_token(text.size());
                        m_Termination = Termination::TrailingSlash;
                        return m_Termination;
                    }
                }
                break;
        }
    }

    push_token(text.size());

    m_Termination = Termination::Normal;

    if (quouteMode == QuoteMode::InSingle) {
        m_Termination = Termination::OpenSingle;
    } else if (quouteMode == QuoteMode::InDouble) {
        m_Termination = Termination::OpenDouble;
    }

    return m_Termination;
}


void PPOSIXTokenizer::ParseToken(const Token& token, std::function<bool(size_t position, char character)>&& callback) const
{
    QuoteMode        quouteMode = QuoteMode::None;
    std::string_view text(&m_Text[token.Start], &m_Text[token.End]);

    for (size_t i = 0; i < text.size(); ++i)
    {
        char character = text[i];

        switch (quouteMode)
        {
            case QuoteMode::None:
                if (character == '\'')
                {
                    quouteMode = QuoteMode::InSingle;
                }
                else if (character == '"')
                {
                    quouteMode = QuoteMode::InDouble;
                }
                else if (character == '\\')
                {
                    if (i + 1 >= text.size()) {
                        return; // Trailing backslash.
                    }
                    const char nextChar = text[i + 1];
                    if (nextChar == '\n')
                    {
                        ++i; // Remove both \ and newline (line continuation).
                    }
                    else
                    {
                        ++i;
                        if (!callback(i, nextChar)) return;
                    }
                }
                else
                {
                    if (!callback(i, character)) return;
                }
                break;

            case QuoteMode::InSingle:
                if (character == '\'') {
                    quouteMode = QuoteMode::None;
                } else {
                    if (!callback(i, character)) return;
                }
                break;

            case QuoteMode::InDouble:
                if (character == '"')
                {
                    quouteMode = QuoteMode::None;
                }
                else if (character == '\\')
                {
                    if (i + 1 >= text.size()) {
                        return; // Trailing backslash.
                    }
                    const char nextChar = text[i + 1];
                    // Backslash in double-quote only special before \, ", $, `, or newline.
                    if (nextChar == '\\' || nextChar == '"' || nextChar == '$' || nextChar == '`') {
                        if (!callback(++i, nextChar)) return;
                    } else if (nextChar == '\n') {
                        ++i; // Remove both \ and newline (line continuation).
                    } else {
                        if (!callback(i, character)) return; // Backslash preserved literally.
                    }
                }
                else
                {
                    if (!callback(i, character)) return;
                }
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PPOSIXTokenizer::GetTokenText(const Token& token) const
{
    if (!token.HasFormatting)
    {
        return PString(&m_Text[token.Start], &m_Text[token.End]);
    }
    else
    {
        PString tokenText;

        ParseToken(token, [&tokenText](size_t position, char character) { tokenText.push_back(character); return true; });

        return tokenText;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PPOSIXTokenizer::TokenToGlobalOffset(const Token& token, size_t tokenOffset) const
{
    if (tokenOffset == token.End - token.Start) {
        return token.End;
    }
    size_t globalPos = token.End;
    size_t tokenPos = 0;
    ParseToken(token, [tokenOffset, &token, &globalPos, &tokenPos](size_t charPos, char character) noexcept
        {
            if (tokenPos == tokenOffset)
            {
                globalPos = token.Start + charPos;
                return false;
            }
            tokenPos++;
            return true;
        }
    );
    return globalPos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PPOSIXTokenizer::GetTokenByPosition(size_t position, size_t& outOffsetInToken) const
{
    auto i = std::lower_bound(m_Tokens.begin(), m_Tokens.end(), position, [](const Token& lhs, size_t rhs) { return lhs.Start < rhs; });
    if (i != m_Tokens.begin())
    {
        --i;
        if (i != m_Tokens.end() && position <= i->End)
        {
            const Token& token = *i;


            size_t tokenPos = 0;
            ParseToken(token, [position, &token, &tokenPos](size_t charPos, char character) noexcept
                {
                    if (token.Start + charPos >= position) {
                        return false;
                    }
                    tokenPos++;
                    return true;
                }
            );
            outOffsetInToken = tokenPos;

            return i - m_Tokens.begin();
        }
    }
    return INVALID_INDEX;
}
