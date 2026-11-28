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
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuPopupSkinned.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

struct PViewRendererDropdownMenuPopupSkinned::Context : PViewRendererContext<PViewStyleDropdownMenuPopup>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}

    std::array<PSkinnedImage, std::to_underlying(PDropdownMenuRowState::Count)> Rows;
    PFontHeight FontHeight;
    PPoint ContentSize;
    size_t PreviousSelection = INVALID_INDEX;
    size_t PreviousPressed = INVALID_INDEX;
    float ItemWidth = 0.0f;
    float ItemHeight = 0.0f;
    bool MetricsValid = false;
    bool HasVisualState = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererDropdownMenuPopupSkinned::PViewRendererDropdownMenuPopupSkinned(const PDropdownMenuSkin& skin) : m_DropdownMenuSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererDropdownMenuPopupSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PDropdownMenuPopupStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuPopupStyleData();
    Context& state = GetDropdownMenuPopupStyleData(target, data, context);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); target.SetBrush(PBrush::zero); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_DropdownMenuSkin.PopupBackground);
    target.SetBrush(PBrush::zero);
    const PRect damage = updateRect & target.GetBounds();
    target.EraseRect(damage);
    const float itemCount = float(data.Items.size());
    const size_t first = size_t(std::clamp(std::floor(damage.top / state.ItemHeight), 0.0f, itemCount));
    const size_t end = size_t(std::clamp(std::ceil(damage.bottom / state.ItemHeight), 0.0f, itemCount));
    for (size_t index = first; index < end; ++index) {
        PaintDropdownMenuRow(target, data, state, index, context);
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressed = data.PressedItem;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PDropdownMenuPopupStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuPopupStyleData();
    *minSize = GetDropdownMenuPopupStyleData(target, data, context).ContentSize;
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupSkinned::PropertyChanged(PView& target, PDropdownMenuPopupProperties properties, PtrTarget* context) const
{
    const PDropdownMenuPopupStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuPopupStyleData();
    Context& state = *static_cast<Context*>(context);
    const bool metricsChanged = properties.HasAny(PDropdownMenuPopupProperties(PDropdownMenuPopupProperty::Items, PDropdownMenuPopupProperty::Font));
    if (metricsChanged) {
        state.MetricsValid = false;
    }
    const PDropdownMenuPopupProperties incremental(PDropdownMenuPopupProperty::Selection, PDropdownMenuPopupProperty::Pressed);
    if (!state.HasVisualState || !(properties & ~incremental).Empty())
    {
        target.Invalidate();
    }
    else
    {
        for (size_t index : {state.PreviousSelection, state.PreviousPressed, data.Selection, data.PressedItem})
        {
            if (index < data.Items.size()) {
                target.Invalidate(GetItemFrame(target, index, context));
            }
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressed = data.PressedItem;
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

PRect PViewRendererDropdownMenuPopupSkinned::GetItemFrame(const PView& target, size_t index, PtrTarget* context) const
{
    const PDropdownMenuPopupStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuPopupStyleData();
    if (index >= data.Items.size()) {
        return PRect();
    }
    const float height = GetDropdownMenuPopupStyleData(target, data, context).ItemHeight;
    const PRect bounds = target.GetBounds();
    return PRect(bounds.left, float(index) * height, bounds.right, float(index + 1) * height);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PViewRendererDropdownMenuPopupSkinned::GetItemFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    const PDropdownMenuPopupStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuPopupStyleData();
    if (data.Items.empty()) {
        return INVALID_INDEX;
    }
    const float height = GetDropdownMenuPopupStyleData(target, data, context).ItemHeight;
    return size_t(std::clamp(std::floor(position.y / height), 0.0f, float(data.Items.size() - 1)));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererDropdownMenuPopupSkinned::Context& PViewRendererDropdownMenuPopupSkinned::GetDropdownMenuPopupStyleData(
    const PView& target,
    const PDropdownMenuPopupStyleData& data, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    if (!state.MetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        state.ItemWidth = 0.0f;
        for (const PString& item : data.Items) {
            state.ItemWidth = std::max(state.ItemWidth, target.GetStringWidth(item));
        }
        const PRect& padding = m_DropdownMenuSkin.RowPadding;
        const float textHeight = state.FontHeight.ascender + state.FontHeight.descender;
        state.ItemHeight = std::max(m_DropdownMenuSkin.MinimumRowHeight, std::ceil(textHeight + padding.top + padding.bottom));
        state.ContentSize = PPoint(std::max(0.0f, state.ItemWidth + padding.left + padding.right), float(data.Items.size()) * state.ItemHeight);
        state.MetricsValid = true;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupSkinned::PaintDropdownMenuRow(
    PView& target,
    const PDropdownMenuPopupStyleData& data,
    Context& state,
    size_t index, PtrTarget* context) const
{
    PDropdownMenuRowState rowState = PDropdownMenuRowState::Normal;
    if (index == data.PressedItem) {
        rowState = PDropdownMenuRowState::Pressed;
    } else if (index == data.Selection) {
        rowState = PDropdownMenuRowState::Selected;
    }
    const size_t appearanceIndex = std::to_underlying(rowState);
    const PDropdownMenuRowAppearance& appearance = m_DropdownMenuSkin.Rows[appearanceIndex];
    const PRect frame = GetItemFrame(target, index, context);
    if (!frame.IsValid()) {
        return;
    }
    const PBrush brush = PGetSkinnedBrush(state.Rows[appearanceIndex], appearance.Image, frame.Size());
    const PRect& padding = m_DropdownMenuSkin.RowPadding;
    const float textHeight = state.FontHeight.ascender + state.FontHeight.descender;
    const PPoint baseline(
        std::round(frame.left + padding.left + appearance.LabelOffset.x),
        std::round(frame.top + padding.top + (frame.Height() - padding.top - padding.bottom - textHeight) * 0.5f
            + state.FontHeight.ascender + appearance.LabelOffset.y));
    target.SetBrushOrigin(frame.TopLeft());
    target.SetFgColor(appearance.LabelColor);
    const bool fits = baseline.x >= frame.left && baseline.x + state.ItemWidth <= frame.right
        && baseline.y - state.FontHeight.ascender >= frame.top && baseline.y + state.FontHeight.descender <= frame.bottom;
    if (fits)
    {
        target.DrawString(brush, data.Items[index], baseline, frame);
    }
    else
    {
        const PIRect clip = PIRect(frame + target.GetScrollOffset()) & PIRect(target.GetNormalizedBounds());
        const Ptr<const PRegion> previousRegion = target.GetDrawingRegion();
        const Ptr<const PRegion> region = (previousRegion != nullptr) ? ptr_new<PRegion>(*previousRegion, clip, false) : ptr_new<PRegion>(clip);
        PScopeExit restoreRegion([&target, previousRegion] { target.SetDrawingRegion(previousRegion); });
        target.SetDrawingRegion(region);
        target.DrawString(brush, data.Items[index], baseline, frame);
    }
}
