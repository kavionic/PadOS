// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <App/Application.h>
#include <ApplicationServer/Protocol.h>
#include <GUI/Font.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/Slider.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/Widgets/TextView.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

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
    RunGUITest([](PApplication&)
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
    RunGUITest([](PApplication&)
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
    RunGUITest([](PApplication& currentApplication)
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
    RunGUITest([](PApplication& application)
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
    RunGUITest([](PApplication& application)
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
    RunGUITest([](PApplication& application)
    {
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
        Ptr<PTextView> label = ptr_new<PTextView>("label", "Test label");
        Ptr<PButton> button = ptr_new<PButton>("button", "Test label");
        Ptr<PSlider> slider = ptr_new<PSlider>("slider");
        Ptr<PTextBox> textBox = ptr_new<PTextBox>("editor", "Test label");
        ASSERT_TRUE(application.AddHandler(button));
        PScopeExit removeHandler([&application, &button] { application.RemoveHandler(button); });
        ASSERT_NE(button->GetStyle(), nullptr);
        ASSERT_TRUE(application.AddHandler(slider));
        PScopeExit removeSlider([&application, &slider] { application.RemoveHandler(slider); });
        ASSERT_NE(slider->GetStyle(), nullptr);
        slider->SetLabel("Value: ");
        slider->SetValueStringFormat("{:.1f}");
        slider->SetLimitLabels("Minimum", "Maximum");

        label->SetFont(font);
        button->SetFont(font);
        slider->SetFont(font);
        textBox->SetFont(font);
        const PPoint labelSize = label->GetPreferredSize(PPrefSizeType::Smallest);
        const PPoint buttonSize = button->GetPreferredSize(PPrefSizeType::Smallest);
        const PPoint sliderSize = slider->GetPreferredSize(PPrefSizeType::Smallest);
        const PPoint editorSize = textBox->GetPreferredSize(PPrefSizeType::Smallest);
        ASSERT_GT(buttonSize.y, 0.0f);
        ASSERT_GT(sliderSize.y, 0.0f);
        ASSERT_NE(textBox->GetChildAt(size_t(0)), nullptr);
        EXPECT_EQ(textBox->GetChildAt(size_t(0))->GetFont(), font);

        font->Set(PFontID::e_FontLarge);
        label->SetFont(font);
        button->SetFont(font);
        slider->SetFont(font);
        textBox->SetFont(font);
        EXPECT_GT(label->GetPreferredSize(PPrefSizeType::Smallest).y, labelSize.y);
        EXPECT_GT(button->GetPreferredSize(PPrefSizeType::Smallest).y, buttonSize.y);
        EXPECT_GT(slider->GetPreferredSize(PPrefSizeType::Smallest).y, sliderSize.y);
        EXPECT_GT(textBox->GetPreferredSize(PPrefSizeType::Smallest).y, editorSize.y);
    });
}

TEST(FontSelection, DescriptionsAndFontIDsUseTheExistingViewAPI)
{
    RunGUITest([](PApplication&)
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
    RunGUITest([](PApplication&)
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
    RunGUITest([](PApplication&)
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

TEST(FontSelection, SliderGeometryUsesTheStyleAndHandlesMissingStyle)
{
    RunGUITest([](PApplication& application)
    {
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            Ptr<PSlider> slider = ptr_new<PSlider>("slider", PSliderFlags::TicksBelow, 10, orientation);
            slider->SetMinMax(10.0f, 30.0f);
            slider->SetValue(20.0f, false);
            slider->SetFrame(PRect(0.0f, 0.0f, 200.0f, 200.0f));
            EXPECT_EQ(slider->GetStyle(), nullptr);
            EXPECT_EQ(slider->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
            EXPECT_FLOAT_EQ(slider->PosToVal(PPoint(50.0f, 50.0f)), 20.0f);
            EXPECT_EQ(slider->ValToPos(20.0f), PPoint());

            ASSERT_TRUE(application.AddHandler(slider));
            PScopeExit removeSlider([&application, &slider] { application.RemoveHandler(slider); });
            slider->OnStyleChanged();
            ASSERT_NE(slider->GetStyle(), nullptr);
            EXPECT_GT(slider->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
            EXPECT_GT(slider->GetPreferredSize(PPrefSizeType::Smallest).y, 0.0f);
            const PPoint minimumPosition = slider->ValToPos(10.0f);
            const PPoint maximumPosition = slider->ValToPos(30.0f);
            EXPECT_FLOAT_EQ(slider->PosToVal(minimumPosition), 10.0f);
            EXPECT_FLOAT_EQ(slider->PosToVal(maximumPosition), 30.0f);
            EXPECT_FLOAT_EQ(slider->PosToVal((minimumPosition + maximumPosition) * 0.5f), 20.0f);
            if (orientation == POrientation::Horizontal) {
                EXPECT_LT(minimumPosition.x, maximumPosition.x);
            } else {
                EXPECT_GT(minimumPosition.y, maximumPosition.y);
            }
            slider->SetStepCount(5);
            EXPECT_FLOAT_EQ(slider->PosToVal(minimumPosition * 0.6f + maximumPosition * 0.4f), 20.0f);
            slider->SetResolution(2.0f);
            slider->SetValue(17.2f, false);
            EXPECT_FLOAT_EQ(slider->GetValue(), 18.0f);

            slider->SetStepCount(0);
            slider->SetFrame(PRect());
            EXPECT_FLOAT_EQ(slider->PosToVal(PPoint(100.0f, 100.0f)), 18.0f);
            slider->SetMinMax(10.0f, 10.0f);
            slider->SetValue(10.0f, false);
            EXPECT_FLOAT_EQ(slider->PosToVal(slider->ValToPos(10.0f)), 10.0f);
        }
    });
}

TEST(FontSelection, SliderPropertyChangesRefreshPrivateMeasurements)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("slider_style_target", PViewFlags::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
        target->SetFont(font);
        target->SetFrame(PRect(0.0f, 0.0f, 200.0f, 200.0f));
        const PGUIStyle* style = target->GetStyle();
        ASSERT_NE(style, nullptr);

        PSliderStyleData data;
        data.Orientation = POrientation::Vertical;
        data.MinLabel = "i";
        data.MaxLabel = "i";
        PPoint initialSize;
        PPoint maximumSize;
        style->SliderGetPreferredSize(*target, data, &initialSize, &maximumSize);
        Ptr<PtrTarget> privateData = data.StyleData;
        ASSERT_NE(privateData, nullptr);

        // Measurement queries must not consume the pending property change.
        data.MinLabel = "A substantially longer minimum label";
        style->SliderPropertyChanged(*target, data, PSliderProperty::MinLabel);
        PPoint changedSize;
        style->SliderGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_GT(changedSize.x, initialSize.x);
        EXPECT_EQ(data.StyleData, privateData);

        // Batch both endpoints, with no intervening paint.
        data.MinLabel = "i";
        data.MaxLabel = "i";
        style->SliderPropertyChanged(*target, data, PSliderProperties(PSliderProperty::MinLabel, PSliderProperty::MaxLabel));
        style->SliderGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_EQ(changedSize, initialSize);

        data.MaxLabel = "Maximum";
        style->SliderPropertyChanged(*target, data, PSliderProperty::MaxLabel);
        PPoint smallFontSize;
        style->SliderGetPreferredSize(*target, data, &smallFontSize, &maximumSize);
        font->Set(PFontID::e_FontLarge);
        target->SetFont(font);
        style->SliderPropertyChanged(*target, data, PSliderProperty::Font);
        style->SliderGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_GT(changedSize.x, smallFontSize.x);
        EXPECT_GT(changedSize.y, smallFontSize.y);
    });
}
