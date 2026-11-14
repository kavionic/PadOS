// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Image.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImage::PImage(Ptr<const PBitmap> bitmap) : PImage(bitmap, (bitmap != nullptr) ? bitmap->GetBounds() : PIRect())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImage::PImage(Ptr<const PBitmap> bitmap, const PIRect& sourceRect) : m_Bitmap(std::move(bitmap))
{
    if (m_Bitmap == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    m_Description = DescribeBitmap(*m_Bitmap, sourceRect);
    m_Description.LayoutSize = PPoint(1.0f, 1.0f);
    m_Description.LayoutMode = PImageLayoutMode::ViewRelative;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImage::PImage(Ptr<const PBitmap> bitmap, const PImageDescription& description) : m_Bitmap(std::move(bitmap)), m_Description(description)
{
    if (m_Bitmap == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    ClassifyOpacity(*m_Bitmap, m_Description);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImage::PImage(const PImage& source) : PtrTarget(), m_Bitmap(source.m_Bitmap), m_Description(source.m_Description)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetLayoutSize(const PPoint& size)
{
    SetLayoutSize(size, m_Description.LayoutMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetLayoutSize(const PPoint& size, PImageLayoutMode mode)
{
    PImageDescription description = m_Description;
    description.LayoutSize = size;
    description.LayoutMode = mode;
    SetDescription(description);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetSliceRect(const PRect& rectangle)
{
    if (rectangle != m_Description.SliceRect)
    {
        PImageDescription description = m_Description;
        description.SliceRect = rectangle;
        ClassifyOpacity(*m_Bitmap, description);
        SetDescription(description);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetTiling(PImageTiling tiling)
{
    PImageDescription description = m_Description;
    description.Tiling = tiling;
    SetDescription(description);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImageDescription PImage::DescribeBitmap(const PBitmap& bitmap, const PIRect& sourceRect)
{
    PImageDescription description;
    description.SourceRect = sourceRect;
    description.SliceRect = PRect(sourceRect);
    description.LayoutSize = PPoint(sourceRect.Size());
    ClassifyOpacity(bitmap, description);
    return description;
}

///////////////////////////////////////////////////////////////////////////////
/// Scan source alpha only when source slicing changes. Layout, tiling and brush
/// scale changes reuse this classification; no raster access is needed to draw.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::ClassifyOpacity(const PBitmap& bitmap, PImageDescription& description)
{
    const PIRect& source = description.SourceRect;
    if (!bitmap.IsValid() || !PImageUtils::IsValid(description) || (source & bitmap.GetBounds()) != source) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const PEColorSpace colorSpace = bitmap.GetColorSpace();
    if (colorSpace != PEColorSpace::RGB16 && colorSpace != PEColorSpace::RGB32 && colorSpace != PEColorSpace::RGBA32) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    description.OpaqueSlices = 0x01ff;
    description.IsOpaque = true;
    if (colorSpace == PEColorSpace::RGBA32)
    {
        const PBitmapRasterRO raster = bitmap.LockRaster();
        if (raster == nullptr) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        const size_t stride = size_t(bitmap.GetBytesPerRow());
        const PRect& slice = description.SliceRect;
        const std::array<int32_t, 4> horizontal = PImageUtils::SourceAxis(source.left, source.right, slice.left, slice.right);
        const std::array<int32_t, 4> vertical = PImageUtils::SourceAxis(source.top, source.bottom, slice.top, slice.bottom);
        for (size_t index = 0; index < 9; ++index)
        {
            const size_t column = index % 3;
            const size_t row = index / 3;
            bool opaque = true;
            for (int32_t y = vertical[row]; y < vertical[row + 1] && opaque; ++y)
            {
                const uint32_t* pixels = reinterpret_cast<const uint32_t*>(raster.Get() + size_t(y) * stride);
                for (int32_t x = horizontal[column]; x < horizontal[column + 1]; ++x)
                {
                    if (PColor::FromRGB32A(pixels[x]).GetAlpha() != 255)
                    {
                        opaque = false;
                        break;
                    }
                }
            }
            if (!opaque) {
                description.OpaqueSlices &= uint16_t(~(0x01 << index));
            }
        }
        description.IsOpaque = description.OpaqueSlices == 0x01ff;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetDescription(PImageDescription description)
{
    if (!PImageUtils::IsValid(description)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (description != m_Description)
    {
        m_Description = description;
        SignalChanged();
    }
}
