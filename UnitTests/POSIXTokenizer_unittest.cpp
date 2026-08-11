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
