// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Math/Point.h>
#include <Utils/EnumBitmask.h>

enum class PTextBoxProperty : uint32_t
{
    EditorSize  = 0x01,
    ReadOnly    = 0x02,
    RaisedFrame = 0x04,
    Enabled     = 0x08,
    Frame       = 0x10
};

using PTextBoxProperties = PEnumBitmask<PTextBoxProperty>;

struct PTextBoxStyleData
{
    // Intrinsic text size, including the optional line gap, without frame insets.
    PPoint EditorSize;
    bool IsReadOnly = false;
    bool HasRaisedFrame = false;
    bool IsEnabled = true;
};
