// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>

#include <GUI/Style/DropdownMenuStyleData.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererDropdownMenu : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::DropdownMenu;

    virtual void PropertyChanged(PView& target, PDropdownMenuProperties properties, PtrTarget* context) const = 0;
    virtual PRect GetEditorFrame(const PView& target, PtrTarget* context) const = 0;
    virtual bool HitTest(const PView& target, const PPoint& position, PtrTarget* context) const = 0;

    static PGUIColorID GetBackgroundColorID(const PDropdownMenuStyleData& data) { return BACKGROUND_COLORS[GetColorIndex(data)]; }
    static PGUIColorID GetTextColorID(const PDropdownMenuStyleData& data) { return TEXT_COLORS[GetColorIndex(data)]; }

private:
    static size_t GetColorIndex(const PDropdownMenuStyleData& data)
    {
        const size_t state = !data.IsEnabled ? 3 : data.IsOpen ? 2 : data.IsPointerOver ? 1 : 0;
        return state + (data.IsReadOnly ? 0 : BACKGROUND_COLORS.size() / 2);
    }

    // Normal, hovered, open, disabled, then the corresponding editable states.
    static constexpr std::array<PGUIColorID, 8> BACKGROUND_COLORS =
    {
        PGUIColorID::DropdownBackgroundNormal,
        PGUIColorID::DropdownBackgroundHovered,
        PGUIColorID::DropdownBackgroundPressed,
        PGUIColorID::DropdownBackgroundDisabled,
        PGUIColorID::DropdownBackgroundEditableNormal,
        PGUIColorID::DropdownBackgroundEditableHovered,
        PGUIColorID::DropdownBackgroundEditablePressed,
        PGUIColorID::DropdownBackgroundEditableDisabled
    };

    static constexpr std::array<PGUIColorID, 8> TEXT_COLORS =
    {
        PGUIColorID::DropdownTextNormal,
        PGUIColorID::DropdownTextHovered,
        PGUIColorID::DropdownTextPressed,
        PGUIColorID::DropdownTextDisabled,
        PGUIColorID::DropdownTextEditableNormal,
        PGUIColorID::DropdownTextEditableHovered,
        PGUIColorID::DropdownTextEditablePressed,
        PGUIColorID::DropdownTextEditableDisabled
    };
};
