// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////


#include <gtest/gtest.h>
#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Widgets/ScrollBar.h>

#include "GUITest.h"

TEST(ScrollBarStyle, PreferredSizeUsesTheOrientation)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("scrollbar_style_target");
        PScrollBarStyleData data;
        PPoint minimumSize;
        PPoint maximumSize;
        style.ScrollBarGetPreferredSize(*target, data, &minimumSize, &maximumSize);
        EXPECT_EQ(minimumSize, PPoint(16.0f, 0.0f));
        EXPECT_EQ(maximumSize, PPoint(16.0f, COORD_MAX));
        data.Orientation = POrientation::Horizontal;
        style.ScrollBarGetPreferredSize(*target, data, &minimumSize, &maximumSize);
        EXPECT_EQ(minimumSize, PPoint(0.0f, 16.0f));
        EXPECT_EQ(maximumSize, PPoint(COORD_MAX, 16.0f));
    });
}

TEST(ScrollBarStyle, DragMappingMatchesMinimumKnobGeometry)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("scrollbar_style_target");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool isHorizontal = orientation == POrientation::Horizontal;
            target->SetFrame(isHorizontal ? PRect(0.0f, 0.0f, 200.0f, 16.0f) : PRect(0.0f, 0.0f, 16.0f, 200.0f));
            PScrollBarStyleData data;
            data.Orientation = orientation;
            data.Proportion = 0.001f;
            for (float value : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
            {
                data.Value = value;
                const PPoint position = style.ScrollBarGetValuePosition(*target, data, value);
                EXPECT_NEAR(style.ScrollBarGetValueFromPosition(*target, data, position), value, 0.00001f);
                EXPECT_EQ(style.ScrollBarHitTest(*target, data, position), PScrollBarPart::Knob);
            }
            const PPoint minimumPosition = style.ScrollBarGetValuePosition(*target, data, 0.0f);
            const PPoint maximumPosition = style.ScrollBarGetValuePosition(*target, data, 1.0f);
            EXPECT_EQ(style.ScrollBarGetValueFromPosition(*target, data, minimumPosition - PPoint(50.0f, 50.0f)), 0.0f);
            EXPECT_EQ(style.ScrollBarGetValueFromPosition(*target, data, maximumPosition + PPoint(50.0f, 50.0f)), 1.0f);

            data.Value = 0.5f;
            EXPECT_EQ(style.ScrollBarHitTest(*target, data, minimumPosition + PPoint(1.0f, 1.0f)), PScrollBarPart::PageDecrement);
            EXPECT_EQ(style.ScrollBarHitTest(*target, data, maximumPosition + PPoint(1.0f, 1.0f)), PScrollBarPart::PageIncrement);
        }
    });
}

TEST(ScrollBarStyle, ValueMappingUsesKnobCenters)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("scrollbar_style_target");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool isHorizontal = orientation == POrientation::Horizontal;
            target->SetFrame(isHorizontal ? PRect(0.0f, 0.0f, 200.0f, 16.0f) : PRect(0.0f, 0.0f, 16.0f, 200.0f));
            const PPoint trackCenter = isHorizontal ? PPoint(100.0f, 8.0f) : PPoint(8.0f, 100.0f);
            PScrollBarStyleData data;
            data.Orientation = orientation;
            data.Value = 0.25f;
            for (float proportion : {0.001f, 0.25f, 0.75f})
            {
                data.Proportion = proportion;
                EXPECT_EQ(style.ScrollBarGetValuePosition(*target, data, 0.5f), trackCenter);
                EXPECT_FLOAT_EQ(style.ScrollBarGetValueFromPosition(*target, data, trackCenter), 0.5f);
            }
        }
    });
}

