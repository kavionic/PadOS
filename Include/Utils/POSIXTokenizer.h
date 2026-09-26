// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2026 23:00

#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include <sys/types.h>
#include <vector>

#include <Utils/String.h>

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
    const PString& GetText() const noexcept { return m_Text; }

    Termination GetTermination() const { return m_Termination; }

    void    ParseToken(const Token& token, std::function<bool(size_t position, char character, bool isQuoted)>&& callback) const;
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
