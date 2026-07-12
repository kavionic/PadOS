// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 11.07.2026 20:30

#pragma once

#include <stdint.h>

#include <span>
#include <utility>

#include <GUI/Color.h>
#include <Math/Point.h>

using PMouseCursorID = uint32_t;
using PMouseCursorToken = uint32_t;

static constexpr PMouseCursorID    PInvalidMouseCursorID = uint32_t(-1);
static constexpr PMouseCursorID    PFirstCustomMouseCursorID = 1000;
static constexpr PMouseCursorToken PInvalidMouseCursorToken = 0;

enum class PMouseCursorPixel : uint8_t
{
    Transparent = 0,
    Color1      = 1,
    Color2      = 2,
    Invert      = 3
};

enum class PStandardMouseCursor : PMouseCursorID
{
    Pointer = 0,
    TextSelect,
    Busy,
    Crosshair,
    ResizeHorizontal,
    ResizeVertical,
    ResizeDiagonalNWSE,
    ResizeDiagonalNESW,
    Move,
    Hand,
    NotAllowed,
    Hidden,
    Count
};

struct PMouseCursorBitmap
{
    int32_t                         Width = 0;
    int32_t                         Height = 0;
    PIPoint                         HotSpot;
    PColor                          Color1 = PColor(0xff000000);
    PColor                          Color2 = PColor(0xffffffff);
    std::span<const PMouseCursorPixel> Raster;
};

static constexpr PMouseCursorID ToMouseCursorID(PStandardMouseCursor cursor)
{
    return std::to_underlying(cursor);
}

static constexpr bool IsStandardMouseCursorID(PMouseCursorID cursorID)
{
    return cursorID < ToMouseCursorID(PStandardMouseCursor::Count);
}
