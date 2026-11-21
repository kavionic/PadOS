// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Brush.h>

struct PSkinnedImage
{
    Ptr<PImage> Image;
    Ptr<PBrush> Brush;
};

Ptr<const PBrush> PGetSkinnedBrush(PSkinnedImage& cache, const PImage& source, const PPoint& size);
PRect PInsetSkinnedFrame(const PRect& frame, const PRect& insets);
