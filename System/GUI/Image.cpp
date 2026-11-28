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

PImage::PImage(Ptr<const PBitmap> bitmap, const PIRect& sourceRect)
{
    if (bitmap == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PImageDescription description = DescribeBitmap(*bitmap, sourceRect);
    description.LayoutSize = PPoint(1.0f, 1.0f);
    description.LayoutMode = PImageLayoutMode::ViewRelative;
    m_Data = ptr_new<PImageData>(std::move(bitmap), description);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImage::PImage(Ptr<const PBitmap> bitmap, const PImageDescription& description)
{
    if (bitmap == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PImageDescription classified = description;
    ClassifyOpacity(*bitmap, classified);
    m_Data = ptr_new<PImageData>(std::move(bitmap), classified);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PImageDescription& PImage::GetDescription() const noexcept
{
    static const PImageDescription EMPTY_DESCRIPTION;
    return IsValid() ? m_Data->GetDescription() : EMPTY_DESCRIPTION;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImageID PImage::GetImageID() const
{
    return IsValid() ? m_Data->Publish() : INVALID_IMAGE_ID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetLayoutSize(const PPoint& size)
{
    SetLayoutSize(size, GetLayoutMode());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetLayoutSize(const PPoint& size, PImageLayoutMode mode)
{
    PImageDescription description = GetDescription();
    description.LayoutSize = size;
    description.LayoutMode = mode;
    SetDescription(description);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetSliceRect(const PRect& rectangle)
{
    if (!IsValid()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (rectangle != GetSliceRect())
    {
        PImageDescription description = GetDescription();
        description.SliceRect = rectangle;
        ClassifyOpacity(*GetBitmap(), description);
        SetDescription(description);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImage::SetTiling(PImageTiling tiling)
{
    PImageDescription description = GetDescription();
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

void PImage::SetDescription(const PImageDescription& description)
{
    if (!IsValid() || !PImageUtils::IsValid(description)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (description != GetDescription())
    {
        if (m_Data->GetPtrCount() > 1 || m_Data->IsPublished()) {
            m_Data = ptr_new<PImageData>(GetBitmap(), description);
        } else {
            m_Data->SetDescription(description);
        }
    }
}
