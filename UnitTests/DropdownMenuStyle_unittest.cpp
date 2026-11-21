// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <cmath>

#include <gtest/gtest.h>

#include <GUI/Style/DropdownMenuPopupStyleData.h>
#include <GUI/Style/DropdownMenuStyleData.h>
#include <GUI/Style/ViewRendererDropdownMenu.h>
#include <GUI/Style/ViewRendererDropdownMenuPopup.h>
#include <GUI/Style/ViewRendererDropdownMenuPopupFrame.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuPopupFrameSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuPopupSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuSkinned.h>
#include <GUI/Widgets/DropdownMenu.h>
#include <GUI/Widgets/TextBox.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

#include "../System/GUI/Widgets/DropdownMenu/DropdownMenuPopupView.h"
#include "GUITest.h"
#include "ViewRendererTest.h"

TEST(DropdownMenuStyle, EditorLayoutAndHitTestingRespectReadOnlyAndEnabledState)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("dropdown_style_target");
        target->SetFrame(PRect(0.0f, 0.0f, 200.0f, 40.0f));
        PDropdownMenuStyleData& data = target->GetDropdownMenuStyleData();
        PViewRendererTestBinding<PViewRendererDropdownMenu> dropdownRenderer(*target, style, PGUIComponentID::DropdownMenu);
        EXPECT_EQ(dropdownRenderer->GetEditorFrame(*target, dropdownRenderer.GetContext()), PRect(0.0f, 0.0f, 164.0f, 40.0f));
        EXPECT_TRUE(dropdownRenderer->HitTest(*target, PPoint(180.0f, 20.0f), dropdownRenderer.GetContext()));
        EXPECT_FALSE(dropdownRenderer->HitTest(*target, PPoint(40.0f, 20.0f), dropdownRenderer.GetContext()));
        data.IsReadOnly = true;
        EXPECT_TRUE(dropdownRenderer->HitTest(*target, PPoint(40.0f, 20.0f), dropdownRenderer.GetContext()));
        data.IsEnabled = false;
        EXPECT_FALSE(dropdownRenderer->HitTest(*target, PPoint(180.0f, 20.0f), dropdownRenderer.GetContext()));
        target->SetFrame(PRect(0.0f, 0.0f, 10.0f, 40.0f));
        EXPECT_EQ(dropdownRenderer->GetEditorFrame(*target, dropdownRenderer.GetContext()).Width(), 0.0f);
        target->SetFrame(PRect());
        EXPECT_EQ(dropdownRenderer->GetEditorFrame(*target, dropdownRenderer.GetContext()), PRect());
    });
}

TEST(DropdownMenuStyle, ItemAndTextChangesRefreshPreferredSizes)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("dropdown_style_target", PViewFlag::WillDraw);
        PDropdownMenuStyleData& data = target->GetDropdownMenuStyleData();
        PViewRendererTestBinding<PViewRendererDropdownMenu> dropdownRenderer(*target, style, PGUIComponentID::DropdownMenu);
        data.EditorSize = PPoint(8.0f, 38.0f);
        data.Items.push_back("i");
        PPoint initialSize;
        PPoint maximumSize;
        dropdownRenderer->GetPreferredSize(*target, &initialSize, &maximumSize, dropdownRenderer.GetContext());
        PtrTarget* privateData = dropdownRenderer.GetContext();
        ASSERT_NE(privateData, nullptr);
        data.Items.push_back("A substantially longer menu label");
        dropdownRenderer->PropertyChanged(*target, PDropdownMenuProperty::Items, dropdownRenderer.GetContext());
        PPoint changedSize;
        dropdownRenderer->GetPreferredSize(*target, &changedSize, &maximumSize, dropdownRenderer.GetContext());
        EXPECT_GT(changedSize.x, initialSize.x);
        EXPECT_EQ(dropdownRenderer.GetContext(), privateData);
        data.Items.pop_back();
        dropdownRenderer->PropertyChanged(*target, PDropdownMenuProperty::Items, dropdownRenderer.GetContext());
        dropdownRenderer->GetPreferredSize(*target, &changedSize, &maximumSize, dropdownRenderer.GetContext());
        EXPECT_EQ(changedSize, initialSize);
        data.Items.clear();
        const PString text = "Custom text";
        data.Text = &text;
        dropdownRenderer->PropertyChanged(
            *target,
            PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Text),
            dropdownRenderer.GetContext());
        dropdownRenderer->GetPreferredSize(*target, &changedSize, &maximumSize, dropdownRenderer.GetContext());
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