TEST(ScrollBarStyle, HitTestingDistinguishesArrowsAndHandlesShortFrames)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("scrollbar_style_target");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool isHorizontal = orientation == POrientation::Horizontal;
            PScrollBarStyleData data;
            data.Orientation = orientation;
            target->SetFrame(isHorizontal ? PRect(0.0f, 0.0f, 200.0f, 16.0f) : PRect(0.0f, 0.0f, 16.0f, 200.0f));
            struct ArrowHit
            {
                float Position;
                PScrollBarPart Part;
            };
            const ArrowHit arrows[] =
            {
                {6.0f, PScrollBarPart::DecrementStart},
                {18.0f, PScrollBarPart::IncrementStart},
                {182.0f, PScrollBarPart::DecrementEnd},
                {194.0f, PScrollBarPart::IncrementEnd}
            };
            for (const ArrowHit& arrow : arrows)
            {
                const PPoint position = isHorizontal ? PPoint(arrow.Position, 8.0f) : PPoint(8.0f, arrow.Position);
                EXPECT_EQ(style.ScrollBarHitTest(*target, data, position), arrow.Part);
            }
            data.IsEnabled = false;
            EXPECT_EQ(style.ScrollBarHitTest(*target, data, PPoint(6.0f, 6.0f)), PScrollBarPart::None);
            data.IsEnabled = true;
            target->SetFrame(isHorizontal ? PRect(0.0f, 0.0f, 10.0f, 16.0f) : PRect(0.0f, 0.0f, 16.0f, 10.0f));
            const PPoint startPosition = isHorizontal ? PPoint(2.0f, 8.0f) : PPoint(8.0f, 2.0f);
            const PPoint endPosition(8.0f, 8.0f);
            EXPECT_EQ(style.ScrollBarHitTest(*target, data, startPosition), PScrollBarPart::DecrementStart);
            EXPECT_EQ(style.ScrollBarHitTest(*target, data, endPosition), PScrollBarPart::IncrementEnd);
            target->SetFrame(PRect());
            EXPECT_EQ(style.ScrollBarHitTest(*target, data, PPoint()), PScrollBarPart::None);
            EXPECT_FLOAT_EQ(style.ScrollBarGetValueFromPosition(*target, data, PPoint(50.0f, 50.0f)), data.Value);
        }
    });
}

TEST(ScrollBarStyle, FullKnobsHaveNoDragTravel)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PView> target = ptr_new<PView>("scrollbar_style_target");
        target->SetFrame(PRect(0.0f, 0.0f, 16.0f, 200.0f));
        PScrollBarStyleData data;
        data.Value = 0.5f;
        data.Proportion = 1.0f;
        EXPECT_EQ(style.ScrollBarGetValuePosition(*target, data, 0.0f), style.ScrollBarGetValuePosition(*target, data, 1.0f));
        EXPECT_FLOAT_EQ(style.ScrollBarGetValueFromPosition(*target, data, PPoint(0.0f, 200.0f)), data.Value);
        data.Proportion = 0.1f;
        data.HasRange = false;
        EXPECT_EQ(style.ScrollBarGetValuePosition(*target, data, 0.0f), style.ScrollBarGetValuePosition(*target, data, 1.0f));
        EXPECT_FLOAT_EQ(style.ScrollBarGetValueFromPosition(*target, data, PPoint(0.0f, 200.0f)), data.Value);
    });
}

TEST(ScrollBarStyle, DetachedScrollBarsHandleMissingStyle)
{
    RunGUITest([](PApplication&)
    {
        Ptr<PScrollBar> scrollBar = ptr_new<PScrollBar>("scrollbar", 10.0f, 30.0f);
        scrollBar->SetFrame(PRect(0.0f, 0.0f, 16.0f, 200.0f));
        scrollBar->SetValue(20.0f);
        scrollBar->SetProportion(0.25f);
        scrollBar->OnStyleChanged();
        EXPECT_EQ(scrollBar->GetStyle(), nullptr);
        EXPECT_EQ(scrollBar->GetPreferredSize(PPrefSizeType::Smallest), PPoint());
        EXPECT_FLOAT_EQ(scrollBar->GetValue(), 20.0f);
        EXPECT_FLOAT_EQ(scrollBar->GetProportion(), 0.25f);
        const PPointerEvent event;
        scrollBar->OnPointerDown(1, PPoint(8.0f, 6.0f), event, PEventPhase::Target);
        scrollBar->SlotTimerTick();
        scrollBar->OnPointerUp(1, PPoint(8.0f, 6.0f), event, PEventPhase::Target);
        EXPECT_FLOAT_EQ(scrollBar->GetValue(), 20.0f);
    });
}
