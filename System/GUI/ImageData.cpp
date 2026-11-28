// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <cassert>

#include <App/Application.h>
#include <GUI/ImageData.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImageData::PImageData(Ptr<const PBitmap> bitmap, const PImageDescription& description) : m_Bitmap(std::move(bitmap)), m_Description(description)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImageData::~PImageData()
{
    PApplication* application = m_Bitmap->GetApplication();
    if (m_ImageID != INVALID_IMAGE_ID && application != nullptr) {
        application->DeleteImage(m_ImageID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PImageID PImageData::Publish() const
{
    PApplication* application = m_Bitmap->GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    if (m_ImageID == INVALID_IMAGE_ID) {
        m_ImageID = application->DefineImage(m_Description, m_Bitmap->GetHandle());
    }
    return m_ImageID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PImageData::SetDescription(const PImageDescription& description)
{
    assert(GetPtrCount() == 1 && !IsPublished());
    m_Description = description;
}
