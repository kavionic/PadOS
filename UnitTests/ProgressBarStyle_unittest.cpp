// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <limits>

#include <gtest/gtest.h>

#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRendererProgressBar.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Styles/Skinned/ViewRendererProgressBarSkinned.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/ProgressBar.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"
#include "SkinnableGUIStyleTest.h"
#include "ViewRendererTest.h"

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
        horizontal->PreferredSizeChanged();
        vertical->PreferredSizeChanged();
        xmlVertical->PreferredSizeChanged();
        EXPECT_EQ(horizontal->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50.0f, 24.0f));
        EXPECT_EQ(horizontal->GetPreferredSize(PPrefSizeType::Greatest), PPoint(LAYOUT_MAX_SIZE, 24.0f));
        EXPECT_EQ(vertical->GetPreferredSize(PPrefSizeType::Smallest), PPoint(24.0f, 50.0f));
        EXPECT_EQ(vertical->GetPreferredSize(PPrefSizeType::Greatest), PPoint(24.0f, LAYOUT_MAX_SIZE));
        EXPECT_EQ(xmlVertical->GetPreferredSize(PPrefSizeType::Smallest), vertical->GetPreferredSize(PPrefSizeType::Smallest));
        EXPECT_EQ(xmlVertical->GetPreferredSize(PPrefSizeType::Greatest), vertical->GetPreferredSize(PPrefSizeType::Greatest));
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

TEST(ProgressBarStyle, DetachedViewsKeepUnclampedValuesAndApplicationStyle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PProgressBar> progressBar = ptr_new<PProgressBar>("progress");
        progressBar->SetProgress(-0.5f);
        EXPECT_FLOAT_EQ(progressBar->GetProgress(), -0.5f);
        progressBar->SetProgress(1.5f);
        progressBar->SetFrame(PRect(0.0f, 0.0f, 100.0f, 24.0f));
        progressBar->OnPaint(progressBar->GetBounds());
        EXPECT_EQ(progressBar->GetStyle(), application.GetGUIStyle());
        progressBar->PreferredSizeChanged();
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50.0f, 24.0f));
        ASSERT_TRUE(application.AddHandler(progressBar));
        PScopeExit removeProgressBar([&application, &progressBar] { application.RemoveHandler(progressBar); });
        progressBar->OnStyleChanged();
        EXPECT_NE(progressBar->GetStyle(), nullptr);
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50.0f, 24.0f));
        EXPECT_FLOAT_EQ(progressBar->GetProgress(), 1.5f);
        application.RemoveHandler(progressBar);
        removeProgressBar.release();
        progressBar->OnStyleChanged();
        EXPECT_EQ(progressBar->GetStyle(), application.GetGUIStyle());
        EXPECT_EQ(progressBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint(50.0f, 24.0f));
        EXPECT_FLOAT_EQ(progressBar->GetProgress(), 1.5f);
    });
}

TEST(ProgressBarStyle, SkinnedSizesAndStyleChangesPreserveProgressAndOverrides)
{
    for (const char* theme : {"Normal", "Dark"})
    {
        RunGUITest([theme](PApplication& application)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            application.SetGUIStyle(ptr_new<PSkinnableGUIStyle>(directory, &application));
            for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
            {
                const bool horizontal = orientation == POrientation::Horizontal;
                Ptr<PProgressBar> progress = ptr_new<PProgressBar>("skinned_progress", orientation);
                progress->PreferredSizeChanged();
                EXPECT_EQ(progress->GetPreferredSize(PPrefSizeType::Smallest), horizontal ? PPoint(50, 24) : PPoint(24, 50));
                EXPECT_EQ(progress->GetPreferredSize(PPrefSizeType::Greatest), horizontal ? PPoint(LAYOUT_MAX_SIZE, 24) : PPoint(24, LAYOUT_MAX_SIZE));
                ASSERT_TRUE(application.AddHandler(progress));
                PScopeExit removeProgress([&application, &progress] { application.RemoveHandler(progress); });
                progress->SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 120.0f);
                progress->SetHeightOverride(PPrefSizeType::All, PSizeOverride::Always, 30.0f);
                progress->SetFrame(PRect(0, 0, 120, 30));
                progress->SetProgress(0.75f);
                progress->OnPaint(progress->GetBounds());
                progress->SetProgress(0.25f);
                progress->OnPaint(progress->GetBounds());
                progress->OnStyleChanged();
                EXPECT_FLOAT_EQ(progress->GetProgress(), 0.25f);
                EXPECT_EQ(progress->GetPreferredSize(PPrefSizeType::Smallest), PPoint(120, 30));
                EXPECT_EQ(progress->GetPreferredSize(PPrefSizeType::Greatest), PPoint(120, 30));
                EXPECT_FALSE(progress->GetBackgroundBrush().IsValid());
            }
            application.SetGUIStyle(ptr_new<PDefaultGUIStyle>());
        });
    }
}

