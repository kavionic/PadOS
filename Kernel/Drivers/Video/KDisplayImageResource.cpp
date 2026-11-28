// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <utility>

#include <Kernel/Drivers/Video/KDisplayImageResource.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDisplayImageResource::KDisplayImageResource(Ptr<KDisplayBitmapResource> bitmap, const PImageDescription& description)
    : m_Bitmap(std::move(bitmap)), m_Description(description)
{
}

///////////////////////////////////////////////////////////////////////////////
/// Preparation depends on the image version, raster size and scale. Origins do
/// not affect the mappings. Active layers retain evicted samplers independently.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const KDisplayBrushSampler> KDisplayImageResource::GetSampler(const PBrushData& brush, const PIPoint& referenceSize)
{
    if (!PBrushUtils::IsValidScale(brush, m_Description)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const PIPoint size = PBrushUtils::TileSize(brush, m_Description, referenceSize);
    for (size_t index = 0; index < m_Samplers.size(); ++index)
    {
        CachedSampler& entry = m_Samplers[index];
        if (entry.Sampler != nullptr && entry.TileSize == size && entry.Scale == brush.Scale)
        {
            if (index != 0) {
                std::swap(m_Samplers[0], entry);
            }
            return m_Samplers[0].Sampler;
        }
    }
    PDisplayBrushLayer layer;
    layer.Bitmap = *m_Bitmap->GetBitmap(false);
    layer.Image = m_Description;
    layer.Data = brush;
    layer.Bounds = PIRect(PIPoint(0, 0), size);
    layer.ReferenceSize = referenceSize;
    Ptr<const KDisplayBrushSampler> sampler = ptr_new<KDisplayBrushSampler>(layer);
    m_Samplers[1] = std::move(m_Samplers[0]);
    m_Samplers[0] = {sampler, size, brush.Scale};
    return sampler;
}

} // namespace kernel
