// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <pugixml.hpp>

#include <GUI/KeyboardView.h>
#include <Utils/XMLObjectParser.h>

TEST(KeyboardKeyNames, ResolvePhysicalKeysWithoutTreatingTextAsKeyCodes)
{
    PKeyCodes keyCode = PKeyCodes::NONE;
    ASSERT_TRUE(p_xml_object_parser::parse("A", keyCode));
    EXPECT_EQ(keyCode, PKeyCodes::A);
    EXPECT_EQ(std::to_underlying(keyCode), 0x04);
    ASSERT_TRUE(p_xml_object_parser::parse("LEFT_SHIFT", keyCode));
    EXPECT_EQ(keyCode, PKeyCodes::LEFT_SHIFT);
    EXPECT_EQ(std::to_underlying(keyCode), 0xe1);
    ASSERT_TRUE(p_xml_object_parser::parse("KEYPAD_ENTER", keyCode));
    EXPECT_EQ(keyCode, PKeyCodes::KEYPAD_ENTER);
    ASSERT_TRUE(p_xml_object_parser::parse("NUM_0", keyCode));
    EXPECT_EQ(keyCode, PKeyCodes::NUM_0);
    EXPECT_FALSE(p_xml_object_parser::parse("\xc3\x85", keyCode));
    EXPECT_FALSE(p_xml_object_parser::parse("SYMBOLS", keyCode));
    EXPECT_FALSE(p_xml_object_parser::parse("", keyCode));
}

TEST(KeyboardButtons, KeepUnicodeTextSeparateFromPhysicalKeys)
{
    pugi::xml_document document;
    ASSERT_TRUE(document.load_string("<Key key='A' normal='A' extra='+'/>"));
    const PKeyButton letter(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(letter.KeyCode, PKeyCodes::A);
    EXPECT_EQ(letter.NormalText, "A");
    EXPECT_EQ(letter.LowerText, "a");
    EXPECT_EQ(letter.ExtraText, "+");

    ASSERT_TRUE(document.load_string("<Key key='LEFT_BRACKET' normal='\xc3\x85' lower='\xc3\xa5' extra='\xc3\x97'/>"));
    const PKeyButton norwegian(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(norwegian.KeyCode, PKeyCodes::LEFT_BRACKET);
    EXPECT_EQ(norwegian.NormalText, "\xc3\x85");
    EXPECT_EQ(norwegian.LowerText, "\xc3\xa5");
    EXPECT_EQ(norwegian.ExtraText, "\xc3\x97");

    ASSERT_TRUE(document.load_string("<Key normal='A'/>"));
    const PKeyButton textOnly(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(textOnly.KeyCode, PKeyCodes::NONE);
    EXPECT_EQ(textOnly.NormalText, "A");
    EXPECT_EQ(textOnly.LowerText, "a");
    EXPECT_TRUE(textOnly.ExtraText.empty());
}

TEST(KeyboardButtons, PreserveControlsAndKeepSymbolsLocal)
{
    pugi::xml_document document;
    ASSERT_TRUE(document.load_string("<Key key='ENTER' normal='&#10;'/>"));
    const PKeyButton enter(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(enter.KeyCode, PKeyCodes::ENTER);
    EXPECT_EQ(enter.NormalText, "\n");
    EXPECT_EQ(enter.LowerText, "\n");

    ASSERT_TRUE(document.load_string("<Key key='SPACE' normal=' '/>"));
    const PKeyButton space(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(space.KeyCode, PKeyCodes::SPACE);
    EXPECT_EQ(space.NormalText, " ");
    EXPECT_EQ(space.LowerText, " ");

    ASSERT_TRUE(document.load_string("<Key key='BACKSPACE'/>"));
    const PKeyButton backspace(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(backspace.KeyCode, PKeyCodes::BACKSPACE);
    EXPECT_TRUE(backspace.NormalText.empty());
    EXPECT_TRUE(backspace.LowerText.empty());
    EXPECT_TRUE(backspace.ExtraText.empty());

    ASSERT_TRUE(document.load_string("<Key key='LEFT_SHIFT'/>"));
    const PKeyButton shift(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(shift.KeyCode, PKeyCodes::LEFT_SHIFT);
    EXPECT_TRUE(shift.NormalText.empty());

    ASSERT_TRUE(document.load_string("<Key key='LEFT_ALT'/>"));
    const PKeyButton symbols(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(symbols.KeyCode, PKeyCodes::LEFT_ALT);
    EXPECT_TRUE(symbols.NormalText.empty());
}

TEST(KeyboardButtons, AcceptLiteralStringsAndOptionalText)
{
    pugi::xml_document document;
    ASSERT_TRUE(document.load_string("<Key key='A' normal='ABC' lower='Abc' extra='\xc3\xa6\xc3\xb8\xc3\xa5'/>"));
    const PKeyButton text(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(text.KeyCode, PKeyCodes::A);
    EXPECT_EQ(text.NormalText, "ABC");
    EXPECT_EQ(text.LowerText, "Abc");
    EXPECT_EQ(text.ExtraText, "\xc3\xa6\xc3\xb8\xc3\xa5");

    ASSERT_TRUE(document.load_string("<Key normal='ENTER' lower=''/>"));
    const PKeyButton literal(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(literal.KeyCode, PKeyCodes::NONE);
    EXPECT_EQ(literal.NormalText, "ENTER");
    EXPECT_TRUE(literal.LowerText.empty());

    ASSERT_TRUE(document.load_string("<Key lower='a' extra='+'/>"));
    const PKeyButton optional(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(optional.KeyCode, PKeyCodes::NONE);
    EXPECT_TRUE(optional.NormalText.empty());
    EXPECT_EQ(optional.LowerText, "a");
    EXPECT_EQ(optional.ExtraText, "+");

    ASSERT_TRUE(document.load_string("<Key/>"));
    const PKeyButton empty(PPoint(), 1.0f, 1.0f, document.first_child());
    EXPECT_EQ(empty.KeyCode, PKeyCodes::NONE);
    EXPECT_TRUE(empty.NormalText.empty());
    EXPECT_TRUE(empty.LowerText.empty());
    EXPECT_TRUE(empty.ExtraText.empty());
}