TEST(DropdownMenuStyle, StyleChangesRefreshCachedPreferredSizesAfterEditorBordersChange)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PDefaultGUIStyle> defaultStyle = ptr_new<PDefaultGUIStyle>(&application);
        const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, "Styles/Skinned/Normal"));
        const Ptr<PSkinnableGUIStyle> skinnedStyle = ptr_new<PSkinnableGUIStyle>(directory, &application);
        application.SetGUIStyle(defaultStyle);
        Ptr<PDropdownMenu> menu = ptr_new<PDropdownMenu>("dropdown_style_change");
        menu->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
        menu->AppendItem("A sufficiently long item to exceed the skin minimum width");
        menu->SetFrame(PRect(0.0f, 0.0f, 200.0f, 40.0f));
        menu->Hide();
        ASSERT_TRUE(application.AddView(menu, PViewDockType::TopLevelView));
        PScopeExit removeMenu([&application, &menu] { application.RemoveView(menu); });
        const Ptr<PTextBox> editor = menu->GetChildAt<PTextBox>(size_t(0));
        ASSERT_NE(editor, nullptr);
        const PRect defaultBorders = PTextBox::GetDefaultStyle()->Borders;
        ASSERT_NE(defaultBorders, PRect());
        const auto checkPreferredSize = [&menu, &editor](const PRect& expectedBorders)
        {
            EXPECT_EQ(editor->GetStyle()->Borders, expectedBorders);
            const PPoint cachedMinimum = menu->GetPreferredSize(PPrefSizeType::Smallest);
            const PViewRendererDropdownMenu* renderer = menu->GetRenderer();
            ASSERT_NE(renderer, nullptr);
            PPoint minimum;
            PPoint maximum;
            renderer->GetPreferredSize(*menu, &minimum, &maximum, menu->GetRendererContext());
            EXPECT_EQ(cachedMinimum, minimum);
        };
        checkPreferredSize(defaultBorders);
        application.SetGUIStyle(skinnedStyle);
        checkPreferredSize(PRect());
        application.SetGUIStyle(defaultStyle);
        checkPreferredSize(defaultBorders);
    });
}

TEST(DropdownMenuStyle, PopupRowGeometryTracksFontsAndHandlesAnEmptyList)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("dropdown_popup_target", PViewFlag::WillDraw);
        target->SetFrame(PRect(0.0f, 0.0f, 150.0f, 100.0f));
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
        target->SetFont(font);
        std::vector<PString>& items = target->GetPopupItems();
        items = {"First", "Second", "Third"};
        PViewRendererTestBinding<PViewRendererDropdownMenuPopup> popupRenderer(*target, style, PGUIComponentID::DropdownMenuPopup);
        PViewRendererTestBinding<PViewRendererDropdownMenuPopupFrame> frameRenderer(*target, style, PGUIComponentID::DropdownMenuPopupFrame);
        PPoint initialSize;
        PPoint maximumSize;
        popupRenderer->GetPreferredSize(*target, &initialSize, &maximumSize, popupRenderer.GetContext());
        for (size_t i = 0; i < items.size(); ++i)
        {
            const PRect itemFrame = popupRenderer->GetItemFrame(*target, i, popupRenderer.GetContext());
            EXPECT_EQ(popupRenderer->GetItemFromPosition(*target, itemFrame.Center(), popupRenderer.GetContext()), i);
        }
        EXPECT_EQ(popupRenderer->GetItemFromPosition(*target, PPoint(10.0f, -20.0f), popupRenderer.GetContext()), 0u);
        EXPECT_EQ(popupRenderer->GetItemFromPosition(*target, PPoint(10.0f, 1000.0f), popupRenderer.GetContext()), items.size() - 1);
        font->Set(PFontID::e_FontLarge);
        target->SetFont(font);
        popupRenderer->PropertyChanged(*target, PDropdownMenuPopupProperty::Font, popupRenderer.GetContext());
        PPoint changedSize;
        popupRenderer->GetPreferredSize(*target, &changedSize, &maximumSize, popupRenderer.GetContext());
        EXPECT_GT(changedSize.y, initialSize.y);
        EXPECT_EQ(frameRenderer->GetContentFrame(*target, frameRenderer.GetContext()), PRect(2.0f, 4.0f, 148.0f, 96.0f));
        PPoint frameSize;
        target->SetPopupContentSize(changedSize);
        frameRenderer->GetPreferredSize(*target, &frameSize, &maximumSize, frameRenderer.GetContext());
        EXPECT_EQ(frameSize, changedSize + PPoint(4.0f, 8.0f));
        items.clear();
        popupRenderer->PropertyChanged(*target, PDropdownMenuPopupProperty::Items, popupRenderer.GetContext());
        popupRenderer->GetPreferredSize(*target, &changedSize, &maximumSize, popupRenderer.GetContext());
        EXPECT_EQ(changedSize.y, 0.0f);
        EXPECT_EQ(popupRenderer->GetItemFromPosition(*target, PPoint(), popupRenderer.GetContext()), INVALID_INDEX);
        EXPECT_EQ(popupRenderer->GetItemFrame(*target, 0, popupRenderer.GetContext()), PRect());
    });
}

