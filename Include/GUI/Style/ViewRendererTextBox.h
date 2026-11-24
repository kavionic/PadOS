// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/TextBoxStyleData.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererTextBox : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::TextBox;

    virtual void PropertyChanged(PView& target, PTextBoxProperties properties, PtrTarget* context) const = 0;
    virtual PRect GetEditorFrame(const PView& target, PtrTarget* context) const = 0;
    virtual PPoint GetSizeForContent(const PView& target, const PPoint& contentSize, PtrTarget* context) const = 0;

    static PGUIColorID GetBackgroundColorID(const PTextBoxStyleData& data)
    {
        return !data.IsEnabled ? PGUIColorID::TextBoxBackgroundDisabled
            : data.IsReadOnly ? PGUIColorID::TextBoxBackgroundReadOnly : PGUIColorID::TextBoxBackgroundNormal;
    }

    static PGUIColorID GetTextColorID(const PTextBoxStyleData& data)
    {
        return !data.IsEnabled ? PGUIColorID::TextBoxTextDisabled : data.IsReadOnly ? PGUIColorID::TextBoxTextReadOnly : PGUIColorID::TextBoxTextNormal;
    }
};
