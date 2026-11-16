// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <cmath>

#include <gtest/gtest.h>
#include <GUI/Style/DropdownMenuStyleData.h>
#include <GUI/Style/DropdownMenuPopupStyleData.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <GUI/Widgets/DropdownMenu.h>
#include <GUI/Widgets/TextBox.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"
#include "../System/GUI/Widgets/DropdownMenu/DropdownMenuPopupView.h"

TEST(DropdownMenuStyle, EditorLayoutAndHitTestingRespectReadOnlyAndEnabledState)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("dropdown_style_target");
        target->SetFrame(PRect(0.0f, 0.0f, 200.0f, 40.0f));
        PDropdownMenuStyleData data;
        EXPECT_EQ(style.DropdownMenuGetEditorFrame(*target, data), PRect(0.0f, 0.0f, 164.0f, 40.0f));
        EXPECT_TRUE(style.DropdownMenuHitTest(*target, data, PPoint(180.0f, 20.0f)));
        EXPECT_FALSE(style.DropdownMenuHitTest(*target, data, PPoint(40.0f, 20.0f)));
        data.IsReadOnly = true;
        EXPECT_TRUE(style.DropdownMenuHitTest(*target, data, PPoint(40.0f, 20.0f)));
        data.IsEnabled = false;
        EXPECT_FALSE(style.DropdownMenuHitTest(*target, data, PPoint(180.0f, 20.0f)));
        target->SetFrame(PRect(0.0f, 0.0f, 10.0f, 40.0f));
        EXPECT_EQ(style.DropdownMenuGetEditorFrame(*target, data).Width(), 0.0f);
        target->SetFrame(PRect());
        EXPECT_EQ(style.DropdownMenuGetEditorFrame(*target, data), PRect());
    });
}

TEST(DropdownMenuStyle, ItemAndTextChangesRefreshPreferredSizes)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("dropdown_style_target", PViewFlag::WillDraw);
        PDropdownMenuStyleData data;
        data.EditorSize = PPoint(8.0f, 38.0f);
        data.Items.push_back("i");
        PPoint initialSize;
        PPoint maximumSize;
        style.DropdownMenuGetPreferredSize(*target, data, &initialSize, &maximumSize);
        Ptr<PtrTarget> privateData = data.StyleData;
        ASSERT_NE(privateData, nullptr);
        data.Items.push_back("A substantially longer menu label");
        style.DropdownMenuPropertyChanged(*target, data, PDropdownMenuProperty::Items);
        PPoint changedSize;
        style.DropdownMenuGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_GT(changedSize.x, initialSize.x);
        EXPECT_EQ(data.StyleData, privateData);
        data.Items.pop_back();
        style.DropdownMenuPropertyChanged(*target, data, PDropdownMenuProperty::Items);
        style.DropdownMenuGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_EQ(changedSize, initialSize);
        data.Items.clear();
        const PString text = "Custom text";
        data.Text = &text;
        style.DropdownMenuPropertyChanged(*target, data, PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Text));
        style.DropdownMenuGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_FLOAT_EQ(changedSize.x, data.EditorSize.x + target->GetStringWidth(text) + 10.0f + std::round(data.EditorSize.y * 0.9f));
    });
}

TEST(DropdownMenuStyle, WidgetUsesTheStyleAndUpdatesItsEditorFont)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PDropdownMenu> menu = ptr_new<PDropdownMenu>("dropdown");
        menu->AppendItem("First item");
        menu->AppendItem("A longer second item");
        menu->SetSelection(1, false);
        EXPECT_EQ(menu->GetStyle(), application.GetGUIStyle());
        EXPECT_GT(menu->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        EXPECT_GT(menu->GetPreferredSize(PPrefSizeType::Smallest).y, 0.0f);
        EXPECT_EQ(menu->GetCurrentString(), "A longer second item");
        const PPointerEvent event;
        menu->OnPointerDown(1, PPoint(190.0f, 20.0f), event, PEventPhase::Target);
        EXPECT_EQ(menu->GetSelection(), 1u);

        ASSERT_TRUE(application.AddHandler(menu));
        PScopeExit removeMenu([&application, &menu] { application.RemoveHandler(menu); });
        menu->OnStyleChanged();
        menu->SetFrame(PRect(0.0f, 0.0f, 200.0f, 40.0f));
        Ptr<PTextBox> editor = ptr_static_cast<PTextBox>(menu->GetChildAt(size_t(0)));
        ASSERT_NE(editor, nullptr);
        EXPECT_EQ(editor->GetFrame(), PRect(0.0f, 0.0f, 164.0f, 40.0f));
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
        menu->SetFont(font);
        const PPoint smallSize = menu->GetPreferredSize(PPrefSizeType::Smallest);
        EXPECT_GT(smallSize.x, 0.0f);
        EXPECT_EQ(editor->GetFont(), font);
        font->Set(PFontID::e_FontLarge);
        menu->SetFont(font);
        EXPECT_GT(menu->GetPreferredSize(PPrefSizeType::Smallest).x, smallSize.x);
        EXPECT_GT(menu->GetPreferredSize(PPrefSizeType::Smallest).y, smallSize.y);
        EXPECT_EQ(editor->GetFont(), font);
        menu->MergeFlags(PDropdownMenuFlag::ReadOnly);
        EXPECT_TRUE(editor->HasFlags(PTextBoxFlag::ReadOnly));
        menu->ClearFlags(PDropdownMenuFlag::ReadOnly);
        EXPECT_FALSE(editor->HasFlags(PTextBoxFlag::ReadOnly));
        menu->SetEnable(false);
        EXPECT_FALSE(editor->IsEnabled());
        menu->Clear();
        EXPECT_EQ(menu->GetItemCount(), 0u);
    });
}

