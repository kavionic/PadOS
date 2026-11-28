// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <utility>

#include <ApplicationServer/ServerBrush.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerBrush::PServerBrush() noexcept
{
    m_Data.Style = PBrushStyle::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerBrush::PServerBrush(PColor color) noexcept
{
    m_Data.Color = color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerBrush::PServerBrush(const PBrushData& data, Ptr<PServerImage> image)
    : m_Data(data), m_Image(std::move(image))
{
    switch (data.Style)
    {
        case PBrushStyle::None:
        case PBrushStyle::SolidPattern:
            break;
        case PBrushStyle::TexturePattern:
            if (m_Image == nullptr || !PBrushUtils::IsValidScale(data, m_Image->GetDescription())) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}
