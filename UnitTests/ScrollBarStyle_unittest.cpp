// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////


#include <gtest/gtest.h>
#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/Default/ViewRendererScrollBarDefault.h>
#include <GUI/Widgets/ScrollBar.h>

#include "GUITest.h"
#include "ViewRendererTest.h"
#include <GUI/Style/ViewRendererScrollBar.h>
#include <GUI/Style/ViewStyleScrollBar.h>

TEST(ScrollBarStyle, PreferredSizeUsesTheOrientation)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("scrollbar_style_target");
        PScrollBarStyleData& data = target->GetScrollBarStyleData();
        PViewRendererTestBinding<PViewRendererScrollBar> renderer(*target, style, PGUIComponentID::ScrollBar);
        PPoint minimumSize;
        PPoint maximumSize;
        renderer->GetPreferredSize(*target, &minimumSize, &maximumSize, renderer.GetContext());
        EXPECT_EQ(minimumSize, PPoint(16.0f, 0.0f));
        EXPECT_EQ(maximumSize, PPoint(16.0f, COORD_MAX));
        data.Orientation = POrientation::Horizontal;
        renderer->GetPreferredSize(*target, &minimumSize, &maximumSize, renderer.GetContext());
        EXPECT_EQ(minimumSize, PPoint(0.0f, 16.0f));
        EXPECT_EQ(maximumSize, PPoint(COORD_MAX, 16.0f));
    });
}

TEST(ScrollBarStyle, DragMappingMatchesMinimumKnobGeometry)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("scrollbar_style_target");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool isHorizontal = orientation == POrientation::Horizontal;
            target->SetFrame(isHorizontal ? PRect(0.0f, 0.0f, 200.0f, 16.0f) : PRect(0.0f, 0.0f, 16.0f, 200.0f));
            PScrollBarStyleData& data = target->GetScrollBarStyleData();
            PViewRendererTestBinding<PViewRendererScrollBar> renderer(*target, style, PGUIComponentID::ScrollBar);
            data.Orientation = orientation;
            data.Proportion = 0.001f;
            for (float value : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
            {
                data.Value = value;
                const PPoint position = renderer->GetValuePosition(*target, value, renderer.GetContext());
                EXPECT_NEAR(renderer->GetValueFromPosition(*target, position, renderer.GetContext()), value, 0.00001f);
                EXPECT_EQ(renderer->HitTest(*target, position, renderer.GetContext()), PScrollBarPart::Knob);
            }
            const PPoint minimumPosition = renderer->GetValuePosition(*target, 0.0f, renderer.GetContext());
            const PPoint maximumPosition = renderer->GetValuePosition(*target, 1.0f, renderer.GetContext());
            EXPECT_EQ(renderer->GetValueFromPosition(*target, minimumPosition - PPoint(50.0f, 50.0f), renderer.GetContext()), 0.0f);
            EXPECT_EQ(renderer->GetValueFromPosition(*target, maximumPosition + PPoint(50.0f, 50.0f), renderer.GetContext()), 1.0f);

            data.Value = 0.5f;
            EXPECT_EQ(renderer->HitTest(*target, minimumPosition + PPoint(1.0f, 1.0f), renderer.GetContext()), PScrollBarPart::PageDecrement);
            EXPECT_EQ(renderer->HitTest(*target, maximumPosition + PPoint(1.0f, 1.0f), renderer.GetContext()), PScrollBarPart::PageIncrement);
        }
    });
}

