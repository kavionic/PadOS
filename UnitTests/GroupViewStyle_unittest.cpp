// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <GUI/Font.h>
#include <GUI/LayoutNode.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/GroupView.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

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
        EXPECT_EQ(group->GetStyle(), nullptr);
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

TEST(GroupViewStyle, DetachedViewsRetainTheirLabelAndHandleMissingStyle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PGroupView> group = ptr_new<PGroupView>("group");
        group->SetLabel("Detached group");
        group->SetFrame(PRect(0.0f, 0.0f, 200.0f, 100.0f));
        group->OnPaint(group->GetBounds());
        EXPECT_EQ(group->GetStyle(), nullptr);
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
        ASSERT_TRUE(application.AddHandler(group));
        group->OnStyleChanged();
        EXPECT_GT(group->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        application.RemoveHandler(group);
        group->OnStyleChanged();
        EXPECT_EQ(group->GetStyle(), nullptr);
        EXPECT_EQ(group->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
        EXPECT_EQ(group->GetLabel(), "Detached group");
    });
}

TEST(GroupViewStyle, PaintingRestoresThePenWidth)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("group_paint_target");
        target->SetFrame(PRect(0.0f, 0.0f, 200.0f, 100.0f));
        target->SetPenWidth(7.0f);
        PGroupViewStyleData data;
        const PGUIStyle* style = application.GetGUIStyle();
        ASSERT_NE(style, nullptr);
        style->GroupViewPaint(*target, target->GetBounds(), data);
        EXPECT_EQ(target->GetPenWidth(), 7.0f);
        data.Label = "Group";
        style->GroupViewPropertyChanged(*target, data, PGroupViewProperty::Label);
        style->GroupViewPaint(*target, target->GetBounds(), data);
        EXPECT_EQ(target->GetPenWidth(), 7.0f);
    });
}
