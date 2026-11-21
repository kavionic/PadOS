// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererScrollBar : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::ScrollBar;

    virtual void PropertyChanged(PView& target, PScrollBarProperties properties, PtrTarget* context) const = 0;
    virtual PScrollBarPart HitTest(const PView& target, const PPoint& position, PtrTarget* context) const = 0;
    // Positions refer to the knob center, including while dragging.
    virtual PPoint GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const = 0;
    virtual float GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const = 0;
};