TEST(DropdownMenuStyle, PopupRowGeometryTracksFontsAndHandlesAnEmptyList)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("dropdown_popup_target", PViewFlag::WillDraw);
        target->SetFrame(PRect(0.0f, 0.0f, 150.0f, 100.0f));
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
        target->SetFont(font);
        std::vector<PString> items{"First", "Second", "Third"};
        PDropdownMenuPopupStyleData data(items);
        PPoint initialSize;
        PPoint maximumSize;
        style.DropdownMenuPopupGetPreferredSize(*target, data, &initialSize, &maximumSize);
        for (size_t i = 0; i < items.size(); ++i)
        {
            const PRect itemFrame = style.DropdownMenuPopupGetItemFrame(*target, data, i);
            EXPECT_EQ(style.DropdownMenuPopupGetItemFromPosition(*target, data, itemFrame.Center()), i);
        }
        EXPECT_EQ(style.DropdownMenuPopupGetItemFromPosition(*target, data, PPoint(10.0f, -20.0f)), 0u);
        EXPECT_EQ(style.DropdownMenuPopupGetItemFromPosition(*target, data, PPoint(10.0f, 1000.0f)), items.size() - 1);
        font->Set(PFontID::e_FontLarge);
        target->SetFont(font);
        style.DropdownMenuPopupPropertyChanged(*target, data, PDropdownMenuPopupProperty::Font);
        PPoint changedSize;
        style.DropdownMenuPopupGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_GT(changedSize.y, initialSize.y);
        EXPECT_EQ(style.DropdownMenuPopupFrameGetContentFrame(*target), PRect(2.0f, 4.0f, 148.0f, 96.0f));
        PPoint frameSize;
        style.DropdownMenuPopupFrameGetPreferredSize(*target, changedSize, &frameSize, &maximumSize);
        EXPECT_EQ(frameSize, changedSize + PPoint(4.0f, 8.0f));
        items.clear();
        style.DropdownMenuPopupPropertyChanged(*target, data, PDropdownMenuPopupProperty::Items);
        style.DropdownMenuPopupGetPreferredSize(*target, data, &changedSize, &maximumSize);
        EXPECT_EQ(changedSize.y, 0.0f);
        EXPECT_EQ(style.DropdownMenuPopupGetItemFromPosition(*target, data, PPoint()), INVALID_INDEX);
        EXPECT_EQ(style.DropdownMenuPopupGetItemFrame(*target, data, 0), PRect());
    });
}

TEST(DropdownMenuStyle, PopupGeometryIsAvailableBeforeHandlerRegistration)
{
    RunGUITest([](PApplication& application)
    {
        const std::vector<PString> items{"First", "Second", "Third"};
        Ptr<osi::DropdownMenuPopupWindow> popup = ptr_new<osi::DropdownMenuPopupWindow>(items, 1);
        const PPoint popupSize = popup->GetPreferredSize(PPrefSizeType::Smallest);
        EXPECT_GT(popupSize.x, 0.0f);
        EXPECT_GT(popupSize.y, 0.0f);
        Ptr<PView> content = popup->GetChildAt(size_t(0));
        ASSERT_NE(content, nullptr);
        EXPECT_GT(content->GetContentSize().x, 0.0f);
        EXPECT_GT(content->GetContentSize().y, 0.0f);
        EXPECT_EQ(content->GetPreferredSize(PPrefSizeType::Smallest), content->GetContentSize());
        EXPECT_EQ(content->GetContentSize() + PPoint(4.0f, 8.0f), popupSize);
        ASSERT_TRUE(application.AddHandler(popup));
        PScopeExit removePopup([&application, &popup] { application.RemoveHandler(popup); });
        popup->OnStyleChanged();
        EXPECT_EQ(popup->GetPreferredSize(PPrefSizeType::Smallest), popupSize);
        popup->SetFrame(PRect::FromSize(popupSize));
        EXPECT_EQ(content->GetFrame(), PRect(2.0f, 4.0f, popupSize.x - 2.0f, popupSize.y - 4.0f));
        EXPECT_EQ(content->GetContentSize() + PPoint(4.0f, 8.0f), popupSize);
    });
}

