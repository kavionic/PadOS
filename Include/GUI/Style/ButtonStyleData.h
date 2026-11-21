// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/GUIDefines.h>
#include <Utils/EnumBitmask.h>
#include <Utils/String.h>

enum class PButtonProperty : uint32_t
{
    Label          = 0x0001,
    Font           = 0x0002,
    LabelAlignment = 0x0004,
    Pressed        = 0x0008,
    Checked        = 0x0010,
    Hovered        = 0x0020,
    Enabled        = 0x0040,
    Frame          = 0x0080,
    Checkable      = 0x0100
};

using PButtonProperties = PEnumBitmask<PButtonProperty>;

struct PButtonStyleData
{
    PString Label;
    PAlignment LabelAlignment = PAlignment::Center;
    bool IsCheckable = false;
    bool IsPressed = false;
    bool IsChecked = false;
    bool IsPointerOver = false;
};
