// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Math/Point.h>
#include <Utils/String.h>

enum class PFontID : uint8_t
{
    e_FontSmall,
    e_FontNormal,
    e_FontLarge,
    e_Font7Seg,
    e_FontCount
};

struct PFontHeight
{
    float ascender;   // Pixels above the baseline (positive).
    float descender;  // Pixels below the baseline (positive).
    float line_gap;   // Additional space between lines (positive).
};

enum class PFontRenderMode : uint8_t
{
    Antialiased,
    Monochrome
};

inline constexpr char FONT_DIRECTORY_PATH[] = "/usr/share/fonts";
inline constexpr uint32_t FONT_DPI = 96;
inline constexpr float FONT_MAX_POINT_SIZE = 512.0f;

struct PFontDescription
{
    PString Family = "DejaVu Sans";
    PString Style = "Book";
    float PointSize = 14.0f;
    float Rotation = 0.0f;   // Counterclockwise degrees; positive angles ascend on screen.
    float Shear = 90.0f;     // Degrees from the baseline; 90 is upright.
    PFontRenderMode RenderMode = PFontRenderMode::Antialiased;
    bool Hinting = true;

    bool operator==(const PFontDescription& rhs) const = default;
};

struct PFontFaceInfo
{
    PString Family;
    PString Style;
    bool Scalable = false;
    bool FixedWidth = false;
};

struct PFontTextMetrics
{
    PPoint Advance;          // Displacement of the pen, including rotation and shear.
    float Width = 0.0f;      // Advance along the unrotated baseline, including kerning.
    size_t ByteCount = 0;    // Complete UTF-8 prefix included in these metrics.
};