TEST(ScrollBarStyle, ValueMappingUsesKnobCenters)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("scrollbar_style_target");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool isHorizontal = orientation == POrientation::Horizontal;
            target->SetFrame(isHorizontal ? PRect(0.0f, 0.0f, 200.0f, 16.0f) : PRect(0.0f, 0.0f, 16.0f, 200.0f));
            const PPoint trackCenter = isHorizontal ? PPoint(100.0f, 8.0f) : PPoint(8.0f, 100.0f);
            PScrollBarStyleData& data = target->GetScrollBarStyleData();
            PViewRendererTestBinding<PViewRendererScrollBar> renderer(*target, style, PGUIComponentID::ScrollBar);
            data.Orientation = orientation;
            data.Value = 0.25f;
            for (float proportion : {0.001f, 0.25f, 0.75f})
            {
                data.Proportion = proportion;
                EXPECT_EQ(renderer->GetValuePosition(*target, 0.5f, renderer.GetContext()), trackCenter);
                EXPECT_FLOAT_EQ(renderer->GetValueFromPosition(*target, trackCenter, renderer.GetContext()), 0.5f);
            }
        }
    });
}

TEST(ScrollBarStyle, HitTestingDistinguishesArrowsAndHandlesShortFrames)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("scrollbar_style_target");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool isHorizontal = orientation == POrientation::Horizontal;
            PScrollBarStyleData& data = target->GetScrollBarStyleData();
            PViewRendererTestBinding<PViewRendererScrollBar> renderer(*target, style, PGUIComponentID::ScrollBar);
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
                EXPECT_EQ(renderer->HitTest(*target, position, renderer.GetContext()), arrow.Part);
            }
            data.IsEnabled = false;
            EXPECT_EQ(renderer->HitTest(*target, PPoint(6.0f, 6.0f), renderer.GetContext()), PScrollBarPart::None);
            data.IsEnabled = true;
            target->SetFrame(isHorizontal ? PRect(0.0f, 0.0f, 10.0f, 16.0f) : PRect(0.0f, 0.0f, 16.0f, 10.0f));
            const PPoint startPosition = isHorizontal ? PPoint(2.0f, 8.0f) : PPoint(8.0f, 2.0f);
            const PPoint endPosition(8.0f, 8.0f);
            EXPECT_EQ(renderer->HitTest(*target, startPosition, renderer.GetContext()), PScrollBarPart::DecrementStart);
            EXPECT_EQ(renderer->HitTest(*target, endPosition, renderer.GetContext()), PScrollBarPart::IncrementEnd);
            target->SetFrame(PRect());
            EXPECT_EQ(renderer->HitTest(*target, PPoint(), renderer.GetContext()), PScrollBarPart::None);
            EXPECT_FLOAT_EQ(renderer->GetValueFromPosition(*target, PPoint(50.0f, 50.0f), renderer.GetContext()), data.Value);
        }
    });
}

TEST(ScrollBarStyle, FullKnobsHaveNoDragTravel)
{
    RunGUITest([](PApplication&)
    {
        const PDefaultGUIStyle style;
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("scrollbar_style_target");
        target->SetFrame(PRect(0.0f, 0.0f, 16.0f, 200.0f));
        PScrollBarStyleData& data = target->GetScrollBarStyleData();
        PViewRendererTestBinding<PViewRendererScrollBar> renderer(*target, style, PGUIComponentID::ScrollBar);
        data.Value = 0.5f;
        data.Proportion = 1.0f;
        EXPECT_EQ(renderer->GetValuePosition(*target, 0.0f, renderer.GetContext()), renderer->GetValuePosition(*target, 1.0f, renderer.GetContext()));
        EXPECT_FLOAT_EQ(renderer->GetValueFromPosition(*target, PPoint(0.0f, 200.0f), renderer.GetContext()), data.Value);
        data.Proportion = 0.1f;
        data.HasRange = false;
        EXPECT_EQ(renderer->GetValuePosition(*target, 0.0f, renderer.GetContext()), renderer->GetValuePosition(*target, 1.0f, renderer.GetContext()));
        EXPECT_FLOAT_EQ(renderer->GetValueFromPosition(*target, PPoint(0.0f, 200.0f), renderer.GetContext()), data.Value);
    });
}

