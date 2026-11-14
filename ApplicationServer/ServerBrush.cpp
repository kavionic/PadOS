// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBrush.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerBrush::PServerBrush(PDisplayDriver& driver, const PBrushDescription& description, Ptr<PSrvBitmap> bitmap)
    : m_Bitmap(std::move(bitmap)), m_Description(description)
{
    driver.RegisterBrush(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerBrush::~PServerBrush()
{
    if (m_Driver != nullptr) {
        m_Driver->UnregisterBrush(this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerBrush::SetProperties(const PBrushDescription& description, Ptr<PSrvBitmap> bitmap)
{
    if (description != m_Description || bitmap != m_Bitmap)
    {
        PDisplayBitmap storage;
        if (bitmap != nullptr) {
            storage = {bitmap->m_ColorSpace, bitmap->m_Size, bitmap->m_BytesPerLine, bitmap->m_Raster, bitmap->m_VideoMem};
        }
        if (!PBrushUtils::IsValidResource(description, (bitmap != nullptr) ? &storage : nullptr)) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        m_Driver->UpdateBrush(this, description, bitmap);
        m_Description = description;
        m_Bitmap = std::move(bitmap);
        AdvanceRevision();
        SignalChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerBrush::SetColor(PColor color)
{
    if (m_Description.Type != PBrushType::Solid || m_Description.Style != PBrushStyle::SolidPattern || m_Description.Color != color)
    {
        m_Driver->SetBrushColor(this, color);
        m_Description.SetColor(color);
        m_Bitmap = nullptr;
        AdvanceRevision();
        SignalChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Retire cached revision stamps before reusing a value, including snapshots
/// held by views that have not drawn since the previous counter cycle.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerBrush::AdvanceRevision()
{
    if (++m_Revision == 0)
    {
        m_Driver->HandleBrushRevisionWrap(this);
        m_Revision = 1;
    }
}
