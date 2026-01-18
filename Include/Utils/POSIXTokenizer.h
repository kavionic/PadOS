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

#pragma once


class PPOSIXTokenizer
{
public:
    enum class QuoteMode { None, InSingle, InDouble };
    enum class Termination { Normal, OpenSingle, OpenDouble, TrailingSlash };

    struct Token
    {
        size_t      Start           = INVALID_INDEX;
        size_t      End             = INVALID_INDEX;
        bool        HasFormatting   = false;
    };

    PPOSIXTokenizer() = default;
    PPOSIXTokenizer(const PString& text) { SetText(text); }

    Termination SetText(const PString& text);

    Termination GetTermination() const { return m_Termination; }

    void    ParseToken(const Token& token, std::function<bool(size_t position, char character)>&& callback) const;
    PString GetTokenText(const Token& token) const;
    size_t  TokenToGlobalOffset(const Token& token, size_t tokenOffset) const;
    const std::vector<Token>& GetTokens() const { return m_Tokens; }

    size_t GetTokenByPosition(size_t position, size_t& outOffsetInToken) const;

    PPOSIXTokenizer& operator=(PPOSIXTokenizer&& rhs) = default;
private:
    PString             m_Text;
    std::vector<Token>  m_Tokens;
    Termination         m_Termination = Termination::Normal;
};