TEST(ProgressBarStyle, SkinnedPaintingRestoresBrushesOriginsModeAndClipping)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("progress_skin_paint", PViewFlag::WillDraw);
        target->SetFrame(PRect(0, 0, 220, 40));
        target->Hide();
        ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
        PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
        PBrush background(PColor(40, 60, 80));
        PBrush drawing(PColor(80, 60, 40));
        const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(2, 3, 218, 218));
        target->SetBackgroundBrush(background);
        target->SetBackgroundBrushOrigin(PPoint(5, 7));
        target->SetBrush(drawing);
        target->SetBrushOrigin(PPoint(11, 13));
        target->SetDrawingRegion(clip);
        target->SetDrawingMode(PDrawingMode::Overlay);
        target->SetPenWidth(7.0f);
        for (const char* theme : {"Normal", "Dark"})
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            const PSkinnableGUIStyle style(directory, &application);
            PProgressBarStyleData& data = target->GetProgressBarStyleData();
            data = PProgressBarStyleData();
            PViewRendererTestBinding<PViewRendererProgressBar> renderer(*target, style, PGUIComponentID::ProgressBar);
            for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
            {
                data.Orientation = orientation;
                for (const PPoint& size : {PPoint(220, 24), PPoint(24, 220), PPoint(7, 7), PPoint(3, 3), PPoint()})
                {
                    target->SetFrame(PRect::FromSize(size));
                    renderer->PropertyChanged(*target, PProgressBarProperty::Frame, renderer.GetContext());
                    for (float value : {1.5f, 1.0f, 0.75f, 0.25f, 0.001f, 0.0f, -0.5f, std::numeric_limits<float>::quiet_NaN()})
                    {
                        data.Value = value;
                        renderer->PropertyChanged(*target, PProgressBarProperty::Value, renderer.GetContext());
                        EXPECT_NO_THROW(renderer->Paint(*target, target->GetNormalizedBounds(), target->GetBounds(), renderer.GetContext()));
                        EXPECT_EQ(target->GetBackgroundBrush(), background);
                        EXPECT_EQ(target->GetBackgroundBrushOrigin(), PPoint(5, 7));
                        EXPECT_EQ(target->GetBrush(), drawing);
                        EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(11, 13));
                        EXPECT_EQ(target->GetDrawingRegion(), clip);
                        EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Overlay);
                        EXPECT_FLOAT_EQ(target->GetPenWidth(), 7.0f);
                    }
                }
            }
            target->SetFrame(PRect(0, 0, 220, 24));
            target->SetBackgroundBrush(PBrush::zero);
            data.Value = 0.5f;
            renderer->Paint(*target, target->GetNormalizedBounds(), target->GetBounds(), renderer.GetContext());
            EXPECT_FALSE(target->GetBackgroundBrush().IsValid());
            target->SetBackgroundBrush(background);
        }
        application.Sync();
    });
}

