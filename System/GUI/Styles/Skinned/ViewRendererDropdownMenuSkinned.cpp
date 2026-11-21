// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleDropdownMenu.h>
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuSkinned.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

struct PViewRendererDropdownMenuSkinned::Context : PViewRendererContext<PViewStyleDropdownMenu>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}

    float ItemWidth = 0.0f;
    bool MetricsValid = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererDropdownMenuSkinned::PViewRendererDropdownMenuSkinned(const PDropdownMenuSkin& skin) : m_DropdownMenuSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererDropdownMenuSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PDropdownMenuStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuStyleData();
    const PDropdownMenuAppearance& appearance = m_DropdownMenuSkin.States[std::to_underlying(GetDropdownMenuState(data))];
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetBackgroundBrush(appearance.Background);
    target.SetBackgroundBrushOrigin(frame.TopLeft());
    target.SetBrush(nullptr);
    target.SetDrawingMode(PDrawingMode::Copy);
    target.EraseRect(frame);
    const PRect arrowFrame = GetDropdownMenuArrowFrame(frame);
    const PRect source(appearance.Arrow->GetBounds());
    const PPoint position = (arrowFrame.TopLeft() + (arrowFrame.Size() - source.Size()) * 0.5f + appearance.ArrowOffset).GetRounded();
    const PRect visible = PRect(position, position + source.Size()) & arrowFrame;
    if (visible.IsValid())
    {
        target.SetDrawingMode(PDrawingMode::Overlay);
        target.DrawBitmap(appearance.Arrow, visible - position + source.TopLeft(), visible.TopLeft());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PDropdownMenuStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuStyleData();
    const Context& state = GetDropdownMenuStyleData(target, data, context);
    const float textWidth = (data.Items.empty() && data.Text != nullptr) ? target.GetStringWidth(*data.Text) : state.ItemWidth;
    const PRect& insets = m_DropdownMenuSkin.EditorInsets;
    *minSize = PPoint(
        std::max(m_DropdownMenuSkin.MinimumSize.x, data.EditorSize.x + textWidth + insets.left + insets.right + m_DropdownMenuSkin.ArrowWidth),
        std::max(m_DropdownMenuSkin.MinimumSize.y, data.EditorSize.y + insets.top + insets.bottom));
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuSkinned::PropertyChanged(PView& target, PDropdownMenuProperties properties, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Font))) {
        state.MetricsValid = false;
    }
    target.Invalidate();
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Text, PDropdownMenuProperty::Font))) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuSkinned::GetEditorFrame(const PView& target, PtrTarget* context) const
{
    PRect frame = target.GetBounds();
    frame.right = GetDropdownMenuArrowFrame(frame).left;
    return PInsetSkinnedFrame(frame, m_DropdownMenuSkin.EditorInsets);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PTextBoxStyle> PViewRendererDropdownMenuSkinned::GetEditorStyle(const PView& target, PtrTarget* context) const
{
    const PDropdownMenuStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuStyleData();
    return m_DropdownMenuSkin.States[std::to_underlying(GetDropdownMenuState(data))].Editor;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PViewRendererDropdownMenuSkinned::HitTest(const PView& target, const PPoint& position, PtrTarget* context) const
{
    const PDropdownMenuStyleData& data = static_cast<Context*>(context)->GetTarget().GetDropdownMenuStyleData();
    const PRect frame = data.IsReadOnly ? target.GetBounds() : GetDropdownMenuArrowFrame(target.GetBounds());
    return data.IsEnabled && frame.DoIntersect(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinState PViewRendererDropdownMenuSkinned::GetDropdownMenuState(const PDropdownMenuStyleData& data)
{
    if (!data.IsEnabled) {
        return PButtonSkinState::Disabled;
    } else if (data.IsOpen) {
        return PButtonSkinState::Pressed;
    } else if (data.IsPointerOver) {
        return PButtonSkinState::Hovered;
    } else {
        return PButtonSkinState::Normal;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuSkinned::GetDropdownMenuArrowFrame(const PRect& frame) const
{
    return PRect(std::max(frame.left, frame.right - m_DropdownMenuSkin.ArrowWidth), frame.top, frame.right, frame.bottom);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererDropdownMenuSkinned::Context& PViewRendererDropdownMenuSkinned::GetDropdownMenuStyleData(
    const PView& target,
    const PDropdownMenuStyleData& data,
    PtrTarget* context)
{
    Context& state = *static_cast<Context*>(context);
    if (!state.MetricsValid)
    {
        state.ItemWidth = 0.0f;
        for (const PString& item : data.Items) {
            state.ItemWidth = std::max(state.ItemWidth, target.GetStringWidth(item));
        }
        state.MetricsValid = true;
    }
    return state;
}
