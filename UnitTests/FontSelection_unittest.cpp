// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <utility>

#include <gtest/gtest.h>
#include <App/Application.h>
#include <ApplicationServer/Protocol.h>
#include <GUI/Font.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/Widgets/TextView.h>
#include <System/ExceptionHandling.h>

// These integration tests require the running appserver and bundled fonts.
class PFontSelectionTestApplication : public PApplication
{
public:
    explicit PFontSelectionTestApplication(std::function<void(PApplication&)> test)
        : PApplication("font_selection_test")
        , m_Test(std::move(test))
    {
        SetDeleteOnExit(false);
    }

    void* Run() override
    {
        CRITICAL_SCOPE(GetMutex());
        EXPECT_NO_THROW(m_Test(*this));
        Flush();
        return nullptr;
    }

private:
    std::function<void(PApplication&)> m_Test;
};

static void RunFontSelectionTest(std::function<void(PApplication&)> test)
{
    Ptr<PFontSelectionTestApplication> application = ptr_new<PFontSelectionTestApplication>(std::move(test));
    ASSERT_EQ(application->Start(PThreadDetachState_Joinable, 0, 16384), PErrorCode::Success);
    ASSERT_EQ(application->Join(nullptr), PErrorCode::Success);
}

static void ExpectFontSelectionHandle(PApplication& application, handle_id fontHandle, PErrorCode expectedError)
{
    PErrorCode error = PErrorCode::Success;
    try
    {
        application.MeasureFont(fontHandle, "A", 100.0f, false);
    }
    PERROR_CATCH([&error](PErrorCode result) { error = result; });
    EXPECT_EQ(error, expectedError);
}

TEST(FontSelection, TextViewsDefaultToOpaqueBlack)
{
    RunFontSelectionTest([](PApplication&)
    {
        Ptr<PTextView> label = ptr_new<PTextView>("label", "Test label");
        EXPECT_EQ(label->GetFgColor(), PColor(0, 0, 0));

        pugi::xml_document document;
        const pugi::xml_node xmlData = document.append_child("TextView");
        PViewFactoryContext context;
        Ptr<PTextView> xmlLabel = ptr_new<PTextView>(context, xmlData);
        EXPECT_EQ(xmlLabel->GetFgColor(), PColor(0, 0, 0));
    });
}

TEST(FontSelection, FontIDsSelectTrueTypePresets)
{
    RunFontSelectionTest([](PApplication&)
    {
        const PString text = "Hello \xc3\x85";
        struct Preset
        {
            PFontID FontID;
            float PointSize;
            float Height;
        };
        const Preset presets[] =
        {
            {PFontID::e_FontSmall, 14.0f, 23.0f},
            {PFontID::e_FontNormal, 14.0f, 23.0f},
            {PFontID::e_FontLarge, 19.0f, 30.0f},
            {PFontID::e_Font7Seg, 61.0f, 96.0f}
        };
        for (const Preset& preset : presets)
        {
            PFont font(preset.FontID);
            EXPECT_FLOAT_EQ(font.GetDescription().PointSize, preset.PointSize);
            EXPECT_EQ(font.GetDescription().Family, "DejaVu Sans");
            EXPECT_EQ(font.GetDescription().Style, "Book");
            const PFontHeight height = font.GetHeight();
            EXPECT_GT(height.ascender, 0.0f);
            EXPECT_FLOAT_EQ(height.ascender + height.descender, preset.Height);
            EXPECT_GT(font.GetStringWidth(text.data(), text.size()), 0.0f);
        }
    });
}

TEST(FontSelection, FontDeletesWaitForRenderQueue)
{
    Ptr<PApplication> application = ptr_new<PApplication>("font_queue_test");
    Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall, ptr_raw_pointer_cast(application));
    for (bool reselect : {true, false})
    {
        const handle_id handle = font->GetHandle();
        if (reselect) {
            font->Set(PFontID::e_FontLarge);
        } else {
            font.Reset();
        }
        ExpectFontSelectionHandle(*application, handle, PErrorCode::Success);
        application->Sync();
        ExpectFontSelectionHandle(*application, handle, PErrorCode::NOENT);
    }
}

TEST(FontSelection, AutomaticRenderQueueFlushProcessesFontDeletes)
{
    Ptr<PApplication> application = ptr_new<PApplication>("font_queue_test");
    Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall, ptr_raw_pointer_cast(application));
    const handle_id handle = font->GetHandle();
    font.Reset();
    for (size_t i = 0; i < PAPPSERVER_MSG_BUFFER_SIZE / sizeof(AppserverMessage); ++i) {
        application->DeleteFont(INVALID_HANDLE);
    }
    ExpectFontSelectionHandle(*application, handle, PErrorCode::NOENT);
    application->Sync();
}