TEST(ProgressBarStyle, SkinGeometryAllowsSignedInsetsAndDoesNotResizeSharedImages)
{
    RunGUITest([](PApplication&)
    {
        const Ptr<PBitmap> bitmap = ptr_new<PBitmap>(32, 24, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster().Get()), 32 * 24, 0);
        PImage image(bitmap);
        image.SetSliceRect(PRect(8, 8, 24, 16));
        image.SetLayoutSize(PPoint(32, 24), PImageLayoutMode::Absolute);
        const PImageDescription original = image.GetDescription();
        PProgressBarSkin skin;
        skin.Horizontal.Track = image;
        skin.Horizontal.Fill = image;
        skin.Horizontal.FillInsets = PRect(-2, 3, 4, -5);
        skin.Horizontal.MinimumLength = 70.0f;
        skin.Horizontal.Thickness = 18.0f;
        skin.Vertical = skin.Horizontal;
        skin.Vertical.MinimumLength = 90.0f;
        skin.Vertical.Thickness = 22.0f;
        const Ptr<const PViewRendererProgressBar> renderer = ptr_new<PViewRendererProgressBarSkinned>(skin);
        Ptr<PViewRendererTestView> first = ptr_new<PViewRendererTestView>("first_progress");
        Ptr<PViewRendererTestView> second = ptr_new<PViewRendererTestView>("second_progress");
        PProgressBarStyleData& firstData = first->GetProgressBarStyleData();
        PProgressBarStyleData& secondData = second->GetProgressBarStyleData();
        PViewRendererTestBinding<PViewRendererProgressBar> firstRenderer(*first, renderer);
        PViewRendererTestBinding<PViewRendererProgressBar> secondRenderer(*second, renderer);
        PPoint minimum;
        PPoint maximum;
        firstRenderer->GetPreferredSize(*first, &minimum, &maximum, firstRenderer.GetContext());
        EXPECT_EQ(minimum, PPoint(70, 18));
        EXPECT_EQ(maximum, PPoint(LAYOUT_MAX_SIZE, 18));
        secondData.Orientation = POrientation::Vertical;
        secondRenderer->GetPreferredSize(*second, &minimum, &maximum, secondRenderer.GetContext());
        EXPECT_EQ(minimum, PPoint(22, 90));
        EXPECT_EQ(maximum, PPoint(22, LAYOUT_MAX_SIZE));
        first->SetFrame(PRect(0, 0, 120, 24));
        firstData.Value = 0.001f;
        firstRenderer->Paint(*first, first->GetNormalizedBounds(), first->GetBounds(), firstRenderer.GetContext());
        PtrTarget* const firstCache = firstRenderer.GetContext();
        second->SetFrame(PRect(0, 0, 24, 180));
        secondData.Value = 0.75f;
        secondRenderer->Paint(*second, second->GetNormalizedBounds(), second->GetBounds(), secondRenderer.GetContext());
        EXPECT_NE(firstRenderer.GetContext(), secondRenderer.GetContext());
        first->SetFrame(second->GetFrame());
        firstData.Value = 1.0f;
        firstRenderer->Paint(*first, first->GetNormalizedBounds(), first->GetBounds(), firstRenderer.GetContext());
        EXPECT_EQ(firstRenderer.GetContext(), firstCache);
        EXPECT_EQ(image.GetDescription(), original);
    });
}

TEST(ProgressBarStyle, XMLSkinAllowsSignedInsetsAndRejectsIncompleteImagesAndInvalidMetrics)
{
    RunGUITest([](PApplication& application)
    {
        PSkinnableGUIStyleTestDirectory directory("ProgressBarMetrics");
        ASSERT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        directory.WriteFile("ProgressBar.xml", [](pugi::xml_node root)
        {
            root.child("Horizontal").attribute("fill_insets") = "Rect(-2, 3, 4, -5)";
            root.child("Vertical").attribute("fill_insets") = "Rect(-2, 3, 4, -5)";
        });
        EXPECT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        directory.WriteFile("ProgressBar.xml", [](pugi::xml_node root) { root.child("Vertical").remove_child("Fill"); });
        EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
        directory.WriteFile("ProgressBar.xml", [](pugi::xml_node root) { root.child("Horizontal").attribute("fill_insets") = "Rect(nan, 3, 4, -5)"; });
        EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
        directory.WriteFile("ProgressBar.xml", [](pugi::xml_node root) { root.child("Vertical").attribute("thickness") = "0"; });
        EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
    });
}
