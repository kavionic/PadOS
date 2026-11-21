// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Skinned/ViewRendererButtonSkinned.h>

#include <algorithm>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererButtonSkinned::PViewRendererButtonSkinned(const PButtonSkin& skin) : m_ButtonSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonSkinned::Paint(PView& target, const PRect& bounds, const PRect& updateRect, PtrTarget* context) const
{
    Context& renderContext = *static_cast<Context*>(context);
    const PButtonStyleData& data = renderContext.GetTarget().GetButtonStyleData();
    const bool isEnabled = renderContext.GetTarget().IsButtonEnabled();
    const Ptr<const PBrush> normal = m_ButtonSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background;
    UpdateMetrics(target, data, renderContext);
    const PButtonSkinState state = GetButtonState(isEnabled, data);
    const PButtonSkinAppearance& appearance = data.IsChecked
        ? m_ButtonSkin.CheckedStates[std::to_underlying(state)] : m_ButtonSkin.States[std::to_underlying(state)];
    target.SetBackgroundBrush((appearance.Background != nullptr) ? appearance.Background : normal);
    target.SetBackgroundBrushOrigin(bounds.TopLeft());
    target.SetBrush(nullptr);

    const PRect& padding = m_ButtonSkin.LabelPadding;
    const PRect labelFrame = bounds.GetResized(padding.left, padding.top, -padding.right, -padding.bottom);
    PPoint position(
        std::round(labelFrame.left + (labelFrame.Width() - renderContext.LabelSize.x) * 0.5f),
        std::round(labelFrame.top + (labelFrame.Height() - renderContext.LabelSize.y) * 0.5f));
    if (data.LabelAlignment == PAlignment::Left) {
        position.x = labelFrame.left;
    } else if (data.LabelAlignment == PAlignment::Right) {
        position.x = labelFrame.right - renderContext.LabelSize.x;
    }
    if (isEnabled && data.IsPressed) {
        position += m_ButtonSkin.PressedLabelOffset;
    }
    position.y += target.GetFontHeight().ascender;
    target.SetFgColor(appearance.LabelColor);
    const PDrawingMode previousMode = target.GetDrawingMode();
    target.SetDrawingMode(PDrawingMode::Copy);
    target.DrawString(data.Label, position, bounds);
    target.SetDrawingMode(previousMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& renderContext = *static_cast<Context*>(context);
    const PButtonStyleData& data = renderContext.GetTarget().GetButtonStyleData();
    UpdateMetrics(target, data, renderContext);
    const PRect& padding = m_ButtonSkin.LabelPadding;
    PPoint size = GetButtonImageMinimumSize();
    size.x = std::max(size.x, renderContext.LabelSize.x + padding.left + padding.right);
    size.y = std::max(size.y, renderContext.LabelSize.y + padding.top + padding.bottom);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererButtonSkinned::GetButtonImageMinimumSize() const
{
    PPoint size;
    for (const std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)>* states
        : {&m_ButtonSkin.States, &m_ButtonSkin.CheckedStates})
    {
        for (const PButtonSkinAppearance& appearance : *states)
        {
            if (appearance.Background != nullptr)
            {
                const Ptr<const PImage> image = appearance.Background->GetImage();
                if (image != nullptr)
                {
                    const PImageDescription& description = image->GetDescription();
                    const PRect& slice = description.SliceRect;
                    const PIRect& source = description.SourceRect;
                    const PPoint scale = appearance.Background->GetScale();
                    size.x = std::max(size.x, (slice.left - float(source.left) + float(source.right) - slice.right) * scale.x);
                    size.y = std::max(size.y, (slice.top - float(source.top) + float(source.bottom) - slice.bottom) * scale.y);
                }
            }
        }
    }
    return size;
}