TEST(FontSelection, FontHandlesAreGlobalAndApplicationOwned)
{
    Ptr<PApplication> firstApplication = ptr_new<PApplication>("first_font_test");
    Ptr<PApplication> secondApplication = ptr_new<PApplication>("second_font_test");
    PFont firstFont(PFontID::e_FontSmall, ptr_raw_pointer_cast(firstApplication));
    PFont secondFont(PFontID::e_FontLarge, ptr_raw_pointer_cast(secondApplication));
    EXPECT_NE(firstFont.GetHandle(), secondFont.GetHandle());
    EXPECT_LT(firstFont.GetStringWidth("Sample", 6), secondFont.GetStringWidth("Sample", 6));
    ExpectFontSelectionHandle(*firstApplication, secondFont.GetHandle(), PErrorCode::NOENT);
    ExpectFontSelectionHandle(*secondApplication, firstFont.GetHandle(), PErrorCode::NOENT);

    firstApplication->DeleteFont(secondFont.GetHandle());
    secondApplication->DeleteFont(firstFont.GetHandle());
    firstApplication->Sync();
    secondApplication->Sync();
    ExpectFontSelectionHandle(*firstApplication, firstFont.GetHandle(), PErrorCode::Success);
    ExpectFontSelectionHandle(*secondApplication, secondFont.GetHandle(), PErrorCode::Success);

    const handle_id oldHandle = firstFont.GetHandle();
    firstFont.Set(PFontID::e_FontNormal);
    EXPECT_EQ(firstFont.GetApplication(), firstApplication);
    firstApplication->Sync();
    ExpectFontSelectionHandle(*firstApplication, oldHandle, PErrorCode::NOENT);
    ExpectFontSelectionHandle(*secondApplication, oldHandle, PErrorCode::NOENT);
    ExpectFontSelectionHandle(*secondApplication, secondFont.GetHandle(), PErrorCode::Success);
}

TEST(FontSelection, FontsDoNotKeepTheirApplicationAliveOrRebindAfterShutdown)
{
    RunFontSelectionTest([](PApplication& currentApplication)
    {
        Ptr<PApplication> application = ptr_new<PApplication>("font_lifetime_test");
        PFont font(PFontID::e_FontSmall, ptr_raw_pointer_cast(application));
        const handle_id handle = font.GetHandle();
        application.Reset();
        EXPECT_EQ(font.GetApplication(), nullptr);
        EXPECT_EQ(PApplication::GetCurrentApplication(), &currentApplication);
        EXPECT_THROW(font.GetStringWidth("A", 1), std::system_error);
        EXPECT_THROW(font.Set(PFontID::e_FontLarge), std::system_error);
        EXPECT_EQ(font.GetHandle(), handle);
    });
}

TEST(FontSelection, ViewsRejectFontsFromOtherApplications)
{
    RunFontSelectionTest([](PApplication& application)
    {
        Ptr<PApplication> otherApplication = ptr_new<PApplication>("foreign_font_test");
        Ptr<PFont> otherFont = ptr_new<PFont>(PFontID::e_FontSmall, ptr_raw_pointer_cast(otherApplication));
        Ptr<PTextView> label = ptr_new<PTextView>("label", "Test label");
        const Ptr<const PFont> originalFont = label->GetFont();
        ASSERT_TRUE(application.AddHandler(label));
        PScopeExit removeHandler([&application, &label] { application.RemoveHandler(label); });
        EXPECT_THROW(label->SetFont(otherFont), std::system_error);
        EXPECT_EQ(label->GetFont(), originalFont);
    });
}

TEST(FontSelection, FailedReselectionPreservesTheFont)
{
    RunFontSelectionTest([](PApplication& application)
    {
        PFont font(PFontID::e_FontSmall);
        const handle_id handle = font.GetHandle();
        const float width = font.GetStringWidth("Sample", 6);
        PFontDescription invalidDescription = font.GetDescription();
        invalidDescription.PointSize = 0.0f;
        EXPECT_THROW(font.Set(invalidDescription), std::system_error);
        EXPECT_EQ(font.GetHandle(), handle);
        EXPECT_EQ(font.GetApplication(), &application);
        EXPECT_FLOAT_EQ(font.GetDescription().PointSize, 14.0f);
        EXPECT_FLOAT_EQ(font.GetStringWidth("Sample", 6), width);
    });
}

TEST(FontSelection, ReselectingUpdatesCachedWidgetSizesAndInternalEditors)
{
    RunFontSelectionTest([](PApplication&)
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
        label->SetFont(font);
        button->SetFont(font);
        textBox->SetFont(font);
        EXPECT_GT(label->GetPreferredSize(PPrefSizeType::Smallest).y, labelSize.y);
        EXPECT_GT(button->GetPreferredSize(PPrefSizeType::Smallest).y, buttonSize.y);
        EXPECT_GT(textBox->GetPreferredSize(PPrefSizeType::Smallest).y, editorSize.y);
    });
}

TEST(FontSelection, DescriptionsAndFontIDsUseTheExistingViewAPI)
{
    RunFontSelectionTest([](PApplication&)
    {
        Ptr<PFont> font = ptr_new<PFont>(PFontDescription());
        Ptr<PTextView> label = ptr_new<PTextView>("label", "AV \xc3\x85");
        label->SetFont(font);
        EXPECT_GT(label->GetFontHeight().ascender, 0.0f);
        EXPECT_FLOAT_EQ(label->GetStringWidth(label->GetText()), font->GetStringWidth("AV \xc3\x85", 5));
        const PFontHeight height = font->GetHeight();
        EXPECT_FLOAT_EQ(label->GetPreferredSize(PPrefSizeType::Smallest).y, height.ascender + height.descender);

        font->Set(PFontID::e_FontSmall);
        label->SetFont(font);
        EXPECT_GT(label->GetFontHeight().ascender, 0.0f);
        EXPECT_FLOAT_EQ(label->GetStringWidth(label->GetText()), font->GetStringWidth("AV \xc3\x85", 5));
        EXPECT_FLOAT_EQ(label->GetPreferredSize(PPrefSizeType::Smallest).y, 23.0f);
    });
}

TEST(FontSelection, LongMeasurementsPreserveKerningAcrossMessageBoundaries)
{
    RunFontSelectionTest([](PApplication&)
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
    });
}

TEST(FontSelection, UTF8CharactersStraddlingAMessageBoundaryRemainWhole)
{
    RunFontSelectionTest([](PApplication&)
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
    });
}
