// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <GUI/Brush.h>
#include <GUI/Styles/DefaultGUIStyle.h>

enum class PButtonSkinState : uint8_t
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Count
};

struct PButtonSkinAppearance
{
    Ptr<const PBrush> Background;
    PColor LabelColor = PColor(0, 0, 0);
};

struct PButtonSkin
{
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> CheckedStates;
    PRect LabelPadding = PRect(8.0f, 4.0f, 8.0f, 4.0f);
    PPoint PressedLabelOffset = PPoint(1.0f, 1.0f);

    PButtonSkin();
};

class PSkinnableGUIStyle : public PDefaultGUIStyle
{
public:
    PSkinnableGUIStyle() = default;
    explicit PSkinnableGUIStyle(const PButtonSkin& buttonSkin);

    void ButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const override;
    void ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

private:
    PRect GetButtonLabelPadding() const;

    PButtonSkin m_ButtonSkin;
};
