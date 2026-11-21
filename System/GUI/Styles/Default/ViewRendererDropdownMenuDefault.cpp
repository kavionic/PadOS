// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Default/ViewRendererDropdownMenuDefault.h>

#include <algorithm>
#include <cmath>

#include <GUI/View.h>
#include <GUI/Bitmap.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleDropdownMenu.h>

struct PViewRendererDropdownMenuDefault::Context : PViewRendererContext<PViewStyleDropdownMenu>
{
    using PViewRendererContext<PViewStyleDropdownMenu>::PViewRendererContext;

    Ptr<PBitmap> ArrowBitmap;
    float ItemWidth = 0.0f;
    bool AreMetricsValid = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererDropdownMenuDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuStyleData& data = state.GetTarget().GetDropdownMenuStyleData();
    if (state.ArrowBitmap == nullptr)
    {
        state.ArrowBitmap = ptr_new<PBitmap>(
            DROPDOWN_MENU_ARROW_WIDTH,
            DROPDOWN_MENU_ARROW_HEIGHT,
            PEColorSpace::MONO1,
            DROPDOWN_MENU_ARROW_BITMAP_RASTER,
            sizeof(uint32_t),
            0,
            target.GetApplication());
    }
    const PRect buttonFrame = GetDropdownMenuArrowFrame(frame);
    target.SetBackgroundBrush(PStandardColorID::DefaultBackground);
    target.DrawFrame(buttonFrame, data.IsOpen ? FRAME_RECESSED : FRAME_RAISED);
    const PPoint center = buttonFrame.Center().GetRounded();
    const PRect arrowFrame = PRect::FromSize(float(DROPDOWN_MENU_ARROW_WIDTH), float(DROPDOWN_MENU_ARROW_HEIGHT));
    target.SetDrawingMode(PDrawingMode::Overlay);
    if (data.IsEnabled)
    {
        target.SetFgColor(0, 0, 0);
        target.DrawBitmap(state.ArrowBitmap, arrowFrame, center - PPoint(9.0f, 4.0f));
    }
    else
    {
        target.SetFgColor(255, 255, 255);
        target.DrawBitmap(state.ArrowBitmap, arrowFrame, center - PPoint(8.0f, 4.0f));
        target.SetFgColor(110, 110, 110);
        target.DrawBitmap(state.ArrowBitmap, arrowFrame, center - PPoint(9.0f, 5.0f));
    }
    target.SetDrawingMode(PDrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuStyleData& data = state.GetTarget().GetDropdownMenuStyleData();
    UpdateDropdownMenuMetrics(target, data, state);
    PPoint size = data.EditorSize;
    size.x += (data.Items.empty() && data.Text != nullptr) ? target.GetStringWidth(*data.Text) : state.ItemWidth;
    size.x += DROPDOWN_MENU_LABEL_PADDING + std::round(size.y * DROPDOWN_MENU_ARROW_ASPECT_RATIO);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuDefault::PropertyChanged(PView& target, PDropdownMenuProperties properties, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Font))) {
        state.AreMetricsValid = false;
    }
    target.Invalidate();
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Text, PDropdownMenuProperty::Font))) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuDefault::GetEditorFrame(const PView& target, PtrTarget* context) const
{
    PRect frame = target.GetBounds();
    frame.right = GetDropdownMenuArrowFrame(frame).left;
    return frame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PTextBoxStyle> PViewRendererDropdownMenuDefault::GetEditorStyle(const PView& target, PtrTarget* context) const
{
    return PTextBox::GetDefaultStyle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PViewRendererDropdownMenuDefault::HitTest(const PView& target, const PPoint& position, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuStyleData& data = state.GetTarget().GetDropdownMenuStyleData();
    const PRect hitFrame = data.IsReadOnly ? target.GetBounds() : GetDropdownMenuArrowFrame(target.GetBounds());
    return data.IsEnabled && hitFrame.DoIntersect(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuDefault::UpdateDropdownMenuMetrics(const PView& target, const PDropdownMenuStyleData& data, Context& state)
{
    if (!state.AreMetricsValid)
    {
        state.ItemWidth = 0.0f;
        for (const PString& item : data.Items) {
            state.ItemWidth = std::max(state.ItemWidth, target.GetStringWidth(item));
        }
        state.AreMetricsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuDefault::GetDropdownMenuArrowFrame(const PRect& frame)
{
    const float width = std::round(std::max(0.0f, frame.Height()) * DROPDOWN_MENU_ARROW_ASPECT_RATIO);
    return PRect(std::max(frame.left, frame.right - width), frame.top, frame.right, frame.bottom);
}
