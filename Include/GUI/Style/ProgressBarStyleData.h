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

enum class PProgressBarProperty : uint32_t
{
    Value = 0x01,
    Frame = 0x02,
    Font  = 0x04
};

using PProgressBarProperties = PEnumBitmask<PProgressBarProperty>;

struct PProgressBarStyleData : PGUIStyleData
{
    // Relative progress: zero is empty and one is complete.
    float Value = 0.0f;
    POrientation Orientation = POrientation::Horizontal;
};
