// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <Utils/String.h>

// Layout and variant are XKB identifiers; Description is the UTF-8 display name.
// An empty Variant selects the layout's default variant.
struct PKeyboardLayoutInfo
{
    PString Layout;
    PString Variant;
    PString Description;
};
