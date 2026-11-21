// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleDropdownMenuPopup.h>
#include <GUI/Styles/Default/ViewRendererDropdownMenuPopupDefault.h>
#include <GUI/View.h>

struct PViewRendererDropdownMenuPopupDefault::Context : PViewRendererContext<PViewStyleDropdownMenuPopup>
{
    using PViewRendererContext<PViewStyleDropdownMenuPopup>::PViewRendererContext;

    PPoint ContentSize;
    PFontHeight FontHeight;
    float ItemHeight = 0.0f;
    size_t PreviousSelection = INVALID_INDEX;
    size_t PreviousPressedItem = INVALID_INDEX;
    bool AreMetricsValid = false;
    bool HasVisualState = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererDropdownMenuPopupDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuPopupStyleData& data = state.GetTarget().GetDropdownMenuPopupStyleData();
    UpdateDropdownMenuPopupMetrics(target, data, state);
    target.SetBackgroundBrush(PStandardColorID::DropdownMenuBackground);
    const PRect bounds = target.GetBounds();
    if (bounds.top < 0.0f) {
        target.EraseRect(PRect(bounds.left, bounds.top, bounds.right, std::min(bounds.bottom, 0.0f)));
    }
    if (bounds.bottom > state.ContentSize.y) {
        target.EraseRect(PRect(bounds.left, std::max(bounds.top, state.ContentSize.y), bounds.right, bounds.bottom));
    }
    if (state.ItemHeight > 0.0f)
    {
        const size_t firstVisible = size_t(std::max(0.0f, std::floor(updateRect.top / state.ItemHeight)));
        const size_t endVisible = std::min(size_t(std::max(0.0f, std::ceil(updateRect.bottom / state.ItemHeight))), data.Items.size());
        for (size_t i = firstVisible; i < endVisible; ++i)
        {
            if (i == data.Selection || i == data.PressedItem)
            {
                target.SetFgColor(255, 255, 255);
                target.SetBrush((i == data.Selection) ? PStandardColorID::DropdownMenuSelection : PStandardColorID::DropdownMenuPressed);
            }
            else
            {
                target.SetFgColor(0, 0, 0);
                target.SetBrush(PStandardColorID::DropdownMenuBackground);
            }
            const PRect itemFrame = CalculateItemFrame(target, data, i, state);
            const PPoint baseline(DROPDOWN_MENU_POPUP_LABEL_PADDING, std::round(itemFrame.top + state.FontHeight.ascender + state.FontHeight.line_gap * 0.5f));
            target.DrawString(target.GetBrush(), data.Items[i], baseline, itemFrame);
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressedItem = data.PressedItem;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuPopupStyleData& data = state.GetTarget().GetDropdownMenuPopupStyleData();
    UpdateDropdownMenuPopupMetrics(target, data, state);
    *minSize = state.ContentSize;
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupDefault::PropertyChanged(PView& target, PDropdownMenuPopupProperties properties, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuPopupStyleData& data = state.GetTarget().GetDropdownMenuPopupStyleData();
    const PDropdownMenuPopupProperties metricProperties(PDropdownMenuPopupProperty::Items, PDropdownMenuPopupProperty::Font);
    const bool metricsChanged = properties.HasAny(metricProperties);
    if (metricsChanged) {
        state.AreMetricsValid = false;
    }
    const PDropdownMenuPopupProperties incrementalProperties(PDropdownMenuPopupProperty::Selection, PDropdownMenuPopupProperty::Pressed);
    const bool propertiesRequireFullUpdate = !(properties & ~incrementalProperties).Empty();
    if (!state.HasVisualState || propertiesRequireFullUpdate)
    {
        target.Invalidate();
    }
    else
    {
        for (size_t index : {state.PreviousSelection, state.PreviousPressedItem, data.Selection, data.PressedItem})
        {
            if (index < data.Items.size()) {
                target.Invalidate(CalculateItemFrame(target, data, index, state));
            }
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressedItem = data.PressedItem;
    state.HasVisualState = true;
    // Notifications can replace the renderer binding, so finish all context updates first.
    if (metricsChanged)
    {
        target.ContentSizeChanged();
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuPopupDefault::GetItemFrame(const PView& target, size_t index, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuPopupStyleData& data = state.GetTarget().GetDropdownMenuPopupStyleData();
    return CalculateItemFrame(target, data, index, state);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PViewRendererDropdownMenuPopupDefault::GetItemFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PDropdownMenuPopupStyleData& data = state.GetTarget().GetDropdownMenuPopupStyleData();
    if (data.Items.empty()) {
        return INVALID_INDEX;
    }
    UpdateDropdownMenuPopupMetrics(target, data, state);
    const float itemHeight = state.ItemHeight;
    if (itemHeight <= 0.0f) {
        return INVALID_INDEX;
    }
    const float index = std::clamp(std::floor(position.y / itemHeight), 0.0f, float(data.Items.size() - 1));
    return size_t(index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupDefault::UpdateDropdownMenuPopupMetrics(const PView& target, const PDropdownMenuPopupStyleData& data, Context& state)
{
    if (!state.AreMetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        state.ItemHeight = std::round(state.FontHeight.descender + state.FontHeight.ascender + state.FontHeight.line_gap);
        state.ContentSize = PPoint(0.0f, float(data.Items.size()) * state.ItemHeight);
        for (const PString& item : data.Items) {
            state.ContentSize.x = std::max(state.ContentSize.x, target.GetStringWidth(item));
        }
        state.ContentSize.x += DROPDOWN_MENU_POPUP_LABEL_PADDING * 2.0f;
        state.AreMetricsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuPopupDefault::CalculateItemFrame(const PView& target, const PDropdownMenuPopupStyleData& data, size_t index, Context& state)
{
    if (index >= data.Items.size()) {
        return PRect();
    }
    UpdateDropdownMenuPopupMetrics(target, data, state);
    const float itemHeight = state.ItemHeight;
    const PRect bounds = target.GetBounds();
    return PRect(bounds.left, float(index) * itemHeight, bounds.right, float(index + 1) * itemHeight);
}