TEST(DropdownMenuStyle, SkinnedFieldsConfigureTheirEditorAndTrackHoverWithoutOpeningAWindow)
{
    for (const char* theme : {"Normal", "Dark"})
    {
        RunGUITest([theme](PApplication& application)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            const Ptr<PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>(directory, &application);
            application.SetGUIStyle(style);
            Ptr<PDropdownMenu> menu = ptr_new<PDropdownMenu>("skinned_dropdown", PDropdownMenuFlag::ReadOnly);
            menu->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
            menu->AppendItem("First item");
            menu->AppendItem("A longer second item");
            menu->SetSelection(1, false);
            menu->SetFrame(PRect(0, 0, 200, 32));
            const Ptr<PTextBox> editor = menu->GetChildAt<PTextBox>(size_t(0));
            ASSERT_NE(editor, nullptr);
            EXPECT_FALSE(editor->GetStyle()->HasFrame);
            EXPECT_EQ(editor->GetStyle()->Borders, PRect());
            EXPECT_EQ(editor->GetFrame(), PRect(10, 4, 164, 27));
            EXPECT_EQ(editor->GetChildAt(size_t(0))->GetFrame(), PRect(0, 0, 154, 23));
            EXPECT_EQ(menu->GetCurrentString(), "A longer second item");
            const PPoint preferred = menu->GetPreferredSize(PPrefSizeType::Smallest);
            EXPECT_FLOAT_EQ(preferred.y, 32.0f);

            PDropdownMenuStyleData data;
            data.IsReadOnly = true;
            EXPECT_EQ(editor->GetStyle(), style->DropdownMenuGetEditorStyle(data));
            EXPECT_TRUE(style->DropdownMenuHitTest(*menu, data, PPoint(20, 16)));
            PPointerEvent mouse;
            mouse.PointerID = PMousePointerID;
            mouse.SupportsHover = true;
            mouse.ToolType = PMotionToolType::Mouse;
            menu->OnPointerEnter(mouse.PointerID, PPoint(20, 16), mouse, PEventPhase::Target);
            data.IsPointerOver = true;
            EXPECT_EQ(editor->GetStyle(), style->DropdownMenuGetEditorStyle(data));
            menu->OnPointerLeave(mouse.PointerID, PPoint(-1, 16), mouse, PEventPhase::Target);
            data.IsPointerOver = false;
            EXPECT_EQ(editor->GetStyle(), style->DropdownMenuGetEditorStyle(data));
            menu->ClearFlags(PDropdownMenuFlag::ReadOnly);
            data.IsReadOnly = false;
            EXPECT_FALSE(style->DropdownMenuHitTest(*menu, data, PPoint(20, 16)));
            EXPECT_TRUE(style->DropdownMenuHitTest(*menu, data, PPoint(185, 16)));
            menu->OnPointerEnter(mouse.PointerID, PPoint(20, 16), mouse, PEventPhase::Target);
            EXPECT_EQ(editor->GetStyle(), style->DropdownMenuGetEditorStyle(data));
            menu->OnPointerMove(mouse.PointerID, PPoint(185, 16), mouse, PEventPhase::Target);
            data.IsPointerOver = true;
            EXPECT_EQ(editor->GetStyle(), style->DropdownMenuGetEditorStyle(data));
            menu->SetEnable(false);
            data.IsEnabled = false;
            EXPECT_EQ(editor->GetStyle(), style->DropdownMenuGetEditorStyle(data));
            EXPECT_FALSE(style->DropdownMenuHitTest(*menu, data, PPoint(185, 16)));

            Ptr<const PBrush> previousBrush;
            for (PButtonSkinState state : {PButtonSkinState::Normal, PButtonSkinState::Hovered, PButtonSkinState::Pressed, PButtonSkinState::Disabled})
            {
                data.IsEnabled = state != PButtonSkinState::Disabled;
                data.IsOpen = state == PButtonSkinState::Pressed;
                data.IsPointerOver = state == PButtonSkinState::Hovered;
                menu->SetDrawingMode(PDrawingMode::Blend);
                style->DropdownMenuPaint(*menu, menu->GetBounds(), data);
                EXPECT_EQ(menu->GetDrawingMode(), PDrawingMode::Blend);
                EXPECT_EQ(menu->GetBrush(), nullptr);
                EXPECT_NE(menu->GetBackgroundBrush(), previousBrush);
                previousBrush = menu->GetBackgroundBrush();
                EXPECT_EQ(style->DropdownMenuGetEditorFrame(*menu, data), PRect(10, 4, 164, 27));
            }
            menu->SetEnable(true);
            menu->SetFont(ptr_new<PFont>(PFontID::e_FontLarge));
            EXPECT_GT(menu->GetPreferredSize(PPrefSizeType::Smallest).y, preferred.y);
            for (const PPoint& size : {PPoint(10, 5), PPoint()})
            {
                menu->SetFrame(PRect::FromSize(size));
                const PRect frame = style->DropdownMenuGetEditorFrame(*menu, data);
                EXPECT_GE(frame.Width(), 0.0f);
                EXPECT_GE(frame.Height(), 0.0f);
                EXPECT_NO_THROW(menu->OnPaint(menu->GetBounds()));
            }
        });
    }
}

