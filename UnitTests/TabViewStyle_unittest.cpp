// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>
#include <GUI/Font.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/TabView.h>
#include <GUI/Widgets/ScrollView.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

TEST(TabViewStyle, TitleAndFontChangesRefreshManagedSizes)
{
    RunGUITest([](PApplication& application)
    {
        pugi::xml_document document;
        pugi::xml_node xmlData = document.append_child("TabView");
        xmlData.append_child("_TabViewTab").append_attribute("name") = "First";
        xmlData.append_child("_TabViewTab").append_attribute("name") = "Second";
        PViewFactoryContext context;
        Ptr<PTabView> tabs = ptr_new<PTabView>(context, xmlData);
        EXPECT_EQ(tabs->GetTabCount(), 2);
        EXPECT_EQ(tabs->GetTabTitle(1), "Second");
        ASSERT_TRUE(application.AddHandler(tabs));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveHandler(tabs); });
        tabs->SetFrame(PRect(0.0f, 0.0f, 600.0f, 200.0f));
        tabs->OnStyleChanged();
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontSmall);
        tabs->SetFont(font);
        Ptr<PScrollView> topView = tabs->GetChildAt<PScrollView>(size_t(0));
        ASSERT_NE(topView, nullptr);
        Ptr<PView> tabsView = topView->GetScrolledView();
        ASSERT_NE(tabsView, nullptr);
        const PPoint smallContentSize = tabsView->GetContentSize();
        const PPoint smallSize = tabs->GetPreferredSize(PPrefSizeType::Smallest);
        const float smallClientTop = tabs->GetClientFrame().top;
        font->Set(PFontID::e_FontLarge);
        tabs->SetFont(font);
        EXPECT_GT(tabs->GetPreferredSize(PPrefSizeType::Smallest).x, smallSize.x);
        EXPECT_GT(tabs->GetClientFrame().top, smallClientTop);
        EXPECT_EQ(topView->GetFont(), font);
        EXPECT_EQ(tabsView->GetFont(), font);
        EXPECT_GT(tabsView->GetContentSize().x, smallContentSize.x);
        const PPoint largeSize = tabs->GetPreferredSize(PPrefSizeType::Smallest);
        tabs->SetTabTitle(0, "A substantially longer tab title");
        EXPECT_GT(tabs->GetPreferredSize(PPrefSizeType::Smallest).x, largeSize.x);
        tabs->SetTabTitle(0, "First");
        EXPECT_EQ(tabs->GetPreferredSize(PPrefSizeType::Smallest), largeSize);
        tabs->InsertTab(1, "Inserted");
        EXPECT_GT(tabs->GetPreferredSize(PPrefSizeType::Smallest).x, largeSize.x);
        tabs->DeleteTab(1);
        EXPECT_EQ(tabs->GetPreferredSize(PPrefSizeType::Smallest), largeSize);
    });
}

TEST(TabViewStyle, ChildAndTopBarSizesUseManagedOverrides)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        Ptr<PView> content = ptr_new<PView>("content");
        content->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 300.0f);
        content->SetWidthOverride(PPrefSizeType::Greatest, PSizeOverride::Always, 400.0f);
        content->SetHeightOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 80.0f);
        content->SetHeightOverride(PPrefSizeType::Greatest, PSizeOverride::Always, 100.0f);
        tabs->AppendTab("Tab", content);
        ASSERT_TRUE(application.AddHandler(tabs));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveHandler(tabs); });
        tabs->SetFrame(PRect(0.0f, 0.0f, 500.0f, 200.0f));
        tabs->OnStyleChanged();
        EXPECT_FLOAT_EQ(tabs->GetPreferredSize(PPrefSizeType::Smallest).x, 305.0f);
        EXPECT_FLOAT_EQ(tabs->GetPreferredSize(PPrefSizeType::Greatest).x, 405.0f);
        EXPECT_FLOAT_EQ(tabs->GetPreferredSize(PPrefSizeType::Smallest).y, 80.0f + tabs->GetClientFrame().top + 3.0f);
        EXPECT_EQ(content->GetFrame(), tabs->GetClientFrame());
        content->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 350.0f);
        EXPECT_FLOAT_EQ(tabs->GetPreferredSize(PPrefSizeType::Smallest).x, 355.0f);
        Ptr<PView> topBar = ptr_new<PView>("top_bar");
        topBar->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 60.0f);
        topBar->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 20.0f);
        tabs->SetTopBarClientView(topBar);
        EXPECT_FLOAT_EQ(topBar->Width(), 60.0f);
        topBar->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 90.0f);
        EXPECT_FLOAT_EQ(topBar->Width(), 90.0f);
        topBar->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 70.0f);
        EXPECT_GE(tabs->GetClientFrame().top, 70.0f);
        EXPECT_EQ(content->GetFrame(), tabs->GetClientFrame());
        tabs->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 120.0f);
        tabs->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 130.0f);
        tabs->SetTabTitle(0, "A long title that exceeds the explicit width");
        tabs->OnStyleChanged();
        EXPECT_EQ(tabs->GetPreferredSize(PPrefSizeType::Smallest), PPoint(120.0f, 130.0f));
        EXPECT_EQ(tabs->GetPreferredSize(PPrefSizeType::Greatest), PPoint(120.0f, 130.0f));
    });
}

