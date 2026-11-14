// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/View.h>
#include <System/ExceptionHandling.h>

PButtonSkin::PButtonSkin()
{
    for (size_t i = 0; i < States.size(); ++i)
    {
        const PStandardColorID color = (i == std::to_underlying(PButtonSkinState::Disabled))
            ? PStandardColorID::ButtonLabelDisabled : PStandardColorID::ButtonLabelNormal;
        States[i].LabelColor = pget_standard_color(color);
        CheckedStates[i].LabelColor = States[i].LabelColor;
    }
}

PSkinnableGUIStyle::PSkinnableGUIStyle(const PButtonSkin& buttonSkin) : m_ButtonSkin(buttonSkin)
{
    if (buttonSkin.LabelPadding.left < 0.0f || buttonSkin.LabelPadding.top < 0.0f
        || buttonSkin.LabelPadding.right < 0.0f || buttonSkin.LabelPadding.bottom < 0.0f) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

void PSkinnableGUIStyle::ButtonPaint(PView& target, const PRect& bounds, bool isEnabled, PButtonStyleData& data) const
{
    const Ptr<const PBrush> normal = m_ButtonSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background;
    if (normal == nullptr)
    {
        PDefaultGUIStyle::ButtonPaint(target, bounds, isEnabled, data);
        return;
    }
    UpdateButtonMetrics(target, data);
    PButtonSkinState state = PButtonSkinState::Normal;
    if (!isEnabled) {
        state = PButtonSkinState::Disabled;
    } else if (data.IsPressed) {
        state = PButtonSkinState::Pressed;
    } else if (data.IsPointerOver) {
        state = PButtonSkinState::Hovered;
    }
    const PButtonSkinAppearance& appearance = data.IsChecked
        ? m_ButtonSkin.CheckedStates[std::to_underlying(state)] : m_ButtonSkin.States[std::to_underlying(state)];
    target.SetBackgroundBrush((appearance.Background != nullptr) ? appearance.Background : normal);
    target.SetBackgroundBrushOrigin(bounds.TopLeft());
    target.SetBrush(nullptr);

    const PRect padding = GetButtonLabelPadding();
    const PRect labelFrame = bounds.GetResized(padding.left, padding.top, -padding.right, -padding.bottom);
    PPoint position(
        std::round(labelFrame.left + (labelFrame.Width() - data.LabelSize.x) * 0.5f),
        std::round(labelFrame.top + (labelFrame.Height() - data.LabelSize.y) * 0.5f));
    if (data.LabelAlignment == PAlignment::Left) {
        position.x = labelFrame.left;
    } else if (data.LabelAlignment == PAlignment::Right) {
        position.x = labelFrame.right - data.LabelSize.x;
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

void PSkinnableGUIStyle::ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_ButtonSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background == nullptr)
    {
        PDefaultGUIStyle::ButtonGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    UpdateButtonMetrics(target, data);
    const PRect padding = GetButtonLabelPadding();
    const PPoint size = data.LabelSize + PPoint(padding.left + padding.right, padding.top + padding.bottom);
    *minSize = size;
    *maxSize = size;
}

PRect PSkinnableGUIStyle::GetButtonLabelPadding() const
{
    PRect padding = m_ButtonSkin.LabelPadding;
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
                    padding.left = std::max(padding.left, (slice.left - float(source.left)) * scale.x);
                    padding.top = std::max(padding.top, (slice.top - float(source.top)) * scale.y);
                    padding.right = std::max(padding.right, (float(source.right) - slice.right) * scale.x);
                    padding.bottom = std::max(padding.bottom, (float(source.bottom) - slice.bottom) * scale.y);
                }
            }
        }
    }
    return padding;
}