TEST(ScrollBarStyle, DetachedScrollBarsUseTheApplicationStyleWithoutCapturingInput)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PScrollBar> scrollBar = ptr_new<PScrollBar>("scrollbar", 10.0f, 30.0f);
        scrollBar->SetFrame(PRect(0.0f, 0.0f, 16.0f, 200.0f));
        scrollBar->SetValue(20.0f);
        scrollBar->SetProportion(0.25f);
        scrollBar->OnStyleChanged();
        EXPECT_EQ(scrollBar->GetStyle(), application.GetGUIStyle());
        EXPECT_GT(scrollBar->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        EXPECT_FLOAT_EQ(scrollBar->GetPreferredSize(PPrefSizeType::Smallest).y, 0.0f);
        EXPECT_FLOAT_EQ(scrollBar->GetValue(), 20.0f);
        EXPECT_FLOAT_EQ(scrollBar->GetProportion(), 0.25f);
        const PPointerEvent event;
        scrollBar->OnPointerDown(1, PPoint(8.0f, 6.0f), event, PEventPhase::Target);
        scrollBar->SlotTimerTick();
        scrollBar->OnPointerUp(1, PPoint(8.0f, 6.0f), event, PEventPhase::Target);
        EXPECT_FLOAT_EQ(scrollBar->GetValue(), 20.0f);
    });
}

struct PScrollBarHoverTestState : PtrTarget
{
    PScrollBarPart HoveredPart = PScrollBarPart::None;
};

class PScrollBarHoverTestRenderer : public PViewRendererScrollBarDefault
{
public:
    explicit PScrollBarHoverTestRenderer(Ptr<PScrollBarHoverTestState> state) : m_State(std::move(state)) {}

    virtual void PropertyChanged(PView& target, PScrollBarProperties properties, PtrTarget* context) const override
    {
        m_State->HoveredPart = dynamic_cast<const PViewStyleScrollBar&>(target).GetScrollBarStyleData().HoveredPart;
        PViewRendererScrollBarDefault::PropertyChanged(target, properties, context);
    }

private:
    const Ptr<PScrollBarHoverTestState> m_State;
};

class PScrollBarHoverTestStyle : public PDefaultGUIStyle
{
public:
    PScrollBarHoverTestStyle() : m_State(ptr_new<PScrollBarHoverTestState>())
    {
        SetRenderer(PGUIComponentID::ScrollBar, ptr_new<PScrollBarHoverTestRenderer>(m_State));
    }

    PScrollBarPart GetHoveredPart() const { return m_State->HoveredPart; }

private:
    const Ptr<PScrollBarHoverTestState> m_State;
};

TEST(ScrollBarStyle, HoverTracksPartsAndIgnoresTouchAndBubblingEvents)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PScrollBarHoverTestStyle> style = ptr_new<PScrollBarHoverTestStyle>();
        application.SetGUIStyle(style);
        Ptr<PScrollBar> scrollBar = ptr_new<PScrollBar>("hover_scrollbar", 0.0f, 100.0f, POrientation::Horizontal);
        scrollBar->SetFrame(PRect::FromSize(200, 16));
        PPointerEvent event;
        event.SupportsHover = true;
        scrollBar->OnPointerOver(1, PPoint(6, 8), event, PEventPhase::Target);
        EXPECT_EQ(style->GetHoveredPart(), PScrollBarPart::DecrementStart);
        scrollBar->OnPointerMove(1, PPoint(194, 8), event, PEventPhase::Target);
        EXPECT_EQ(style->GetHoveredPart(), PScrollBarPart::IncrementEnd);
        scrollBar->OnPointerOut(1, PPoint(201, 8), event, PEventPhase::Target);
        EXPECT_EQ(style->GetHoveredPart(), PScrollBarPart::None);
        event.SupportsHover = false;
        scrollBar->OnPointerOver(1, PPoint(6, 8), event, PEventPhase::Target);
        scrollBar->OnPointerMove(1, PPoint(194, 8), event, PEventPhase::Target);
        EXPECT_EQ(style->GetHoveredPart(), PScrollBarPart::None);
        event.SupportsHover = true;
        scrollBar->OnPointerOver(1, PPoint(6, 8), event, PEventPhase::Bubble);
        EXPECT_EQ(style->GetHoveredPart(), PScrollBarPart::None);
    });
}
