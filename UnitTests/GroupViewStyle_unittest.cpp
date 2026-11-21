// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>

#include <gtest/gtest.h>

#include <GUI/Font.h>
#include <GUI/LayoutNode.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRendererGroupView.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Styles/Skinned/ViewRendererGroupViewSkinned.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/GroupView.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"
#include "SkinnableGUIStyleTest.h"
#include "ViewRendererTest.h"

TEST(GroupViewStyle, LabelAndFontChangesRefreshManagedPreferredSizes)
{
    RunGUITest([](PApplication& application)
    {
        pugi::xml_document document;
        pugi::xml_node xmlData = document.append_child("GroupView");
        xmlData.append_attribute("label") = "Group";
        PViewFactoryContext context;
        Ptr<PGroupView> group = ptr_new<PGroupView>(context, xmlData);
        EXPECT_EQ(group->GetLabel(), "Group");
        ASSERT_TRUE(application.AddHandler(group));
        PScopeExit removeGroup([&application, &group] { application.RemoveHandler(group); });
        group->OnStyleChanged();
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
        group->SetFont(font);
        const PPoint smallSize = group->GetPreferredSize(PPrefSizeType::Smallest);
        EXPECT_GT(smallSize.x, group->GetStringWidth(group->GetLabel()));
        EXPECT_GT(smallSize.y, 0.0f);
        font->Set(PFontID::e_FontLarge);
        group->SetFont(font);
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).x, smallSize.x);
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).y, smallSize.y);
        const PPoint largeSize = group->GetPreferredSize(PPrefSizeType::Smallest);
        group->SetLabel("A substantially longer group title");
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).x, largeSize.x);
        group->SetLabel(PString::zero);
        EXPECT_LT(group->GetPreferredSize(PPrefSizeType::Smallest).x, smallSize.x);
        EXPECT_LT(group->GetPreferredSize(PPrefSizeType::Smallest).y, smallSize.y);
    });
}

TEST(GroupViewStyle, ChildLayoutAndSizeOverridesRemainEffective)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PGroupView> group = ptr_new<PGroupView>("group");
        Ptr<PView> child = ptr_new<PView>("content");
        child->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 320.0f);
        child->SetWidthOverride(PPrefSizeType::Greatest, PSizeOverride::Always, 400.0f);
        child->SetHeightOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 80.0f);
        child->SetHeightOverride(PPrefSizeType::Greatest, PSizeOverride::Always, 100.0f);
        const PRect borders(10.0f, 20.0f, 30.0f, 40.0f);
        child->SetBorders(borders);
        group->AddChild(child);
        group->SetLayoutNode(ptr_new<PLayoutNode>());
        group->SetLabel("Group");
        EXPECT_EQ(group->GetStyle(), application.GetGUIStyle());
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(360.0f, 140.0f));
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Greatest), PPoint(440.0f, 160.0f));
        ASSERT_TRUE(application.AddHandler(group));
        PScopeExit removeGroup([&application, &group] { application.RemoveHandler(group); });
        group->OnStyleChanged();
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(360.0f, 140.0f));
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Greatest), PPoint(440.0f, 160.0f));
        EXPECT_EQ(child->GetBorders(), borders);
        child->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 380.0f);
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(420.0f, 140.0f));
        group->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 50.0f);
        group->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 60.0f);
        group->SetLabel("A label wider than the explicit width override");
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50.0f, 60.0f));
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Greatest), PPoint(50.0f, 60.0f));
    });
}

TEST(GroupViewStyle, DetachedViewsRetainTheirLabelAndApplicationStyle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PGroupView> group = ptr_new<PGroupView>("group");
        group->SetLabel("Detached group");
        group->SetFrame(PRect(0.0f, 0.0f, 200.0f, 100.0f));
        group->OnPaint(group->GetBounds());
        EXPECT_EQ(group->GetStyle(), application.GetGUIStyle());
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).y, 0.0f);
        ASSERT_TRUE(application.AddHandler(group));
        group->OnStyleChanged();
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        application.RemoveHandler(group);
        group->OnStyleChanged();
        EXPECT_EQ(group->GetStyle(), application.GetGUIStyle());
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).y, 0.0f);
        EXPECT_EQ(group->GetLabel(), "Detached group");
    });
}

