// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ApplicationServer/ServerBitmap.h>
#include <GUI/ImageTypes.h>
#include <Ptr/Ptr.h>

class PServerImage : public PtrTarget
{
public:
    PServerImage(PDisplayDriver& driver, const PImageDescription& description, Ptr<PSrvBitmap> bitmap);
    virtual ~PServerImage() override;

    const PImageDescription& GetDescription() const noexcept { return m_Description; }
    const Ptr<PSrvBitmap>& GetBitmap() const noexcept { return m_Bitmap; }

    uint32_t m_RenderSequence = 0;
    size_t m_ResourceIndex = 0;
    PDisplayDriver* m_Driver = nullptr;
    PDriverResourceID m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;

private:
    Ptr<PSrvBitmap> m_Bitmap;
    PImageDescription m_Description;
};
