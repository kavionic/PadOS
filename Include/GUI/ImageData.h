// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Bitmap.h>
#include <GUI/ImageTypes.h>
#include <Ptr/Ptr.h>

// Shared by image values. Published definitions remain immutable until released.
class PImageData : public PtrTarget
{
public:
    PImageData(Ptr<const PBitmap> bitmap, const PImageDescription& description);
    virtual ~PImageData() override;

    const Ptr<const PBitmap>&   GetBitmap() const noexcept { return m_Bitmap; }
    const PImageDescription&    GetDescription() const noexcept { return m_Description; }
    bool                        IsPublished() const noexcept { return m_ImageID != INVALID_IMAGE_ID; }
    PImageID                    Publish() const;
    void                        SetDescription(const PImageDescription& description);

private:
    Ptr<const PBitmap>  m_Bitmap;
    PImageDescription   m_Description;
    mutable PImageID    m_ImageID = INVALID_IMAGE_ID;
};
