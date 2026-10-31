// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <GUI/Color.h>
#include <GUI/GUIDefines.h>
#include <GUI/Style/GUIStyleData.h>
#include <Utils/EnumBitmask.h>
#include <Utils/String.h>

enum class PSliderProperty : uint32_t
{
    Value           = 0x0001,
    Label           = 0x0002,
    ValueLabel      = 0x0004,
    MinLabel        = 0x0008,
    MaxLabel        = 0x0010,
    ShadowValues    = 0x0020,
    Pressed         = 0x0040,
    Enabled         = 0x0080,
    TickCount       = 0x0100,
    SliderSize      = 0x0200,
    SliderColors    = 0x0400,
    Appearance      = 0x0800,
    Font            = 0x1000,
    Frame           = 0x2000
};

using PSliderProperties = PEnumBitmask<PSliderProperty>;

struct PSliderStyleData : PGUIStyleData
{
    PString Label;
    PString ValueLabel;
    PString MinLabel;
    PString MaxLabel;

    // Relative values: zero at the minimum and one at the maximum.
    std::vector<float> ShadowValues;
    float Value = 0.0f;
    PColor SliderColor1;
    PColor SliderColor2;
    float SliderSize = 5.0f;
    int TickCount = 10;
    POrientation Orientation = POrientation::Horizontal;
    bool IsPressed = false;
    bool IsEnabled = true;
    bool HasValueLabel = false;
    bool TicksAbove = false;
    bool TicksBelow = false;
    bool KnobPointUp = false;
    bool KnobPointDown = false;
};
