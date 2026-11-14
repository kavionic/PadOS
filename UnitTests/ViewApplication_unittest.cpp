// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <GUI/View.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

class PViewApplicationTestView : public PView
{
public:
    using PView::PView;

    void OnLayoutChanged() override
    {
        ++m_LayoutCount;
        PView::OnLayoutChanged();
    }

    void DetachedFromScreen() override
    {
        ++m_DetachCount;
    }

    size_t m_LayoutCount = 0;
    size_t m_DetachCount = 0;
};

TEST(ViewApplication, DetachedViewsUseTheCurrentApplicationForStylesAndBrushes)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("detached", PViewFlag::WillDraw);
        EXPECT_EQ(view->GetLooper(), nullptr);
        EXPECT_EQ(view->GetServerHandle(), INVALID_HANDLE);
        EXPECT_FALSE(view->HasFlags(PViewFlag::IsAttachedToScreen));
        EXPECT_EQ(view->GetApplication(), &application);
        EXPECT_EQ(view->GetStyle(), application.GetGUIStyle());
        EXPECT_EQ(view->GetStandardBrush(PStandardColorID::DefaultBackground), application.GetStandardBrush(PStandardColorID::DefaultBackground));
        EXPECT_EQ(view->GetStandardBrush(PNamedColors::white), application.GetStandardBrush(PNamedColors::white));
        view->SetBrush(ptr_new<PBrush>(PColor(20, 40, 60), view->GetApplication()));
        view->SetBackgroundBrush(ptr_new<PBrush>(PColor(60, 40, 20), view->GetApplication()));
        EXPECT_EQ(view->GetBrush()->GetApplication(), &application);
        EXPECT_EQ(view->GetBackgroundBrush()->GetApplication(), &application);
    });
}

TEST(ViewApplication, OwnAndAncestorLoopersTakePrecedenceOverTheCurrentApplication)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PApplication> otherApplication = ptr_new<PApplication>("other_view_application");
        Ptr<PView> parent = ptr_new<PView>("parent");
        Ptr<PViewApplicationTestView> child = ptr_new<PViewApplicationTestView>("child");
        ASSERT_TRUE(otherApplication->AddHandler(parent));
        PScopeExit removeParent([&otherApplication, &parent] { otherApplication->RemoveHandler(parent); });
        parent->AddChild(child);
        EXPECT_EQ(parent->GetApplication(), otherApplication);
        EXPECT_EQ(child->GetApplication(), otherApplication);
        EXPECT_EQ(child->GetStyle(), otherApplication->GetGUIStyle());
        EXPECT_EQ(child->GetStandardBrush(PNamedColors::white), otherApplication->GetStandardBrush(PNamedColors::white));
        EXPECT_EQ(
            child->GetStandardBrush(PStandardColorID::DefaultBackground),
            otherApplication->GetStandardBrush(PStandardColorID::DefaultBackground));
        ASSERT_TRUE(application.AddHandler(child));
        PScopeExit removeChild([&application, &child] { application.RemoveHandler(child); });
        EXPECT_EQ(child->GetApplication(), &application);
        application.RemoveHandler(child);
        removeChild.release();
        EXPECT_EQ(child->GetApplication(), otherApplication);
        parent->RemoveChild(child);
        EXPECT_EQ(child->GetApplication(), &application);
        EXPECT_EQ(child->m_DetachCount, 0u);
    });
}