TEST(TabViewStyle, OverflowHitTestingAndScrollingRespectTheTopBarClient)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("target", PViewFlag::WillDraw);
        Ptr<PView> tabsTarget = ptr_new<PView>("tabs_target", PViewFlag::WillDraw);
        target->SetFrame(PRect(0.0f, 0.0f, 220.0f, 120.0f));
        PTabViewStyleData data;
        data.Titles = {"First long tab", "Second long tab"};
        data.HasTopBarClient = true;
        data.TopBarMinSize = PPoint(60.0f, 20.0f);
        data.TopBarMaxSize = PPoint(80.0f, 20.0f);
        const PGUIStyle* style = application.GetGUIStyle();
        ASSERT_NE(style, nullptr);
        PTabViewFrames frames = style->TabViewGetFrames(*target, data);
        ASSERT_TRUE(frames.TabsFrame.IsValid());
        EXPECT_LE(frames.TabsFrame.right, frames.TopBarClientFrame.left);
        EXPECT_GE(frames.TopBarClientFrame.Width(), 60.0f);
        tabsTarget->SetFrame(PRect::FromSize(frames.TabsFrame.Size()));
        const PPoint firstTabPosition(4.0f, tabsTarget->GetBounds().Center().y);
        EXPECT_EQ(style->TabViewGetTabFromPosition(*target, *tabsTarget, data, firstTabPosition), 0u);
        EXPECT_EQ(style->TabViewGetTabFromPosition(*target, *tabsTarget, data, PPoint(-1.0f, firstTabPosition.y)), INVALID_INDEX);
        EXPECT_LE(frames.TabsFrame.bottom, frames.ClientFrame.top);
        const PPoint contentSize = style->TabViewGetTabsContentSize(*target, data);
        EXPECT_GT(contentSize.x, frames.TabsFrame.Width());
        EXPECT_FLOAT_EQ(contentSize.y, frames.TabsFrame.Height());
        tabsTarget->ScrollTo(PPoint(frames.TabsFrame.Width() - contentSize.x, 0.0f));
        const PPoint lastTabPosition(tabsTarget->GetBounds().right - 8.0f, tabsTarget->GetBounds().Center().y);
        EXPECT_EQ(style->TabViewGetTabFromPosition(*target, *tabsTarget, data, lastTabPosition), 1u);
        target->SetFrame(PRect(0.0f, 0.0f, 1200.0f, 120.0f));
        frames = style->TabViewGetFrames(*target, data);
        EXPECT_LE(style->TabViewGetTabsContentSize(*target, data).x, frames.TabsFrame.Width());
        target->SetFrame(PRect(0.0f, 0.0f, 10.0f, 5.0f));
        frames = style->TabViewGetFrames(*target, data);
        EXPECT_GE(frames.TabsFrame.Width(), 0.0f);
        EXPECT_GE(frames.ClientFrame.Width(), 0.0f);
        EXPECT_GE(frames.ClientFrame.Height(), 0.0f);
    });
}