TEST(DropdownMenuStyle, SkinnedPopupsShareGeometryForPaintingScrollingAndHitTesting)
{
    RunGUITest([](PApplication& application)
    {
        for (const char* theme : {"Normal", "Dark"})
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            const PSkinnableGUIStyle style(directory, &application);
            Ptr<PView> target = ptr_new<PView>("skinned_popup", PViewFlag::WillDraw);
            target->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
            target->SetFrame(PRect(0, 0, 200, 80));
            target->Hide();
            ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
            PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
            std::vector<PString> items{"First", "Second", "Third"};
            PDropdownMenuPopupStyleData data(items);
            PPoint minimum;
            PPoint maximum;
            style.DropdownMenuPopupGetPreferredSize(*target, data, &minimum, &maximum);
            EXPECT_EQ(minimum, maximum);
            EXPECT_FLOAT_EQ(minimum.y, 96.0f);
            EXPECT_EQ(style.DropdownMenuPopupFrameGetContentFrame(*target), PRect(4, 4, 196, 76));
            PPoint popupSize;
            style.DropdownMenuPopupFrameGetPreferredSize(*target, minimum, &popupSize, &maximum);
            EXPECT_EQ(popupSize, minimum + PPoint(8, 8));
            const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(1, 2, 198, 78));
            target->SetDrawingRegion(clip);
            target->SetDrawingMode(PDrawingMode::Overlay);
            style.DropdownMenuPopupFramePaint(*target, target->GetBounds());
            for (size_t index = 0; index < items.size(); ++index)
            {
                const PRect frame = style.DropdownMenuPopupGetItemFrame(*target, data, index);
                EXPECT_EQ(frame, PRect(0, float(index) * 32.0f, 200, float(index + 1) * 32.0f));
                EXPECT_EQ(style.DropdownMenuPopupGetItemFromPosition(*target, data, frame.Center()), index);
                data.Selection = index;
                data.PressedItem = (index + 1) % items.size();
                style.DropdownMenuPopupPropertyChanged(
                    *target,
                    data,
                    PDropdownMenuPopupProperties(PDropdownMenuPopupProperty::Selection, PDropdownMenuPopupProperty::Pressed));
                style.DropdownMenuPopupPaint(*target, target->GetBounds(), data);
                EXPECT_EQ(target->GetDrawingRegion(), clip);
                EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Overlay);
                EXPECT_EQ(target->GetBrush(), nullptr);
            }
            const Ptr<PtrTarget> privateData = data.StyleData;
            target->ScrollTo(PPoint(0, -32));
            EXPECT_EQ(style.DropdownMenuPopupGetItemFromPosition(*target, data, target->GetBounds().TopLeft()), 1u);
            style.DropdownMenuPopupPaint(*target, target->GetBounds(), data);
            target->SetFont(ptr_new<PFont>(PFontID::e_FontLarge));
            style.DropdownMenuPopupPropertyChanged(*target, data, PDropdownMenuPopupProperty::Font);
            PPoint larger;
            style.DropdownMenuPopupGetPreferredSize(*target, data, &larger, &maximum);
            EXPECT_GT(larger.y, minimum.y);
            EXPECT_EQ(data.StyleData, privateData);
            items.clear();
            style.DropdownMenuPopupPropertyChanged(*target, data, PDropdownMenuPopupProperty::Items);
            style.DropdownMenuPopupGetPreferredSize(*target, data, &minimum, &maximum);
            EXPECT_FLOAT_EQ(minimum.y, 0.0f);
            EXPECT_EQ(style.DropdownMenuPopupGetItemFromPosition(*target, data, PPoint()), INVALID_INDEX);
            EXPECT_EQ(style.DropdownMenuPopupGetItemFrame(*target, data, 0), PRect());
            EXPECT_NO_THROW(style.DropdownMenuPopupPaint(*target, target->GetBounds(), data));
        }
        application.Sync();
    });
}

