// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>

#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererButtonBase : public PViewRenderer
{
public:
    virtual void PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context) const = 0;

protected:
    // Normal, hovered, pressed, disabled, then the corresponding checked states.
    static constexpr std::array<PGUIColorID, 8> BUTTON_LABEL_COLORS =
    {
        PGUIColorID::ButtonLabelNormal,
        PGUIColorID::ButtonLabelHovered,
        PGUIColorID::ButtonLabelPressed,
        PGUIColorID::ButtonLabelDisabled,
        PGUIColorID::ButtonLabelCheckedNormal,
        PGUIColorID::ButtonLabelCheckedHovered,
        PGUIColorID::ButtonLabelCheckedPressed,
        PGUIColorID::ButtonLabelCheckedDisabled
    };

    static constexpr std::array<PGUIColorID, 8> CHECK_BOX_LABEL_COLORS =
    {
        PGUIColorID::CheckBoxLabelNormal,
        PGUIColorID::CheckBoxLabelHovered,
        PGUIColorID::CheckBoxLabelPressed,
        PGUIColorID::CheckBoxLabelDisabled,
        PGUIColorID::CheckBoxLabelCheckedNormal,
        PGUIColorID::CheckBoxLabelCheckedHovered,
        PGUIColorID::CheckBoxLabelCheckedPressed,
        PGUIColorID::CheckBoxLabelCheckedDisabled
    };

    static constexpr std::array<PGUIColorID, 8> RADIO_BUTTON_LABEL_COLORS =
    {
        PGUIColorID::RadioButtonLabelNormal,
        PGUIColorID::RadioButtonLabelHovered,
        PGUIColorID::RadioButtonLabelPressed,
        PGUIColorID::RadioButtonLabelDisabled,
        PGUIColorID::RadioButtonLabelCheckedNormal,
        PGUIColorID::RadioButtonLabelCheckedHovered,
        PGUIColorID::RadioButtonLabelCheckedPressed,
        PGUIColorID::RadioButtonLabelCheckedDisabled
    };

    static PGUIColorID GetLabelColorID(const std::array<PGUIColorID, 8>& colors, bool isEnabled, const PButtonStyleData& data)
    {
        const size_t state = !isEnabled ? 3 : data.IsPressed ? 2 : data.IsPointerOver ? 1 : 0;
        return colors[state + (data.IsChecked ? colors.size() / 2 : 0)];
    }
};