TEST(DropdownMenuStyle, PopupGeometryCanBeCalculatedBeforeHandlerRegistration)
{
    RunGUITest([](PApplication& application)
    {
        const std::vector<PString> items{"First", "Second", "Third"};
        Ptr<osi::DropdownMenuPopupWindow> popup = ptr_new<osi::DropdownMenuPopupWindow>(items, 1);
        Ptr<PView> content = popup->GetChildAt(size_t(0));
        ASSERT_NE(content, nullptr);
        content->PreferredSizeChanged();
        const PPoint popupSize = popup->GetPreferredSize(PPrefSizeType::Smallest);
        EXPECT_GT(popupSize.x, 0.0f);
        EXPECT_GT(popupSize.y, 0.0f);
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

            const PViewRendererDropdownMenu* menuRenderer = menu->GetRenderer();
            EXPECT_EQ(editor->GetStyle(), menuRenderer->GetEditorStyle(*menu, menu->GetRendererContext()));
            EXPECT_TRUE(menuRenderer->HitTest(*menu, PPoint(20, 16), menu->GetRendererContext()));
            PPointerEvent mouse;
            mouse.PointerID = PMousePointerID;
            mouse.SupportsHover = true;
            mouse.ToolType = PMotionToolType::Mouse;
            menu->OnPointerEnter(mouse.PointerID, PPoint(20, 16), mouse, PEventPhase::Target);
            EXPECT_EQ(editor->GetStyle(), menuRenderer->GetEditorStyle(*menu, menu->GetRendererContext()));
            menu->OnPointerLeave(mouse.PointerID, PPoint(-1, 16), mouse, PEventPhase::Target);
            EXPECT_EQ(editor->GetStyle(), menuRenderer->GetEditorStyle(*menu, menu->GetRendererContext()));
            menu->ClearFlags(PDropdownMenuFlag::ReadOnly);
            EXPECT_FALSE(menuRenderer->HitTest(*menu, PPoint(20, 16), menu->GetRendererContext()));
            EXPECT_TRUE(menuRenderer->HitTest(*menu, PPoint(185, 16), menu->GetRendererContext()));
            menu->OnPointerEnter(mouse.PointerID, PPoint(20, 16), mouse, PEventPhase::Target);
            EXPECT_EQ(editor->GetStyle(), menuRenderer->GetEditorStyle(*menu, menu->GetRendererContext()));
            menu->OnPointerMove(mouse.PointerID, PPoint(185, 16), mouse, PEventPhase::Target);
            EXPECT_EQ(editor->GetStyle(), menuRenderer->GetEditorStyle(*menu, menu->GetRendererContext()));
            menu->SetEnable(false);
            EXPECT_EQ(editor->GetStyle(), menuRenderer->GetEditorStyle(*menu, menu->GetRendererContext()));
            EXPECT_FALSE(menuRenderer->HitTest(*menu, PPoint(185, 16), menu->GetRendererContext()));

            Ptr<PViewRendererTestView> drawingTarget = ptr_new<PViewRendererTestView>("dropdown_paint_target", PViewFlag::WillDraw);
            drawingTarget->SetFrame(menu->GetFrame());
            drawingTarget->SetFont(menu->GetFont());
            PDropdownMenuStyleData& data = drawingTarget->GetDropdownMenuStyleData();
            PViewRendererTestBinding<PViewRendererDropdownMenu> drawingRenderer(*drawingTarget, *style, PGUIComponentID::DropdownMenu);
            Ptr<const PBrush> previousBrush;
            for (PButtonSkinState state : {PButtonSkinState::Normal, PButtonSkinState::Hovered, PButtonSkinState::Pressed, PButtonSkinState::Disabled})
            {
                data.IsEnabled = state != PButtonSkinState::Disabled;
                data.IsOpen = state == PButtonSkinState::Pressed;
                data.IsPointerOver = state == PButtonSkinState::Hovered;
                drawingTarget->SetDrawingMode(PDrawingMode::Blend);
                drawingRenderer->Paint(*drawingTarget, drawingTarget->GetBounds(), drawingTarget->GetBounds(), drawingRenderer.GetContext());
                EXPECT_EQ(drawingTarget->GetDrawingMode(), PDrawingMode::Blend);
                EXPECT_EQ(drawingTarget->GetBrush(), nullptr);
                EXPECT_NE(drawingTarget->GetBackgroundBrush(), previousBrush);
                previousBrush = drawingTarget->GetBackgroundBrush();
                EXPECT_EQ(drawingRenderer->GetEditorFrame(*drawingTarget, drawingRenderer.GetContext()), PRect(10, 4, 164, 27));
            }
            menu->SetEnable(true);
            menu->SetFont(ptr_new<PFont>(PFontID::e_FontLarge));
            EXPECT_GT(menu->GetPreferredSize(PPrefSizeType::Smallest).y, preferred.y);
            for (const PPoint& size : {PPoint(10, 5), PPoint()})
            {
                menu->SetFrame(PRect::FromSize(size));
                const PRect frame = menuRenderer->GetEditorFrame(*menu, menu->GetRendererContext());
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
            Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("skinned_popup", PViewFlag::WillDraw);
            target->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
            target->SetFrame(PRect(0, 0, 200, 80));
            target->Hide();
            ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
            PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
            std::vector<PString>& items = target->GetPopupItems();
            items = {"First", "Second", "Third"};
            PDropdownMenuPopupStyleData& data = target->GetDropdownMenuPopupStyleData();
            PViewRendererTestBinding<PViewRendererDropdownMenuPopup> popupRenderer(*target, style, PGUIComponentID::DropdownMenuPopup);
            PViewRendererTestBinding<PViewRendererDropdownMenuPopupFrame> frameRenderer(*target, style, PGUIComponentID::DropdownMenuPopupFrame);
            PPoint minimum;
            PPoint maximum;
            popupRenderer->GetPreferredSize(*target, &minimum, &maximum, popupRenderer.GetContext());
            EXPECT_EQ(minimum, maximum);
            EXPECT_FLOAT_EQ(minimum.y, 96.0f);
            EXPECT_EQ(frameRenderer->GetContentFrame(*target, frameRenderer.GetContext()), PRect(4, 4, 196, 76));
            PPoint popupSize;
            target->SetPopupContentSize(minimum);
            frameRenderer->GetPreferredSize(*target, &popupSize, &maximum, frameRenderer.GetContext());
            EXPECT_EQ(popupSize, minimum + PPoint(8, 8));
            const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(1, 2, 198, 78));
            target->SetDrawingRegion(clip);
            target->SetDrawingMode(PDrawingMode::Overlay);
            frameRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), frameRenderer.GetContext());
            for (size_t index = 0; index < items.size(); ++index)
            {
                const PRect frame = popupRenderer->GetItemFrame(*target, index, popupRenderer.GetContext());
                EXPECT_EQ(frame, PRect(0, float(index) * 32.0f, 200, float(index + 1) * 32.0f));
                EXPECT_EQ(popupRenderer->GetItemFromPosition(*target, frame.Center(), popupRenderer.GetContext()), index);
                data.Selection = index;
                data.PressedItem = (index + 1) % items.size();
                popupRenderer->PropertyChanged(
                    *target,
                    PDropdownMenuPopupProperties(PDropdownMenuPopupProperty::Selection, PDropdownMenuPopupProperty::Pressed),
                    popupRenderer.GetContext());
                popupRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), popupRenderer.GetContext());
                EXPECT_EQ(target->GetDrawingRegion(), clip);
                EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Overlay);
                EXPECT_EQ(target->GetBrush(), nullptr);
            }
            PtrTarget* const privateData = popupRenderer.GetContext();
            target->ScrollTo(PPoint(0, -32));
            EXPECT_EQ(popupRenderer->GetItemFromPosition(*target, target->GetBounds().TopLeft(), popupRenderer.GetContext()), 1u);
            popupRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), popupRenderer.GetContext());
            target->SetFont(ptr_new<PFont>(PFontID::e_FontLarge));
            popupRenderer->PropertyChanged(*target, PDropdownMenuPopupProperty::Font, popupRenderer.GetContext());
            PPoint larger;
            popupRenderer->GetPreferredSize(*target, &larger, &maximum, popupRenderer.GetContext());
            EXPECT_GT(larger.y, minimum.y);
            EXPECT_EQ(popupRenderer.GetContext(), privateData);
            items.clear();
            popupRenderer->PropertyChanged(*target, PDropdownMenuPopupProperty::Items, popupRenderer.GetContext());
            popupRenderer->GetPreferredSize(*target, &minimum, &maximum, popupRenderer.GetContext());
            EXPECT_FLOAT_EQ(minimum.y, 0.0f);
            EXPECT_EQ(popupRenderer->GetItemFromPosition(*target, PPoint(), popupRenderer.GetContext()), INVALID_INDEX);
            EXPECT_EQ(popupRenderer->GetItemFrame(*target, 0, popupRenderer.GetContext()), PRect());
            EXPECT_NO_THROW(popupRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), popupRenderer.GetContext()));
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
        Ptr<PViewRendererTestView> first = ptr_new<PViewRendererTestView>("first_popup", PViewFlag::WillDraw);
        Ptr<PViewRendererTestView> second = ptr_new<PViewRendererTestView>("second_popup", PViewFlag::WillDraw);
        first->SetFrame(PRect(0, 0, 200, 40));
        second->SetFrame(PRect(0, 0, 120, 60));
        PViewRendererTestBinding<PViewRendererDropdownMenu> fieldRenderer(*first, ptr_new<PViewRendererDropdownMenuSkinned>(skin));
        PViewRendererTestBinding<PViewRendererDropdownMenuPopupFrame> frameRenderer(
            *first,
            ptr_new<PViewRendererDropdownMenuPopupFrameSkinned>(skin, application.GetStandardBrush(PStandardColorID::DefaultBackground)));
        EXPECT_EQ(fieldRenderer->GetEditorFrame(*first, fieldRenderer.GetContext()), PRect(-2, 3, 149, 34));
        EXPECT_EQ(frameRenderer->GetContentFrame(*first, frameRenderer.GetContext()), PRect(7, 5, 191, 34));
        EXPECT_FALSE(fieldRenderer->HitTest(*first, PPoint(152, 20), fieldRenderer.GetContext()));
        EXPECT_TRUE(fieldRenderer->HitTest(*first, PPoint(153, 20), fieldRenderer.GetContext()));
        const std::vector<PString> items{"A", "B"};
        first->SetPopupItems(items);
        second->SetPopupItems(items);
        const Ptr<const PViewRendererDropdownMenuPopup> popupRenderer = ptr_new<PViewRendererDropdownMenuPopupSkinned>(skin);
        PViewRendererTestBinding<PViewRendererDropdownMenuPopup> firstPopupRenderer(*first, popupRenderer);
        PViewRendererTestBinding<PViewRendererDropdownMenuPopup> secondPopupRenderer(*second, popupRenderer);
        const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(1, 2, 190, 38));
        first->SetDrawingRegion(clip);
        firstPopupRenderer->Paint(*first, first->GetBounds(), first->GetBounds(), firstPopupRenderer.GetContext());
        secondPopupRenderer->Paint(*second, second->GetBounds(), second->GetBounds(), secondPopupRenderer.GetContext());
        EXPECT_NE(firstPopupRenderer.GetContext(), secondPopupRenderer.GetContext());
        EXPECT_EQ(first->GetDrawingRegion(), clip);
        EXPECT_EQ(image->GetDescription(), description);
        first->SetFrame(PRect(0, 0, 50, 12));
        firstPopupRenderer->Paint(*first, first->GetBounds(), first->GetBounds(), firstPopupRenderer.GetContext());
        EXPECT_EQ(image->GetDescription(), description);
    });
}
