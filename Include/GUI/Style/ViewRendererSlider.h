// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/SliderStyleData.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererSlider : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::Slider;

    virtual void PropertyChanged(PView& target, PSliderProperties properties, PtrTarget* context) const = 0;
    virtual PPoint GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const = 0;
    virtual float GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const = 0;
};
