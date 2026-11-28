// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <utility>

#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerImage.h>
#include <DeviceControl/DisplayBrush.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerImage::PServerImage(PDisplayDriver& driver, const PImageDescription& description, Ptr<PSrvBitmap> bitmap)
    : m_Bitmap(std::move(bitmap)), m_Description(description)
{
    PDisplayBitmap storage;
    if (m_Bitmap != nullptr) {
        storage = {m_Bitmap->m_ColorSpace, m_Bitmap->m_Size, m_Bitmap->m_BytesPerLine, m_Bitmap->m_Raster, m_Bitmap->m_VideoMem};
    }
    if (!PImageUtils::IsValidResource(description, (m_Bitmap != nullptr) ? &storage : nullptr)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    driver.RegisterImage(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerImage::~PServerImage()
{
    if (m_Driver != nullptr) {
        m_Driver->UnregisterImage(this);
    }
}
