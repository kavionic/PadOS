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

struct PGUIStyleVariantInfo
{
    PString Name;
    PString DisplayName;
    // Resolved filesystem path; empty when the variant has no preview.
    PString PreviewPath;

    bool operator==(const PGUIStyleVariantInfo& rhs) const = default;
};

struct PGUIStyleInfo
{
    PString Name;
    std::vector<PGUIStyleVariantInfo> Variants;
};