TEST(TabViewStyle, ScrollContainerClampsAfterContentAndFrameChanges)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        tabs->AppendTab("First long tab");
        tabs->AppendTab("Second long tab");
        ASSERT_TRUE(application.AddHandler(tabs));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveHandler(tabs); });
        tabs->SetFrame(PRect(0.0f, 0.0f, 160.0f, 120.0f));
        tabs->OnStyleChanged();
        Ptr<PScrollView> topView = tabs->GetChildAt<PScrollView>(size_t(0));
        ASSERT_NE(topView, nullptr);
        Ptr<PView> tabsView = topView->GetScrolledView();
        ASSERT_NE(tabsView, nullptr);
        ASSERT_GT(tabsView->GetContentSize().x, tabsView->Width());
        EXPECT_EQ(tabsView->GetFrame(), topView->GetNormalizedBounds());
        const auto scrollToEnd = [&topView]
        {
            PPointerEvent event;
            event.WheelDelta = PPoint(-10000.0f, 0.0f);
            topView->OnPointerWheel(PMousePointerID, topView->GetBounds().Center(), event, PEventPhase::Bubble);
        };
        scrollToEnd();
        const PPoint endOffset(tabsView->Width() - tabsView->GetContentSize().x, 0.0f);
        EXPECT_EQ(tabsView->GetScrollOffset(), endOffset);
        EXPECT_EQ(topView->GetScrollOffset(), PPoint());
        EXPECT_EQ(tabs->GetScrollOffset(), PPoint());
        scrollToEnd();
        EXPECT_EQ(tabsView->GetScrollOffset(), endOffset);
        tabs->SetFrame(PRect(0.0f, 0.0f, 1200.0f, 120.0f));
        EXPECT_EQ(tabsView->GetScrollOffset(), PPoint());
        tabs->SetFrame(PRect(0.0f, 0.0f, 160.0f, 120.0f));
        Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontLarge);
        tabs->SetFont(font);
        scrollToEnd();
        font->Set(PFontID::e_FontSmall);
        tabs->SetFont(font);
        EXPECT_FLOAT_EQ(tabsView->GetScrollOffset().x, std::min(0.0f, tabsView->Width() - tabsView->GetContentSize().x));
        Ptr<PView> topBar = ptr_new<PView>("top_bar");
        topBar->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 60.0f);
        topBar->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 20.0f);
        tabs->SetTopBarClientView(topBar);
        EXPECT_LE(topView->GetFrame().right, topBar->GetFrame().left);
        scrollToEnd();
        tabs->SetTopBarClientView(nullptr);
        EXPECT_FLOAT_EQ(tabsView->GetScrollOffset().x, std::min(0.0f, tabsView->Width() - tabsView->GetContentSize().x));
        tabs->SetTabTitle(0, "A");
        tabs->SetTabTitle(1, "B");
        EXPECT_EQ(tabsView->GetScrollOffset(), PPoint());
        const float contentWidth = tabsView->GetContentSize().x;
        tabs->DeleteTab(1);
        EXPECT_LT(tabsView->GetContentSize().x, contentWidth);
    });
}

TEST(TabViewStyle, ScrollCancellationAllowsAnotherPointer)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        tabs->AppendTab("First long tab");
        tabs->AppendTab("Second long tab");
        ASSERT_TRUE(application.AddHandler(tabs));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveHandler(tabs); });
        tabs->SetFrame(PRect(0.0f, 0.0f, 160.0f, 120.0f));
        tabs->OnStyleChanged();
        Ptr<PScrollView> topView = tabs->GetChildAt<PScrollView>(size_t(0));
        ASSERT_NE(topView, nullptr);
        const PPoint position = topView->GetBounds().Center();
        const PPointerID firstPointer = GetTouchPointerID(0);
        const PPointerID secondPointer = GetTouchPointerID(1);
        topView->OnPointerDown(firstPointer, position, PPointerEvent(), PEventPhase::Capture);
        EXPECT_EQ(topView->GetInertialScrollerState(), PInertialScroller::State::WaitForThreshold);
        topView->OnPointerCancel(firstPointer, position, PPointerEvent(), PEventPhase::Capture);
        EXPECT_EQ(topView->GetInertialScrollerState(), PInertialScroller::State::Idle);
        topView->OnPointerDown(secondPointer, position, PPointerEvent(), PEventPhase::Capture);
        EXPECT_EQ(topView->GetInertialScrollerState(), PInertialScroller::State::WaitForThreshold);
        topView->OnPointerCancel(secondPointer, position, PPointerEvent(), PEventPhase::Target);
        EXPECT_EQ(topView->GetInertialScrollerState(), PInertialScroller::State::Idle);
    });
}

