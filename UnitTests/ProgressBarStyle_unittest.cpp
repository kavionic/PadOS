// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/ProgressBar.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

TEST(ProgressBarStyle, PreferredSizesFollowTheConstructorAndXmlOrientations)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PProgressBar> horizontal = ptr_new<PProgressBar>("horizontal");
        Ptr<PProgressBar> vertical = ptr_new<PProgressBar>("vertical", POrientation::Vertical);
        pugi::xml_document document;
        pugi::xml_node xmlData = document.append_child("ProgressBar");
        xmlData.append_attribute("orientation") = "vertical";
        PViewFactoryContext context;
        Ptr<PProgressBar> xmlVertical = ptr_new<PProgressBar>(context, xmlData);
        ASSERT_TRUE(application.AddHandler(horizontal));
        PScopeExit removeHorizontal([&application, &horizontal] { application.RemoveHandler(horizontal); });
        ASSERT_TRUE(application.AddHandler(vertical));
        PScopeExit removeVertical([&application, &vertical] { application.RemoveHandler(vertical); });
        ASSERT_TRUE(application.AddHandler(xmlVertical));
        PScopeExit removeXmlVertical([&application, &xmlVertical] { application.RemoveHandler(xmlVertical); });
        horizontal->OnStyleChanged();
        vertical->OnStyleChanged();
        xmlVertical->OnStyleChanged();
        EXPECT_EQ(horizontal->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50.0f, 24.0f));
        EXPECT_EQ(horizontal->GetPreferredSize(PPrefSizeType::Greatest), PPoint(LAYOUT_MAX_SIZE, 24.0f));
        EXPECT_EQ(vertical->GetPreferredSize(PPrefSizeType::Smallest), PPoint(24.0f, 50.0f));
        EXPECT_EQ(vertical->GetPreferredSize(PPrefSizeType::Greatest), PPoint(24.0f, LAYOUT_MAX_SIZE));
        EXPECT_EQ(xmlVertical->GetPreferredSize(PPrefSizeType::Smallest), vertical->GetPreferredSize(PPrefSizeType::Smallest));
        EXPECT_EQ(xmlVertical->GetPreferredSize(PPrefSizeType::Greatest), vertical->GetPreferredSize(PPrefSizeType::Greatest));
    });
}

TEST(ProgressBarStyle, StyleChangesPreserveProgressAndManagedSizeOverrides)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PProgressBar> progressBar = ptr_new<PProgressBar>("progress");
        ASSERT_TRUE(application.AddHandler(progressBar));
        PScopeExit removeProgressBar([&application, &progressBar] { application.RemoveHandler(progressBar); });
        progressBar->OnStyleChanged();
        progressBar->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 120.0f);
        progressBar->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 30.0f);
        progressBar->SetProgress(0.375f);
        progressBar->SetFrame(PRect(0.0f, 0.0f, 120.0f, 30.0f));
        progressBar->OnPaint(progressBar->GetBounds());
        progressBar->SetFrame(PRect(0.0f, 0.0f, 160.0f, 40.0f));
        progressBar->OnStyleChanged();
        EXPECT_FLOAT_EQ(progressBar->GetProgress(), 0.375f);
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint(120.0f, 30.0f));
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Greatest), PPoint(120.0f, 30.0f));
    });
}

TEST(ProgressBarStyle, DetachedViewsKeepUnclampedValuesAndHandleMissingStyle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PProgressBar> progressBar = ptr_new<PProgressBar>("progress");
        progressBar->SetProgress(-0.5f);
        EXPECT_FLOAT_EQ(progressBar->GetProgress(), -0.5f);
        progressBar->SetProgress(1.5f);
        progressBar->SetFrame(PRect(0.0f, 0.0f, 100.0f, 24.0f));
        progressBar->OnPaint(progressBar->GetBounds());
        EXPECT_EQ(progressBar->GetStyle(), nullptr);
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
        ASSERT_TRUE(application.AddHandler(progressBar));
        PScopeExit removeProgressBar([&application, &progressBar] { application.RemoveHandler(progressBar); });
        progressBar->OnStyleChanged();
        EXPECT_NE(progressBar->GetStyle(), nullptr);
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50.0f, 24.0f));
        EXPECT_FLOAT_EQ(progressBar->GetProgress(), 1.5f);
        application.RemoveHandler(progressBar);
        removeProgressBar.release();
        progressBar->OnStyleChanged();
        EXPECT_EQ(progressBar->GetStyle(), nullptr);
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
        EXPECT_FLOAT_EQ(progressBar->GetProgress(), 1.5f);
    });
}
