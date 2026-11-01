// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <GUI/Style/GUIStyleData.h>
#include <Utils/EnumBitmask.h>
#include <Utils/String.h>
#include <Utils/Utils.h>

enum class PDropdownMenuPopupProperty : uint32_t
{
    Items     = 0x01,
    Font      = 0x02,
    Selection = 0x04,
    Pressed   = 0x08,
    Frame     = 0x10
};

using PDropdownMenuPopupProperties = PEnumBitmask<PDropdownMenuPopupProperty>;

struct PDropdownMenuPopupStyleData : PGUIStyleData
{
    explicit PDropdownMenuPopupStyleData(const std::vector<PString>& items) : Items(items) {}

    const std::vector<PString>& Items;
    size_t Selection = INVALID_INDEX;
    size_t PressedItem = INVALID_INDEX;
};
