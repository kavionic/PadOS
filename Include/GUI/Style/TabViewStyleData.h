// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <GUI/Style/GUIStyleData.h>
#include <Math/Rect.h>
#include <Utils/EnumBitmask.h>
#include <Utils/String.h>
#include <Utils/Utils.h>

enum class PTabViewProperty : uint32_t
{
    TabInserted = 0x0001,
    TabDeleted  = 0x0002,
    Title       = 0x0004,
    Font        = 0x0008,
    Selection   = 0x0010,
    Scroll      = 0x0020,
    Frame       = 0x0040,
    TopBar      = 0x0080,
    Highlight   = 0x0100
};

using PTabViewProperties = PEnumBitmask<PTabViewProperty>;

struct PTabViewStyleData : PGUIStyleData
{
    std::vector<PString> Titles;
    size_t Selection = INVALID_INDEX;
    size_t HighlightedTab = INVALID_INDEX;
    PPoint TopBarMinSize;
    PPoint TopBarMaxSize;
    bool HasTopBarClient = false;
};

struct PTabViewFrames
{
    PRect ClientFrame;
    PRect TabsFrame;
    PRect TopBarClientFrame;
};
