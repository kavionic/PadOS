// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <ApplicationServer/FontProtocol.h>
#include <GUI/Font.h>
#include <GUI/FontMetrics.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/Widgets/TextView.h>

// These integration tests require the running appserver and bundled fonts.
TEST(FontSelection, TextViewsDefaultToOpaqueBlack)
{
    Ptr<PTextView> label = ptr_new<PTextView>("label", "Test label");
    EXPECT_EQ(label->GetFgColor(), PColor(0, 0, 0));

    pugi::xml_document document;
    const pugi::xml_node xmlData = document.append_child("TextView");
    PViewFactoryContext context;
    Ptr<PTextView> xmlLabel = ptr_new<PTextView>(context, nullptr, xmlData);
    EXPECT_EQ(xmlLabel->GetFgColor(), PColor(0, 0, 0));
}

TEST(FontSelection, FontIDsSelectTrueTypeFontsWithMatchingHeights)
{
    const PString text = "Hello \xc3\x85";
    const PFontID fontIDs[] = {PFontID::e_FontSmall, PFontID::e_FontNormal, PFontID::e_FontLarge, PFontID::e_Font7Seg};
    for (PFontID fontID : fontIDs)
    {
        PFont font(fontID);
        ASSERT_TRUE(font.IsFreeType());
        EXPECT_EQ(font.Get(), PFontID::e_FontCount);
        EXPECT_EQ(font.GetDescription().Family, "DejaVu Sans");
        EXPECT_EQ(font.GetDescription().Style, "Book");
        const PFontHeight height = font.GetHeight();
        EXPECT_GT(height.ascender, 0.0f);
        EXPECT_FLOAT_EQ(height.ascender + height.descender, PFontMetrics::GetFontHeight(fontID));
        EXPECT_GT(font.GetStringWidth(text.data(), text.size()), 0.0f);
    }
}

TEST(FontSelection, ReselectingUpdatesCachedWidgetSizesAndInternalEditors)
{
    Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
    Ptr<PTextView> label = ptr_new<PTextView>("label", "Test label");
    Ptr<PButton> button = ptr_new<PButton>("button", "Test label");
    Ptr<PTextBox> textBox = ptr_new<PTextBox>("editor", "Test label");
    label->SetFont(font);
    button->SetFont(font);
    textBox->SetFont(font);
    const PPoint labelSize = label->GetPreferredSize(PPrefSizeType::Smallest);
    const PPoint buttonSize = button->GetPreferredSize(PPrefSizeType::Smallest);
    const PPoint editorSize = textBox->GetPreferredSize(PPrefSizeType::Smallest);
    ASSERT_NE(textBox->GetChildAt(size_t(0)), nullptr);
    EXPECT_EQ(textBox->GetChildAt(size_t(0))->GetFont(), font);

    font->Set(PFontID::e_FontLarge);
    ASSERT_TRUE(font->IsFreeType());
    label->SetFont(font);
    button->SetFont(font);
    textBox->SetFont(font);
    EXPECT_GT(label->GetPreferredSize(PPrefSizeType::Smallest).y, labelSize.y);
    EXPECT_GT(button->GetPreferredSize(PPrefSizeType::Smallest).y, buttonSize.y);
    EXPECT_GT(textBox->GetPreferredSize(PPrefSizeType::Smallest).y, editorSize.y);
}

TEST(FontSelection, DescriptionsAndFontIDsUseTheExistingViewAPI)
{
    Ptr<PFont> font = ptr_new<PFont>(PFontDescription());
    Ptr<PTextView> label = ptr_new<PTextView>("label", "AV \xc3\x85");
    label->SetFont(font);
    ASSERT_TRUE(label->GetFont()->IsFreeType());
    EXPECT_GT(label->GetFontHeight().ascender, 0.0f);
    EXPECT_FLOAT_EQ(label->GetStringWidth(label->GetText()), font->GetStringWidth("AV \xc3\x85", 5));
    const PFontHeight height = font->GetHeight();
    EXPECT_FLOAT_EQ(label->GetPreferredSize(PPrefSizeType::Smallest).y, height.ascender + height.descender);

    font->Set(PFontID::e_FontSmall);
    label->SetFont(font);
    ASSERT_TRUE(label->GetFont()->IsFreeType());
    EXPECT_GT(label->GetFontHeight().ascender, 0.0f);
    EXPECT_FLOAT_EQ(label->GetStringWidth(label->GetText()), font->GetStringWidth("AV \xc3\x85", 5));
    EXPECT_FLOAT_EQ(label->GetPreferredSize(PPrefSizeType::Smallest).y, PFontMetrics::GetFontHeight(PFontID::e_FontSmall));
}

TEST(FontSelection, LongMeasurementsPreserveKerningAcrossMessageBoundaries)
{
    PFont font{PFontDescription()};
    const float widthAV = font.GetStringWidth("AV", 2);
    const float widthVA = font.GetStringWidth("VA", 2);
    const float widthA = font.GetStringWidth("A", 1);
    const float widthV = font.GetStringWidth("V", 1);
    ASSERT_LT(widthAV, widthA + widthV);
    const size_t repetitions = FONT_MEASUREMENT_CHUNK_BYTES + 1;
    PString text;
    text.reserve(repetitions * 2);
    for (size_t i = 0; i < repetitions; ++i) {
        text += "AV";
    }
    const float expectedWidth = float(repetitions) * widthAV + float(repetitions - 1) * (widthVA - widthV - widthA);
    EXPECT_NEAR(font.GetStringWidth(text.data(), text.size()), expectedWidth, 0.01f);

    const size_t prefixLength = FONT_MEASUREMENT_CHUNK_BYTES + 2;
    const float prefixWidth = font.GetStringWidth(text.data(), prefixLength);
    EXPECT_EQ(font.GetStringLength(text, prefixWidth), int(prefixLength));
    EXPECT_EQ(font.GetStringLength(text, prefixWidth - 0.5f), int(prefixLength - 1));
    EXPECT_EQ(font.GetStringLength(text, prefixWidth - 0.5f, true), int(prefixLength));
}

TEST(FontSelection, UTF8CharactersStraddlingAMessageBoundaryRemainWhole)
{
    PFont font{PFontDescription()};
    const PString character = "\xe2\x82\xac";
    const float characterWidth = font.GetStringWidth(character.data(), character.size());
    const size_t repetitions = FONT_MEASUREMENT_CHUNK_BYTES;
    PString text;
    for (size_t i = 0; i < repetitions; ++i) {
        text += character;
    }
    EXPECT_NEAR(font.GetStringWidth(text.data(), text.size()), float(repetitions) * characterWidth, 0.01f);
    const size_t prefixCharacters = FONT_MEASUREMENT_CHUNK_BYTES / character.size() + 2;
    const float prefixWidth = float(prefixCharacters) * characterWidth;
    EXPECT_EQ(font.GetStringLength(text, prefixWidth), int(prefixCharacters * character.size()));
    EXPECT_EQ(font.GetStringLength(text, prefixWidth - 0.5f), int((prefixCharacters - 1) * character.size()));
    EXPECT_EQ(font.GetStringLength(text, prefixWidth - 0.5f, true), int(prefixCharacters * character.size()));
}
