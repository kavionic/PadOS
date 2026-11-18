// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <Utils/String.h>

inline constexpr const char* DEFAULT_GUI_STYLE_NAME = "Default";
inline constexpr const char* DEFAULT_GUI_STYLE_VARIANT = "Normal";

struct PGUIStyleInfo
{
    PString Name;
    std::vector<PString> Variants;
};
