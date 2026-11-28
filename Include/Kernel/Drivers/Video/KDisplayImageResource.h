// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>

#include <Kernel/Drivers/Video/KDisplayBrushSampler.h>
#include <Kernel/Drivers/Video/KDisplayResources.h>

namespace kernel
{

class KDisplayImageResource : public PtrTarget
{
public:
    KDisplayImageResource(Ptr<KDisplayBitmapResource> bitmap, const PImageDescription& description);

    const Ptr<KDisplayBitmapResource>& GetBitmap() const noexcept { return m_Bitmap; }
    const PImageDescription& GetDescription() const noexcept { return m_Description; }
    Ptr<const KDisplayBrushSampler> GetSampler(const PBrushData& brush, const PIPoint& referenceSize);

private:
    static constexpr size_t SAMPLER_CACHE_SIZE = 2;

    struct CachedSampler
    {
        Ptr<const KDisplayBrushSampler> Sampler;
        PIPoint TileSize;
        PPoint Scale;
    };

    Ptr<KDisplayBitmapResource> m_Bitmap;
    PImageDescription m_Description;
    std::array<CachedSampler, SAMPLER_CACHE_SIZE> m_Samplers;
};

} // namespace kernel