TEST(GroupViewStyle, PaintingRestoresThePenWidth)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("group_paint_target", PViewFlag::WillDraw);
        target->SetFrame(PRect(0.0f, 0.0f, 200.0f, 100.0f));
        target->SetPenWidth(7.0f);
        PGroupViewStyleData& data = target->GetGroupViewStyleData();
        const PGUIStyle* style = application.GetGUIStyle();
        ASSERT_NE(style, nullptr);
        PViewRendererTestBinding<PViewRendererGroupView> renderer(*target, *style, PGUIComponentID::GroupView);
        renderer->Paint(*target, target->GetBounds(), target->GetBounds(), renderer.GetContext());
        EXPECT_EQ(target->GetPenWidth(), 7.0f);
        data.Label = "Group";
        renderer->PropertyChanged(*target, PGroupViewProperty::Label, renderer.GetContext());
        renderer->Paint(*target, target->GetBounds(), target->GetBounds(), renderer.GetContext());
        EXPECT_EQ(target->GetPenWidth(), 7.0f);
    });
}

TEST(GroupViewStyle, SkinnedGroupsTrackTitlesAndFontsWithoutChangingChildLayout)
{
    for (const char* theme : {"Normal", "Dark"})
    {
        RunGUITest([theme](PApplication& application)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            const Ptr<PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>(directory, &application);
            application.SetGUIStyle(style);
            Ptr<PGroupView> group = ptr_new<PGroupView>("skinned_group");
            group->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
            EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(28, 28));
            group->SetLabel("Motion");
            const PPoint smallSize = group->GetPreferredSize(PPrefSizeType::Smallest);
            EXPECT_FLOAT_EQ(smallSize.x, group->GetStringWidth("Motion") + 28.0f);
            group->SetFont(ptr_new<PFont>(PFontID::e_FontLarge));
            const PPoint largeSize = group->GetPreferredSize(PPrefSizeType::Smallest);
            EXPECT_GT(largeSize.x, smallSize.x);
            EXPECT_GT(largeSize.y, smallSize.y);
            group->SetLabel("A much longer group title");
            EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).x, largeSize.x);
            group->SetLabel(PString::zero);
            EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(28, 28));
            group->SetLabel("Group");
            Ptr<PView> child = ptr_new<PView>("content");
            child->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 320.0f);
            child->SetWidthOverride(PPrefSizeType::Greatest, PSizeOverride::Always, 400.0f);
            child->SetHeightOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 80.0f);
            child->SetHeightOverride(PPrefSizeType::Greatest, PSizeOverride::Always, 100.0f);
            const PRect borders(10, 20, 30, 40);
            child->SetBorders(borders);
            group->AddChild(child);
            group->SetLayoutNode(ptr_new<PLayoutNode>());
            EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(360, 140));
            EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Greatest), PPoint(440, 160));
            ASSERT_TRUE(application.AddHandler(group));
            PScopeExit removeGroup([&application, &group] { application.RemoveHandler(group); });
            group->OnStyleChanged();
            EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(360, 140));
            EXPECT_EQ(child->GetBorders(), borders);
            group->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 50.0f);
            group->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 60.0f);
            EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50, 60));
        });
    }
}

TEST(GroupViewStyle, SkinnedPaintingPreservesBackgroundInheritanceAndDrawingState)
{
    RunGUITest([](PApplication& application)
    {
        for (const char* theme : {"Normal", "Dark"})
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            const PSkinnableGUIStyle style(directory, &application);
            Ptr<PView> parent = ptr_new<PView>("group_parent");
            const Ptr<PBrush> background = ptr_new<PBrush>(PColor(40, 60, 80));
            parent->SetBackgroundBrush(background);
            parent->SetFrame(PRect(0, 0, 400, 240));
            Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("group_paint_target", PViewFlag::WillDraw);
            parent->AddChild(target);
            parent->Hide();
            ASSERT_TRUE(application.AddView(parent, PViewDockType::TopLevelView));
            PScopeExit removeParent([&application, &parent] { application.RemoveView(parent); });
            const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(2, 3, 238, 158));
            target->SetDrawingRegion(clip);
            target->SetPenWidth(7.0f);
            target->SetDrawingMode(PDrawingMode::Overlay);
            PGroupViewStyleData& data = target->GetGroupViewStyleData();
            data.Label = "Skinned group";
            PViewRendererTestBinding<PViewRendererGroupView> renderer(*target, style, PGUIComponentID::GroupView);
            for (const PPoint& size : {PPoint(240, 160), PPoint(310, 190), PPoint(12, 9), PPoint()})
            {
                target->SetFrame(PRect::FromSize(size));
                renderer->PropertyChanged(*target, PGroupViewProperty::Frame, renderer.GetContext());
                EXPECT_NO_THROW(renderer->Paint(*target, target->GetBounds(), target->GetBounds(), renderer.GetContext()));
                EXPECT_EQ(target->GetBackgroundBrush(), nullptr);
                EXPECT_EQ(parent->GetBackgroundBrush(), background);
                EXPECT_EQ(target->GetDrawingRegion(), clip);
                EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Overlay);
                EXPECT_FLOAT_EQ(target->GetPenWidth(), 7.0f);
                EXPECT_EQ(target->GetBrush(), nullptr);
            }
            target->SetFrame(PRect(0, 0, 240, 160));
            target->ScrollTo(PPoint(-3, -7));
            PtrTarget* const cache = renderer.GetContext();
            renderer->Paint(*target, target->GetBounds(), target->GetBounds().GetResized(10, 10, -10, -10), renderer.GetContext());
            data.Label.clear();
            renderer->PropertyChanged(*target, PGroupViewProperty::Label, renderer.GetContext());
            renderer->Paint(*target, target->GetBounds(), target->GetBounds(), renderer.GetContext());
            EXPECT_EQ(renderer.GetContext(), cache);
            EXPECT_EQ(target->GetBackgroundBrush(), nullptr);
        }
        application.Sync();
    });
}