TEST(DropdownMenuStyle, SkinMetricsAllowOverlapAndResizingDoesNotChangeSharedImages)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PBitmap> bitmap = ptr_new<PBitmap>(16, 16, PEColorSpace::RGBA32);
        const Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(4, 4, 12, 12));
        image->SetLayoutSize(PPoint(16, 16), PImageLayoutMode::Absolute);
        const PImageDescription description = image->GetDescription();
        const Ptr<PBrush> brush = ptr_new<PBrush>(image);
        const Ptr<PTextBoxStyle> editor = ptr_new<PTextBoxStyle>();
        editor->Borders = PRect();
        editor->HasFrame = false;
        PDropdownMenuSkin skin;
        for (PDropdownMenuAppearance& appearance : skin.States)
        {
            appearance.Background = brush;
            appearance.Arrow = bitmap;
            appearance.Editor = editor;
        }
        for (PDropdownMenuRowAppearance& appearance : skin.Rows)
        {
            appearance.Image = image;
            appearance.LabelOffset = PPoint(-100, -100);
        }
        skin.PopupFrame = brush;
        skin.PopupBackground = application.GetStandardBrush(PNamedColors::black);
        skin.ArrowWidth = 47.0f;
        skin.EditorInsets = PRect(-2, 3, 4, 6);
        skin.PopupInsets = PRect(7, 5, 9, 6);
        skin.RowPadding = PRect(-2, 3, 4, 6);
        skin.MinimumRowHeight = 19.0f;
        const PSkinnableGUIStyle style(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), PSliderSkin(), PScrollBarSkin(), PTabViewSkin(), skin, &application);
        Ptr<PView> first = ptr_new<PView>("first_popup", PViewFlag::WillDraw);
        Ptr<PView> second = ptr_new<PView>("second_popup", PViewFlag::WillDraw);
        first->SetFrame(PRect(0, 0, 200, 40));
        second->SetFrame(PRect(0, 0, 120, 60));
        PDropdownMenuStyleData fieldData;
        EXPECT_EQ(style.DropdownMenuGetEditorFrame(*first, fieldData), PRect(-2, 3, 149, 34));
        EXPECT_EQ(style.DropdownMenuPopupFrameGetContentFrame(*first), PRect(7, 5, 191, 34));
        EXPECT_FALSE(style.DropdownMenuHitTest(*first, fieldData, PPoint(152, 20)));
        EXPECT_TRUE(style.DropdownMenuHitTest(*first, fieldData, PPoint(153, 20)));
        const std::vector<PString> items{"A", "B"};
        PDropdownMenuPopupStyleData firstData(items);
        PDropdownMenuPopupStyleData secondData(items);
        const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(1, 2, 190, 38));
        first->SetDrawingRegion(clip);
        style.DropdownMenuPopupPaint(*first, first->GetBounds(), firstData);
        style.DropdownMenuPopupPaint(*second, second->GetBounds(), secondData);
        EXPECT_NE(firstData.StyleData, secondData.StyleData);
        EXPECT_EQ(first->GetDrawingRegion(), clip);
        EXPECT_EQ(image->GetDescription(), description);
        first->SetFrame(PRect(0, 0, 50, 12));
        style.DropdownMenuPopupPaint(*first, first->GetBounds(), firstData);
        EXPECT_EQ(image->GetDescription(), description);
        const PSkinnableGUIStyle fallback;
        const PDefaultGUIStyle defaultStyle;
        EXPECT_EQ(fallback.DropdownMenuGetEditorStyle(fieldData), defaultStyle.DropdownMenuGetEditorStyle(fieldData));
        EXPECT_EQ(fallback.DropdownMenuGetEditorFrame(*first, fieldData), defaultStyle.DropdownMenuGetEditorFrame(*first, fieldData));
    });
}
