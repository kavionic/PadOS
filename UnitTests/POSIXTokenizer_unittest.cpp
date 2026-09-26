// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.08.2026 00:00

#include <gtest/gtest.h>

#include <Utils/POSIXTokenizer.h>

TEST(PPOSIXTokenizer, SplitsPipeOperators)
{
    const PPOSIXTokenizer tokenizer("ls -al|less");
    const std::vector<PPOSIXTokenizer::Token>& tokens = tokenizer.GetTokens();

    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokenizer.GetTokenText(tokens[0]), "ls");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[1]), "-al");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[2]), "|");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[3]), "less");
    EXPECT_FALSE(tokens[2].HasFormatting);
}

TEST(PPOSIXTokenizer, SplitsSpacedPipeOperators)
{
    const PPOSIXTokenizer tokenizer("ls -al | less");
    const std::vector<PPOSIXTokenizer::Token>& tokens = tokenizer.GetTokens();

    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokenizer.GetTokenText(tokens[0]), "ls");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[1]), "-al");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[2]), "|");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[3]), "less");
    EXPECT_FALSE(tokens[2].HasFormatting);
}

TEST(PPOSIXTokenizer, PreservesQuotedAndEscapedPipes)
{
    const PPOSIXTokenizer tokenizer("echo 'one|two' three\\|four \"five|six\"");
    const std::vector<PPOSIXTokenizer::Token>& tokens = tokenizer.GetTokens();

    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokenizer.GetTokenText(tokens[0]), "echo");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[1]), "one|two");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[2]), "three|four");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[3]), "five|six");
    EXPECT_TRUE(tokens[1].HasFormatting);
    EXPECT_TRUE(tokens[2].HasFormatting);
    EXPECT_TRUE(tokens[3].HasFormatting);
}

TEST(PPOSIXTokenizer, IdentifiesQuotedCharacters)
{
    const PPOSIXTokenizer tokenizer("plain 'single' \"double\" escaped\\*");
    const std::vector<PPOSIXTokenizer::Token>& tokens = tokenizer.GetTokens();

    ASSERT_EQ(tokens.size(), 4u);

    auto getQuotedCharacters = [&tokenizer](const PPOSIXTokenizer::Token& token)
        {
            std::vector<bool> quotedCharacters;
            tokenizer.ParseToken(token, [&quotedCharacters](size_t position, char character, bool isQuoted)
                {
                    quotedCharacters.push_back(isQuoted);
                    return true;
                }
            );
            return quotedCharacters;
        };

    EXPECT_EQ(getQuotedCharacters(tokens[0]), std::vector<bool>(5, false));
    EXPECT_EQ(getQuotedCharacters(tokens[1]), std::vector<bool>(6, true));
    EXPECT_EQ(getQuotedCharacters(tokens[2]), std::vector<bool>(6, true));
    EXPECT_EQ(
        getQuotedCharacters(tokens[3]),
        (std::vector<bool>{false, false, false, false, false, false, false, true}));
}

TEST(PPOSIXTokenizer, SplitsConsecutivePipeOperators)
{
    const PPOSIXTokenizer tokenizer("one||two");
    const std::vector<PPOSIXTokenizer::Token>& tokens = tokenizer.GetTokens();

    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokenizer.GetTokenText(tokens[0]), "one");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[1]), "|");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[2]), "|");
    EXPECT_EQ(tokenizer.GetTokenText(tokens[3]), "two");
}
