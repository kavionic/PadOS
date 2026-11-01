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
        Ptr<PView> target = ptr_new<PView>("dropdown_style_target");
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
        EXPECT_EQ(menu->GetStyle(), nullptr);
        EXPECT_EQ(menu->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
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
        Ptr<PView> target = ptr_new<PView>("dropdown_popup_target");
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

TEST(DropdownMenuStyle, PopupViewsAcquireGeometryWhenTheyGainAnApplication)
{
    RunGUITest([](PApplication& application)
    {
        const std::vector<PString> items{"First", "Second", "Third"};
        Ptr<osi::DropdownMenuPopupWindow> popup = ptr_new<osi::DropdownMenuPopupWindow>(items, 1);
        EXPECT_EQ(popup->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
        Ptr<PView> content = popup->GetChildAt(size_t(0));
        ASSERT_NE(content, nullptr);
        EXPECT_EQ(content->GetContentSize(), PPoint());
        ASSERT_TRUE(application.AddHandler(popup));
        PScopeExit removePopup([&application, &popup] { application.RemoveHandler(popup); });
        popup->OnStyleChanged();
        const PPoint popupSize = popup->GetPreferredSize(PPrefSizeType::Smallest);
        EXPECT_GT(popupSize.x, 0.0f);
        EXPECT_GT(popupSize.y, 0.0f);
        popup->SetFrame(PRect::FromSize(popupSize));
        EXPECT_EQ(content->GetFrame(), PRect(2.0f, 4.0f, popupSize.x - 2.0f, popupSize.y - 4.0f));
        EXPECT_EQ(content->GetContentSize() + PPoint(4.0f, 8.0f), popupSize);
    });
}
