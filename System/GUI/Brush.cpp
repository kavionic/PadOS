// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <App/Application.h>
#include <GUI/Brush.h>
#include <System/ExceptionHandling.h>

const PBrush PBrush::zero;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrush::PBrush(PColor color, PApplication* application)
{
    PBrushData data;
    data.Color = color;
    Create(application, data);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrush::PBrush(PBrushStyle style, PApplication* application)
{
    if (style != PBrushStyle::None && style != PBrushStyle::SolidPattern) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PBrushData data;
    data.Style = style;
    Create(application, data);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrush::PBrush(Ptr<const PBitmap> bitmap) : PBrush(bitmap, (bitmap != nullptr) ? bitmap->GetBounds() : PIRect())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrush::PBrush(Ptr<const PBitmap> bitmap, const PIRect& sourceRect)
{
    PImage image(bitmap, sourceRect);
    image.SetLayoutSize(PPoint(sourceRect.Size()), PImageLayoutMode::Absolute);
    PBrushData data;
    data.Type = PBrushType::Bitmap;
    data.Style = PBrushStyle::TexturePattern;
    Create(image.GetApplication(), data, image);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrush::PBrush(const PImage& image)
{
    if (!image.IsValid()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PBrushData data;
    data.Type = PBrushType::Image;
    data.Style = PBrushStyle::TexturePattern;
    Create(image.GetApplication(), data, image);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBrush::operator==(const PBrush& rhs) const noexcept
{
    return m_Data == rhs.m_Data || (IsValid() && rhs.IsValid() && GetApplication() == rhs.GetApplication()
        && m_Data->GetData() == rhs.m_Data->GetData() && m_Data->GetImage() == rhs.m_Data->GetImage());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication* PBrush::GetApplication() const
{
    return (m_Data != nullptr) ? m_Data->GetApplication() : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBrush::IsOpaque() const noexcept
{
    return m_Data != nullptr && m_Data->GetData().IsOpaque(&m_Data->GetImage().GetDescription());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::SetColor(PColor color)
{
    if (IsValid())
    {
        PBrushData data = m_Data->GetData();
        data.SetColor(color);
        SetProperties(data, PImage());
    }
    else
    {
        *this = PBrush(color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::SetBitmap(Ptr<const PBitmap> bitmap)
{
    SetBitmap(bitmap, (bitmap != nullptr) ? bitmap->GetBounds() : PIRect());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::SetBitmap(Ptr<const PBitmap> bitmap, const PIRect& sourceRect)
{
    if (bitmap == nullptr || (IsValid() && bitmap->GetApplication() != GetApplication())) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PImage image(bitmap, sourceRect);
    image.SetLayoutSize(PPoint(sourceRect.Size()), PImageLayoutMode::Absolute);
    PBrushData data = (m_Data != nullptr) ? m_Data->GetData() : PBrushData();
    data.Type = PBrushType::Bitmap;
    data.Style = PBrushStyle::TexturePattern;
    SetProperties(data, image);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::SetImage(const PImage& image)
{
    if (!image.IsValid() || (IsValid() && image.GetApplication() != GetApplication())) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PBrushData data = (m_Data != nullptr) ? m_Data->GetData() : PBrushData();
    data.Type = PBrushType::Image;
    data.Style = PBrushStyle::TexturePattern;
    SetProperties(data, image);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::SetStyle(PBrushStyle style)
{
    switch (style)
    {
        case PBrushStyle::None:
        case PBrushStyle::SolidPattern:
            break;
        case PBrushStyle::TexturePattern:
            if (!GetImage().IsValid()) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PBrushData data = (m_Data != nullptr) ? m_Data->GetData() : PBrushData();
    data.Style = style;
    SetProperties(data, GetImage());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::SetScale(const PPoint& scale)
{
    PBrushData data = (m_Data != nullptr) ? m_Data->GetData() : PBrushData();
    if (m_Data == nullptr) {
        data.Style = PBrushStyle::None;
    }
    data.Scale = scale;
    SetProperties(data, GetImage());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrushData PBrush::GetServerData() const
{
    PBrushData data;
    data.Style = PBrushStyle::None;
    if (m_Data != nullptr)
    {
        data = m_Data->GetData();
        if (data.Style == PBrushStyle::TexturePattern) {
            data.ImageID = m_Data->GetImage().GetImageID();
        }
    }
    return data;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::Create(PApplication* application, const PBrushData& data, const PImage& image)
{
    if (!PBrushUtils::IsValidScale(data, image.GetDescription())) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (application == nullptr) {
        application = PApplication::GetCurrentApplication();
    }
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    assert(!application->IsRunning() || application->GetMutex().IsLocked());
    m_Data = ptr_new<PBrushSharedData>(application, data, image);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBrush::SetProperties(const PBrushData& data, const PImage& image)
{
    if (!PBrushUtils::IsValidScale(data, image.GetDescription())) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (!IsValid())
    {
        Create(image.GetApplication(), data, image);
    }
    else if (data != m_Data->GetData() || image != m_Data->GetImage())
    {
        PApplication* application = GetApplication();
        if (application == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NOENT);
        }
        assert(!application->IsRunning() || application->GetMutex().IsLocked());
        if (m_Data->GetPtrCount() > 1) {
            m_Data = ptr_new<PBrushSharedData>(application, data, image);
        } else {
            m_Data->SetProperties(data, image);
        }
    }
}
