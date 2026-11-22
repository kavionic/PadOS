// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/View.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/TextView.h>
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

TEST(ViewApplication, ViewsStartWithoutDrawingOrBackgroundBrushes)
{
    RunGUITest([](PApplication&)
    {
        Ptr<PView> view = ptr_new<PView>("default_brushes");
        pugi::xml_document document;
        ASSERT_TRUE(document.load_string("<View name='xml_brushes'/>"));
        PViewFactoryContext context;
        Ptr<PView> xmlView = ptr_new<PView>(context, document.first_child());
        Ptr<PView> proxy = ptr_new<PView>(INVALID_HANDLE, "proxy_brushes", PRect(0, 0, 32, 32));
        for (const Ptr<PView>& target : {view, xmlView, proxy})
        {
            EXPECT_EQ(target->GetBackgroundBrush(), nullptr);
            EXPECT_EQ(target->GetBrush(), nullptr);
        }
    });
}

TEST(ViewApplication, TextViewsPreserveTheirBackgroundWhenADrawingBrushIsSelected)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<const PBrush> background = application.GetStandardBrush(PNamedColors::blue);
        const Ptr<const PBrush> drawing = application.GetStandardBrush(PNamedColors::red);
        for (PTextViewFlags flags : {PTextViewFlags::zero, PTextViewFlags(PTextViewFlag::MultiLine)})
        {
            Ptr<PTextView> view = ptr_new<PTextView>("text_background", "Text on its own background", flags);
            view->SetFrame(PRect(0, 0, 100, 80));
            view->SetBackgroundBrush(background);
            view->SetBackgroundBrushOrigin(PPoint(3, 5));
            view->SetBrush(drawing);
            view->SetBrushOrigin(PPoint(7, 9));
            view->OnPaint(view->GetBounds());
            EXPECT_EQ(view->GetBackgroundBrush(), background);
            EXPECT_EQ(view->GetBackgroundBrushOrigin(), PPoint(3, 5));
            EXPECT_EQ(view->GetBrush(), drawing);
            EXPECT_EQ(view->GetDrawingBrushOrigin(), PPoint(7, 9));
        }
    });
}

TEST(ViewApplication, TextBackgroundOverridesPreserveSelectedBrushes)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("text_overrides", PViewFlag::WillDraw);
        view->SetFrame(PRect(0, 0, 120, 60));
        const Ptr<const PBrush> background = application.GetStandardBrush(PNamedColors::blue);
        const Ptr<const PBrush> drawing = application.GetStandardBrush(PNamedColors::red);
        const Ptr<const PBrush> overrideBrush = application.GetStandardBrush(PNamedColors::green);
        view->SetBackgroundBrush(background);
        view->SetBrush(drawing);
        view->SetBackgroundBrushOrigin(PPoint(3, 5));
        view->SetBrushOrigin(PPoint(7, 9));
        view->Hide();
        ASSERT_TRUE(application.AddView(view, PViewDockType::TopLevelView));
        PScopeExit removeView([&application, &view] { application.RemoveView(view); });
        const PPoint baseline(2, view->GetFontHeight().ascender);
        for (const Ptr<const PBrush>& brush : {Ptr<const PBrush>(), overrideBrush})
        {
            view->MovePenTo(baseline);
            view->DrawString(brush, "Text");
            view->DrawString(brush, "Text", baseline);
            view->MovePenTo(baseline);
            view->DrawString(brush, "Text", 100.0f, false);
            view->DrawString(brush, "Text", baseline, 100.0f, false);
            view->DrawString(brush, "Text", baseline, view->GetBounds(), false);
        }
        view->DrawString("Text", baseline, view->GetBounds());
        application.Sync();
        EXPECT_EQ(view->GetBackgroundBrush(), background);
        EXPECT_EQ(view->GetBrush(), drawing);
        EXPECT_EQ(view->GetBackgroundBrushOrigin(), PPoint(3, 5));
        EXPECT_EQ(view->GetDrawingBrushOrigin(), PPoint(7, 9));
    });
}

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
        EXPECT_EQ(view->GetStyle()->GetBrush(PGUIColorID::Background)->GetApplication(), &application);
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
        EXPECT_EQ(otherApplication->GetGUIStyle()->GetBrush(PGUIColorID::Background)->GetApplication(), otherApplication);
        otherApplication->SetGUIStyle(nullptr);
        EXPECT_EQ(otherApplication->GetGUIStyle()->GetBrush(PGUIColorID::Background)->GetApplication(), otherApplication);
        Ptr<PView> parent = ptr_new<PView>("parent");
        Ptr<PViewApplicationTestView> child = ptr_new<PViewApplicationTestView>("child");
        ASSERT_TRUE(otherApplication->AddHandler(parent));
        PScopeExit removeParent([&otherApplication, &parent] { otherApplication->RemoveHandler(parent); });
        parent->AddChild(child);
        EXPECT_EQ(parent->GetApplication(), otherApplication);
        EXPECT_EQ(child->GetApplication(), otherApplication);
        EXPECT_EQ(child->GetStyle(), otherApplication->GetGUIStyle());
        EXPECT_EQ(child->GetStyle()->GetBrush(PGUIColorID::Background)->GetApplication(), otherApplication);
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
        root->Hide();
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
        view->Hide();
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

TEST(ViewApplication, DrawingRegionsPreserveSnapshotsAcrossAttachmentAndScrolling)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("drawing_regions", PViewFlag::WillDraw);
        view->SetFrame(PRect::FromSize(64.0f, 64.0f));
        EXPECT_EQ(view->GetDrawingRegion(), nullptr);
        PRegion source(PIRect(4, 6, 40, 50));
        view->SetDrawingRegion(source);
        const Ptr<const PRegion> saved = view->GetDrawingRegion();
        ASSERT_NE(saved, nullptr);
        source.Clear();
        ASSERT_EQ(saved->m_Rects.size(), 1u);
        EXPECT_EQ(saved->m_Rects[0], PIRect(4, 6, 40, 50));
        view->Hide();
        ASSERT_TRUE(application.AddView(view, PViewDockType::TopLevelView));
        PScopeExit removeView([&application, &view] { application.RemoveView(view); });
        view->ScrollBy(PPoint(-7, -9));
        EXPECT_EQ(view->GetDrawingRegion(), saved);
        EXPECT_EQ(saved->m_Rects[0], PIRect(4, 6, 40, 50));
        view->FillRect(view->GetBounds(), PNamedColors::red);
        view->SetDrawingRegion(PRegion());
        ASSERT_NE(view->GetDrawingRegion(), nullptr);
        EXPECT_TRUE(view->GetDrawingRegion()->IsEmpty());
        view->FillRect(view->GetBounds(), PNamedColors::green);
        view->ClearDrawingRegion();
        EXPECT_EQ(view->GetDrawingRegion(), nullptr);
        view->FillRect(view->GetBounds(), PNamedColors::blue);
        view->SetDrawingRegion(saved);
        EXPECT_EQ(view->GetDrawingRegion(), saved);
        application.Sync();
    });
}
