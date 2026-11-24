// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <utility>

#include <GUI/Brush.h>
#include <GUI/Style/ViewRenderer.h>

enum class PGUIColorID : uint8_t
{
    // Text and graphics drawn on the style's background.
    Foreground,
    // Flat background color; the corresponding brush may be textured.
    Background,
    ForegroundDisabled,
    GroupLabel,

    ButtonLabelNormal,
    ButtonLabelHovered,
    ButtonLabelPressed,
    ButtonLabelDisabled,
    ButtonLabelCheckedNormal,
    ButtonLabelCheckedHovered,
    ButtonLabelCheckedPressed,
    ButtonLabelCheckedDisabled,

    CheckBoxLabelNormal,
    CheckBoxLabelHovered,
    CheckBoxLabelPressed,
    CheckBoxLabelDisabled,
    CheckBoxLabelCheckedNormal,
    CheckBoxLabelCheckedHovered,
    CheckBoxLabelCheckedPressed,
    CheckBoxLabelCheckedDisabled,

    RadioButtonLabelNormal,
    RadioButtonLabelHovered,
    RadioButtonLabelPressed,
    RadioButtonLabelDisabled,
    RadioButtonLabelCheckedNormal,
    RadioButtonLabelCheckedHovered,
    RadioButtonLabelCheckedPressed,
    RadioButtonLabelCheckedDisabled,

    TabLabelNormal,
    TabLabelHovered,
    TabLabelPressed,
    TabLabelSelectedNormal,
    TabLabelSelectedHovered,
    TabLabelSelectedPressed,

    DropdownBackgroundNormal,
    DropdownBackgroundHovered,
    DropdownBackgroundPressed,
    DropdownBackgroundDisabled,
    DropdownBackgroundEditableNormal,
    DropdownBackgroundEditableHovered,
    DropdownBackgroundEditablePressed,
    DropdownBackgroundEditableDisabled,
    DropdownTextNormal,
    DropdownTextHovered,
    DropdownTextPressed,
    DropdownTextDisabled,
    DropdownTextEditableNormal,
    DropdownTextEditableHovered,
    DropdownTextEditablePressed,
    DropdownTextEditableDisabled,

    TextBoxBackgroundNormal,
    TextBoxBackgroundReadOnly,
    TextBoxBackgroundDisabled,
    TextBoxTextNormal,
    TextBoxTextReadOnly,
    TextBoxTextDisabled,
    Count
};

class PGUIStyle : public PtrTarget
{
public:
    explicit PGUIStyle(PApplication* application = nullptr);

    // Return a configured brush, or lazily create a solid brush from the palette color.
    // Hold the owning application mutex when obtaining or using brushes.
    Ptr<const PBrush> GetBrush(PGUIColorID colorID) const;

    PColor GetColor(PGUIColorID colorID) const { return m_Colors[std::to_underlying(colorID)]; }

    static Ptr<const PViewRenderer> CreateDefaultRenderer(PGUIComponentID componentID);

    // Configure renderers before binding views; existing bindings retain their renderer.
    // A null renderer restores lazy default selection for subsequent lookups.
    void SetRenderer(PGUIComponentID componentID, Ptr<const PViewRenderer> renderer);

    // Lookup may populate the cache. Serialize access with the owning application mutex.
    Ptr<const PViewRenderer> GetRenderer(PGUIComponentID componentID) const;

    template<class TRendererType>
    Ptr<const TRendererType> GetRenderer(PGUIComponentID componentID) const
    {
        return ptr_dynamic_cast<const TRendererType>(GetRenderer(componentID));
    }

protected:
    // Configure colors and optional brushes during construction, before the style is installed.
    void SetColor(PGUIColorID colorID, PColor color) { m_Colors[std::to_underlying(colorID)] = color; }
    void SetBrush(PGUIColorID colorID, Ptr<const PBrush> brush) { m_Brushes[std::to_underlying(colorID)] = std::move(brush); }

private:
    WeakPtr<PApplication> m_Application;
    std::array<PColor, std::to_underlying(PGUIColorID::Count)> m_Colors;
    mutable std::array<Ptr<const PBrush>, std::to_underlying(PGUIColorID::Count)> m_Brushes;
    mutable std::array<Ptr<const PViewRenderer>, std::to_underlying(PGUIComponentID::Count)> m_Renderers;
};
