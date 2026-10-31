// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/GUIDefines.h>
#include <GUI/Style/GUIStyleData.h>
#include <Utils/EnumBitmask.h>

enum class PScrollBarPart : uint8_t
{
    None,
    Knob,
    DecrementStart,
    IncrementStart,
    DecrementEnd,
    IncrementEnd,
    PageDecrement,
    PageIncrement
};

enum class PScrollBarProperty : uint32_t
{
    Value       = 0x01,
    Proportion  = 0x02,
    Range       = 0x04,
    Pressed     = 0x08,
    Enabled     = 0x10,
    Frame       = 0x20,
    Font        = 0x40
};

using PScrollBarProperties = PEnumBitmask<PScrollBarProperty>;

struct PScrollBarStyleData : PGUIStyleData
{
    // Relative value: zero at the minimum and one at the maximum.
    float Value = 0.0f;
    float Proportion = 0.1f;
    POrientation Orientation = POrientation::Vertical;
    PScrollBarPart PressedPart = PScrollBarPart::None;
    bool HasRange = true;
    bool IsEnabled = true;
};