TEST(TabViewStyle, SelectionAndDeletionHandleSharedAndMissingContents)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        ASSERT_TRUE(application.AddHandler(tabs));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveHandler(tabs); });
        Ptr<PView> shared = ptr_new<PView>("shared");
        tabs->AppendTab("First", shared);
        tabs->AppendTab("Second", shared);
        tabs->AppendTab("Empty");
        size_t notifications = 0;
        tabs->SignalSelectionChanged.Connect([&notifications](size_t, Ptr<PView>, PTabView*) { ++notifications; });
        tabs->SetSelection(1, false);
        EXPECT_EQ(notifications, 0u);
        EXPECT_TRUE(shared->IsVisible());
        tabs->SetSelection(0);
        EXPECT_EQ(notifications, 1u);
        tabs->SetSelection(1, false);
        EXPECT_EQ(tabs->DeleteTab(0), shared);
        EXPECT_EQ(shared->GetParent(), tabs);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(tabs->GetTabTitle(0), "Second");
        EXPECT_EQ(tabs->DeleteTab(0), shared);
        EXPECT_EQ(shared->GetParent(), nullptr);
        EXPECT_TRUE(shared->IsVisible());
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(tabs->DeleteTab(0), nullptr);
        EXPECT_EQ(tabs->GetSelection(), INVALID_INDEX);
        tabs->AppendTab("No contents");
        EXPECT_EQ(tabs->DeleteTab(0), nullptr);
        EXPECT_EQ(tabs->GetSelection(), INVALID_INDEX);
    });
}

TEST(TabViewStyle, DetachedViewsRetainTabsAndHandleMissingStyle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        tabs->AppendTab("First");
        tabs->AppendTab("Second");
        tabs->SetSelection(1, false);
        tabs->SetFrame(PRect(0.0f, 0.0f, 300.0f, 150.0f));
        tabs->OnPaint(tabs->GetBounds());
        EXPECT_EQ(tabs->GetStyle(), nullptr);
        EXPECT_EQ(tabs->GetClientFrame(), tabs->GetNormalizedBounds());
        EXPECT_EQ(tabs->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
        ASSERT_TRUE(application.AddHandler(tabs));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveHandler(tabs); });
        tabs->OnStyleChanged();
        EXPECT_GT(tabs->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        EXPECT_GT(tabs->GetClientFrame().top, 0.0f);
        application.RemoveHandler(tabs);
        removeTabs.release();
        tabs->OnStyleChanged();
        EXPECT_EQ(tabs->GetStyle(), nullptr);
        EXPECT_EQ(tabs->GetTabTitle(1), "Second");
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(tabs->GetClientFrame(), tabs->GetNormalizedBounds());
    });
}

static void SendTabViewPointerEvent(
    PTabView& tabs,
    const PView& tabsView,
    PPointerEvent& event,
    PPointerEventType type,
    const PPoint& position)
{
    event.EventType = type;
    event.ScreenPosition = tabsView.ConvertToScreen(position);
    tabs.HandlePointerEvent(event);
}

TEST(TabViewStyle, TouchSelectsOnDownAndReleaseDoesNotChangeSelection)
{
    RunGUITest([](PApplication& application)
    {
        size_t notifications = 0;
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        tabs->AppendTab("First");
        tabs->AppendTab("Second");
        tabs->SetSelection(0, false);
        tabs->SetFrame(PRect(0.0f, 0.0f, 400.0f, 120.0f));
        ASSERT_TRUE(application.AddView(tabs, PViewDockType::TopLevelView));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveView(tabs); });
        ASSERT_NE(tabs->GetServerHandle(), INVALID_HANDLE);
        Ptr<PScrollView> topView = tabs->GetChildAt<PScrollView>(size_t(0));
        ASSERT_NE(topView, nullptr);
        Ptr<PView> tabsView = topView->GetScrolledView();
        ASSERT_NE(tabsView, nullptr);
        const PPoint firstPosition(4.0f, tabsView->GetBounds().Center().y);
        const PPoint secondPosition(tabsView->GetContentSize().x - 8.0f, firstPosition.y);
        PPointerEvent touch;
        touch.PointerID = GetTouchPointerID(0);
        touch.ToolType = PMotionToolType::Finger;
        tabs->SignalSelectionChanged.Connect([&notifications](size_t, Ptr<PView>, PTabView*) { ++notifications; });
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Down, secondPosition);
        ASSERT_TRUE(tabsView->HasPointerCapture(touch.PointerID));
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(notifications, 1u);
        PPointerEvent otherTouch = touch;
        otherTouch.PointerID = GetTouchPointerID(1);
        SendTabViewPointerEvent(*tabs, *tabsView, otherTouch, PPointerEventType::Up, secondPosition);
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(notifications, 1u);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Up, secondPosition);
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(notifications, 1u);
        EXPECT_FALSE(tabsView->HasPointerCapture(touch.PointerID));
        tabs->SetSelection(0, false);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Down, secondPosition);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Move, firstPosition);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Up, firstPosition);
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(notifications, 2u);
        const PPoint outsidePosition(-10.0f, firstPosition.y);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Down, firstPosition);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(notifications, 3u);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Move, outsidePosition);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Up, outsidePosition);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(notifications, 3u);
    });
}