TEST(ViewApplication, DetachedChangesDoNotQueueLayoutFocusCaptureOrScreenDetach)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PViewApplicationTestView> parent = ptr_new<PViewApplicationTestView>("parent");
        Ptr<PViewApplicationTestView> child = ptr_new<PViewApplicationTestView>("child");
        ASSERT_TRUE(application.AddHandler(parent));
        PScopeExit removeParent([&application, &parent] { application.RemoveHandler(parent); });
        parent->AddChild(child);
        parent->SetFrame(PRect::FromSize(100.0f, 100.0f));
        child->SetFrame(PRect::FromSize(50.0f, 50.0f));
        parent->InvalidateLayout();
        child->InvalidateLayout();
        application.Idle();
        EXPECT_EQ(parent->m_LayoutCount, 0u);
        EXPECT_EQ(child->m_LayoutCount, 0u);
        child->SetKeyboardFocus();
        EXPECT_EQ(application.GetKeyboardFocus(), nullptr);
        EXPECT_FALSE(child->HasKeyboardFocus());
        EXPECT_FALSE(child->SetPointerCapture(GetTouchPointerID(0)));
        EXPECT_FALSE(child->HasPointerCapture(GetTouchPointerID(0)));
        parent->RemoveChild(child);
        EXPECT_EQ(child->m_DetachCount, 0u);
        const WeakPtr<PViewApplicationTestView> weakChild = child;
        child = nullptr;
        EXPECT_EQ(weakChild.Lock(), nullptr);
    });
}

TEST(ViewApplication, AttachedLayoutOnlyChildrenDoNotNeedAServerHandle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> root = ptr_new<PView>("root", PViewFlag::WillDraw);
        Ptr<PViewApplicationTestView> child = ptr_new<PViewApplicationTestView>("layout_only");
        root->SetFrame(PRect::FromSize(100.0f, 100.0f));
        root->AddChild(child);
        ASSERT_TRUE(application.AddView(root, PViewDockType::TopLevelView));
        PScopeExit removeRoot([&application, &root] { application.RemoveView(root); });
        EXPECT_TRUE(child->HasFlags(PViewFlag::IsAttachedToScreen));
        EXPECT_EQ(child->GetServerHandle(), INVALID_HANDLE);
        const size_t previousLayouts = child->m_LayoutCount;
        child->InvalidateLayout();
        application.Idle();
        EXPECT_GT(child->m_LayoutCount, previousLayouts);
        EXPECT_TRUE(child->RemoveThis());
        EXPECT_EQ(child->m_DetachCount, 1u);
        EXPECT_FALSE(child->HasFlags(PViewFlag::IsAttachedToScreen));
        EXPECT_EQ(child->GetLooper(), nullptr);
        EXPECT_EQ(child->GetApplication(), &application);
        EXPECT_EQ(child->GetStyle(), application.GetGUIStyle());
    });
}

TEST(ViewApplication, ExplicitRectangleFillsPreserveDrawingState)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("rectangle_fills", PViewFlag::WillDraw);
        view->SetFrame(PRect::FromSize(64.0f, 64.0f));
        view->SetBackgroundBrush(PNamedColors::black);
        view->SetBrush(PNamedColors::red);
        view->SetBrushOrigin(PPoint(3.0f, 5.0f));
        view->SetFgColor(PNamedColors::white);
        view->SetPenWidth(3.0f);
        const Ptr<const PBrush> drawingBrush = view->GetBrush();
        const Ptr<const PBrush> backgroundBrush = view->GetBackgroundBrush();
        const Ptr<PBrush> explicitBrush = ptr_new<PBrush>(PColor(PNamedColors::blue), &application);
        const Ptr<PBrush> noFill = ptr_new<PBrush>(PBrushStyle::None, &application);
        ASSERT_TRUE(application.AddView(view, PViewDockType::TopLevelView));
        PScopeExit removeView([&application, &view] { application.RemoveView(view); });

        const PRect rect(8.0f, 8.0f, 56.0f, 56.0f);
        view->FillRect(rect, explicitBrush);
        view->FillRect(rect, PColor(10, 20, 30));
        view->FillRect(rect, PStandardColorID::DefaultBackground);
        view->FillRect(rect, PNamedColors::green);
        view->FillRect(rect, nullptr);
        view->FillRect(rect, noFill);
        view->DrawRect(rect);
        application.Sync();

        EXPECT_EQ(view->GetBrush(), drawingBrush);
        EXPECT_EQ(view->GetBackgroundBrush(), backgroundBrush);
        EXPECT_EQ(view->GetDrawingBrushOrigin(), PPoint(3.0f, 5.0f));
        EXPECT_EQ(view->GetFgColor(), PColor(PNamedColors::white));
        EXPECT_EQ(view->GetPenWidth(), 3.0f);
    });
}