TEST(GroupViewStyle, SkinGeometryAllowsSignedSpacingAndPreservesSharedImages)
{
    RunGUITest([](PApplication&)
    {
        const Ptr<PBitmap> bitmap = ptr_new<PBitmap>(32, 32, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster().Get()), 32 * 32, 0);
        const Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(8, 8, 24, 24));
        image->SetLayoutSize(PPoint(32, 32), PImageLayoutMode::Absolute);
        const PImageDescription original = image->GetDescription();
        PGroupViewSkin skin;
        skin.Frame = image;
        skin.FrameInsets = PRect(-2, 3, 4, -5);
        skin.LabelPadding = PRect(-3, -2, 7, 6);
        skin.LabelGap = -1.0f;
        skin.MinimumSize = PPoint(70, 50);
        const Ptr<const PViewRendererGroupView> renderer = ptr_new<PViewRendererGroupViewSkinned>(skin);
        Ptr<PViewRendererTestView> first = ptr_new<PViewRendererTestView>("first_group");
        Ptr<PViewRendererTestView> second = ptr_new<PViewRendererTestView>("second_group");
        first->SetFrame(PRect(0, 0, 240, 100));
        second->SetFrame(PRect(0, 0, 180, 140));
        PGroupViewStyleData& firstData = first->GetGroupViewStyleData();
        PViewRendererTestBinding<PViewRendererGroupView> firstRenderer(*first, renderer);
        PViewRendererTestBinding<PViewRendererGroupView> secondRenderer(*second, renderer);
        PPoint minimum;
        PPoint maximum;
        firstRenderer->GetPreferredSize(*first, &minimum, &maximum, firstRenderer.GetContext());
        EXPECT_EQ(minimum, PPoint(70, 50));
        firstData.Label = "An overlapping title";
        firstRenderer->PropertyChanged(*first, PGroupViewProperty::Label, firstRenderer.GetContext());
        firstRenderer->Paint(*first, first->GetBounds(), first->GetBounds(), firstRenderer.GetContext());
        secondRenderer->Paint(*second, second->GetBounds(), second->GetBounds(), secondRenderer.GetContext());
        EXPECT_NE(firstRenderer.GetContext(), secondRenderer.GetContext());
        first->SetFrame(PRect(0, 0, 12, 9));
        firstRenderer->Paint(*first, first->GetBounds(), first->GetBounds(), firstRenderer.GetContext());
        EXPECT_EQ(image->GetDescription(), original);
    });
}

TEST(GroupViewStyle, XMLSkinAllowsSignedSpacingAndRejectsInvalidMetrics)
{
    RunGUITest([](PApplication& application)
    {
        PSkinnableGUIStyleTestDirectory directory("GroupViewMetrics");
        ASSERT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        directory.WriteFile("GroupView.xml", [](pugi::xml_node root)
        {
            root.attribute("frame_insets") = "Rect(-2, 3, 4, -5)";
            root.attribute("label_padding") = "Rect(-3, -2, 7, 6)";
            root.attribute("label_gap") = "-1";
        });
        EXPECT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        directory.WriteFile("GroupView.xml", [](pugi::xml_node root) { root.attribute("label_gap") = "nan"; });
        EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
        directory.WriteFile("GroupView.xml", [](pugi::xml_node root) { root.attribute("minimum_size") = "Point(0, 50)"; });
        EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
    });
}