TEST(TabViewStyle, MouseHoverDoesNotSelectAndScrollPreservesSelection)
{
    RunGUITest([](PApplication& application)
    {
        std::vector<size_t> selections;
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        tabs->AppendTab("First long tab");
        tabs->AppendTab("Second long tab");
        tabs->SetSelection(1, false);
        tabs->SetFrame(PRect(0.0f, 0.0f, 160.0f, 120.0f));
        ASSERT_TRUE(application.AddView(tabs, PViewDockType::TopLevelView));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveView(tabs); });
        ASSERT_NE(tabs->GetServerHandle(), INVALID_HANDLE);
        Ptr<PScrollView> topView = tabs->GetChildAt<PScrollView>(size_t(0));
        ASSERT_NE(topView, nullptr);
        Ptr<PView> tabsView = topView->GetScrolledView();
        ASSERT_NE(tabsView, nullptr);
        ASSERT_GT(tabsView->GetContentSize().x, tabsView->Width());
        const PPoint position(4.0f, tabsView->GetBounds().Center().y);
        PPointerEvent mouse;
        mouse.PointerID = PMousePointerID;
        mouse.ToolType = PMotionToolType::Mouse;
        mouse.SupportsHover = true;
        tabs->SignalSelectionChanged.Connect([&selections](size_t index, Ptr<PView>, PTabView*) { selections.push_back(index); });
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Move, position);
        EXPECT_EQ(tabs->GetSelection(), 1u);
        mouse.Button = PMouseButton::Left;
        mouse.Buttons = GetPointerButtonMask(PMouseButton::Left);
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Down, position);
        ASSERT_TRUE(tabsView->HasPointerCapture(mouse.PointerID));
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(selections, std::vector<size_t>({0}));
        const PPoint dragPosition = position + PPoint(topView->GetStartScrollThreshold() + 1.0f, 0.0f);
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Move, dragPosition);
        EXPECT_TRUE(topView->HasPointerCapture(mouse.PointerID));
        EXPECT_FALSE(tabsView->HasPointerCapture(mouse.PointerID));
        EXPECT_EQ(tabsView->GetScrollOffset(), PPoint());
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(selections, std::vector<size_t>({0}));
        mouse.Buttons = PPointerButtonMaskNone;
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Up, dragPosition);
        topView->ReleasePointerCapture(mouse.PointerID);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(selections, std::vector<size_t>({0}));
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Move, position);
        mouse.Buttons = GetPointerButtonMask(PMouseButton::Left);
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Down, position);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        mouse.Buttons = PPointerButtonMaskNone;
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Up, position);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(selections, std::vector<size_t>({0}));
        mouse.WheelDelta = PPoint(-0.25f, 0.0f);
        SendTabViewPointerEvent(*tabs, *tabsView, mouse, PPointerEventType::Wheel, position);
        EXPECT_LT(tabsView->GetScrollOffset().x, 0.0f);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(selections, std::vector<size_t>({0}));
    });
}

