// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <Math/Point.h>
#include <Utils/EnumBitmask.h>
#include <Utils/String.h>

enum class PDropdownMenuProperty : uint32_t
{
    Items    = 0x01,
    Text     = 0x02,
    Font     = 0x04,
    ReadOnly = 0x08,
    Enabled  = 0x10,
    Open     = 0x20,
    Frame    = 0x40,
    Hovered  = 0x80
};

using PDropdownMenuProperties = PEnumBitmask<PDropdownMenuProperty>;

struct PDropdownMenuStyleData
{
    std::vector<PString> Items;
    // Borrows the editor's string object; the editor retains ownership.
    const PString* Text = nullptr;
    // Intrinsic size of the empty text editor, excluding renderer insets.
    PPoint EditorSize;
    bool IsReadOnly = false;
    bool IsEnabled = true;
    bool IsOpen = false;
    bool IsPointerOver = false;
};
