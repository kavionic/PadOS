// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewStyle.h>
#include <GUI/Style/ViewRendererSlider.h>
#include <GUI/Style/SliderStyleData.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleSlider : public PViewStyle<PViewRendererSlider>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PSliderStyleData& GetSliderStyleData() const = 0;
};