TEST(TabViewStyle, CancelAndWheelPreserveSelectionAndTabEditsCancelInteraction)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        tabs->AppendTab("First long tab");
        tabs->AppendTab("Second long tab");
        tabs->SetSelection(1, false);
        tabs->SetFrame(PRect(0.0f, 0.0f, 160.0f, 120.0f));
        ASSERT_TRUE(application.AddView(tabs, PViewDockType::TopLevelView));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveView(tabs); });
        ASSERT_NE(tabs->GetServerHandle(), INVALID_HANDLE);
        Ptr<PScrollView> topView = tabs->GetChildAt<PScrollView>(size_t(0));
        ASSERT_NE(topView, nullptr);
        Ptr<PView> tabsView = topView->GetScrolledView();
        ASSERT_NE(tabsView, nullptr);
        PPoint position(4.0f, tabsView->GetBounds().Center().y);
        PPointerEvent touch;
        touch.PointerID = GetTouchPointerID(0);
        touch.ToolType = PMotionToolType::Finger;
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Down, position);
        ASSERT_TRUE(tabsView->HasPointerCapture(touch.PointerID));
        EXPECT_EQ(tabs->GetSelection(), 0u);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Cancel, position);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Up, position);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        tabs->SetSelection(1, false);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Down, position);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        PPointerEvent wheel;
        wheel.PointerID = PMousePointerID;
        wheel.ToolType = PMotionToolType::Mouse;
        wheel.SupportsHover = true;
        wheel.WheelDelta = PPoint(-0.25f, 0.0f);
        SendTabViewPointerEvent(*tabs, *tabsView, wheel, PPointerEventType::Wheel, position);
        EXPECT_LT(tabsView->GetScrollOffset().x, 0.0f);
        EXPECT_FALSE(tabsView->HasPointerCapture(touch.PointerID));
        EXPECT_EQ(tabs->GetSelection(), 0u);
        position = tabsView->GetBounds().TopLeft() + PPoint(4.0f, tabsView->Height() * 0.5f);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Up, position);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        EXPECT_EQ(topView->GetInertialScrollerState(), PInertialScroller::State::Idle);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Down, position);
        ASSERT_TRUE(tabsView->HasPointerCapture(touch.PointerID));
        EXPECT_EQ(topView->GetInertialScrollerState(), PInertialScroller::State::WaitForThreshold);
        EXPECT_EQ(tabs->GetSelection(), 0u);
        tabs->InsertTab(0, "Inserted");
        EXPECT_EQ(tabs->GetSelection(), 1u);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Up, position);
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(tabs->GetTabTitle(tabs->GetSelection()), "First long tab");
    });
}

TEST(TabViewStyle, ScrollPreservesSelectionChangedByNotification)
{
    RunGUITest([](PApplication& application)
    {
        std::vector<size_t> selections;
        Ptr<PTabView> tabs = ptr_new<PTabView>("tabs");
        tabs->AppendTab("First long tab");
        tabs->AppendTab("Second long tab");
        tabs->AppendTab("Third long tab");
        tabs->SetSelection(2, false);
        tabs->SetFrame(PRect(0.0f, 0.0f, 160.0f, 120.0f));
        ASSERT_TRUE(application.AddView(tabs, PViewDockType::TopLevelView));
        PScopeExit removeTabs([&application, &tabs] { application.RemoveView(tabs); });
        ASSERT_NE(tabs->GetServerHandle(), INVALID_HANDLE);
        Ptr<PScrollView> topView = tabs->GetChildAt<PScrollView>(size_t(0));
        ASSERT_NE(topView, nullptr);
        Ptr<PView> tabsView = topView->GetScrolledView();
        ASSERT_NE(tabsView, nullptr);
        tabs->SignalSelectionChanged.Connect([&selections](size_t index, Ptr<PView>, PTabView* source)
        {
            selections.push_back(index);
            if (index == 0) {
                source->SetSelection(1);
            }
        });
        const PPoint position(4.0f, tabsView->GetBounds().Center().y);
        PPointerEvent touch;
        touch.PointerID = GetTouchPointerID(0);
        touch.ToolType = PMotionToolType::Finger;
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Down, position);
        ASSERT_TRUE(tabsView->HasPointerCapture(touch.PointerID));
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(selections, std::vector<size_t>({0, 1}));
        const PPoint dragPosition = position + PPoint(topView->GetStartScrollThreshold() + 1.0f, 0.0f);
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Move, dragPosition);
        EXPECT_TRUE(topView->HasPointerCapture(touch.PointerID));
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(selections, std::vector<size_t>({0, 1}));
        SendTabViewPointerEvent(*tabs, *tabsView, touch, PPointerEventType::Up, dragPosition);
        topView->ReleasePointerCapture(touch.PointerID);
        EXPECT_EQ(tabs->GetSelection(), 1u);
        EXPECT_EQ(selections, std::vector<size_t>({0, 1}));
    });
}
