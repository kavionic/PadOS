// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Brush.h>
#include <App/Application.h>
#include <System/ExceptionHandling.h>

PBrush::PBrush(PColor color, PApplication* application)
{
    m_Description.Color = color;
    Create(application);
}

PBrush::PBrush(PBrushStyle style, PApplication* application)
{
    if (style != PBrushStyle::None && style != PBrushStyle::SolidPattern) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    m_Description.Style = style;
    Create(application);
}

PBrush::PBrush(Ptr<const PBitmap> bitmap) : PBrush(bitmap, (bitmap != nullptr) ? bitmap->GetBounds() : PIRect())
{
}

PBrush::PBrush(Ptr<const PBitmap> bitmap, const PIRect& sourceRect) : m_Bitmap(std::move(bitmap))
{
    if (m_Bitmap == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    m_Description.Image = PImage::DescribeBitmap(*m_Bitmap, sourceRect);
    m_Description.Type = PBrushType::Bitmap;
    m_Description.Style = PBrushStyle::TexturePattern;
    Create(m_Bitmap->GetApplication());
}

PBrush::PBrush(Ptr<const PImage> image) : m_Image(std::move(image))
{
    if (m_Image == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    m_Bitmap = m_Image->GetBitmap();
    m_Description.Image = m_Image->GetDescription();
    m_Description.Type = PBrushType::Image;
    m_Description.Style = PBrushStyle::TexturePattern;
    m_Image->SignalChanged.Connect(this, &PBrush::ImageChanged);
    Create(m_Image->GetApplication());
}

PBrush::~PBrush()
{
    PApplication* application = GetApplication();
    if (application != nullptr) {
        application->DeleteBrush(m_Handle);
    }
}

PApplication* PBrush::GetApplication() const
{
    return ptr_raw_pointer_cast(m_Application);
}

void PBrush::SetColor(PColor color)
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    if (m_Description.Type != PBrushType::Solid || m_Description.Style != PBrushStyle::SolidPattern || m_Description.Color != color)
    {
        application->SetBrushColor(m_Handle, color);
        if (m_Image != nullptr) {
            m_Image->SignalChanged.Disconnect(this, &PBrush::ImageChanged);
        }
        m_Description.SetColor(color);
        m_Image = nullptr;
        m_Bitmap = nullptr;
    }
}

void PBrush::SetBitmap(Ptr<const PBitmap> bitmap)
{
    SetBitmap(bitmap, (bitmap != nullptr) ? bitmap->GetBounds() : PIRect());
}

void PBrush::SetBitmap(Ptr<const PBitmap> bitmap, const PIRect& sourceRect)
{
    if (bitmap == nullptr || bitmap->GetApplication() != GetApplication()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PBrushDescription description = m_Description;
    description.Image = PImage::DescribeBitmap(*bitmap, sourceRect);
    description.Type = PBrushType::Bitmap;
    description.Style = PBrushStyle::TexturePattern;
    SetProperties(description, std::move(bitmap), nullptr);
}

void PBrush::SetImage(Ptr<const PImage> image)
{
    if (image == nullptr || image->GetApplication() != GetApplication()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PBrushDescription description = m_Description;
    description.Image = image->GetDescription();
    description.Type = PBrushType::Image;
    description.Style = PBrushStyle::TexturePattern;
    Ptr<const PBitmap> bitmap = image->GetBitmap();
    SetProperties(description, std::move(bitmap), std::move(image));
}

void PBrush::SetStyle(PBrushStyle style)
{
    switch (style)
    {
        case PBrushStyle::None:
        case PBrushStyle::SolidPattern:
            break;
        case PBrushStyle::TexturePattern:
            if (m_Bitmap == nullptr) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PBrushDescription description = m_Description;
    description.Style = style;
    SetProperties(description, m_Bitmap, m_Image);
}

void PBrush::SetScale(const PPoint& scale)
{
    PBrushDescription description = m_Description;
    description.Scale = scale;
    SetProperties(description, m_Bitmap, m_Image);
}

void PBrush::Create(PApplication* application)
{
    if (application == nullptr) {
        application = PApplication::GetCurrentApplication();
    }
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    m_Application = ptr_weak_cast(application);
    m_Handle = application->CreateBrush(m_Description, (m_Bitmap != nullptr) ? m_Bitmap->GetHandle() : INVALID_HANDLE);
}

// Publish image metadata only when the shared image changes. Existing brush
// handles remain valid, and the server invalidates their background dependents.
void PBrush::ImageChanged()
{
    PBrushDescription description = m_Description;
    description.Image = m_Image->GetDescription();
    SetProperties(description, m_Bitmap, m_Image);
}

void PBrush::SetProperties(const PBrushDescription& description, Ptr<const PBitmap> bitmap, Ptr<const PImage> image)
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    if (!PBrushUtils::IsValidScale(description)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (description != m_Description || bitmap != m_Bitmap || image != m_Image)
    {
        const bool connectImage = image != nullptr && image != m_Image;
        if (connectImage) {
            image->SignalChanged.Connect(this, &PBrush::ImageChanged);
        }
        PScopeFail disconnectImage([this, &image, connectImage]
        {
            if (connectImage) {
                image->SignalChanged.Disconnect(this, &PBrush::ImageChanged);
            }
        });
        application->UpdateBrush(m_Handle, description, (bitmap != nullptr) ? bitmap->GetHandle() : INVALID_HANDLE);
        if (m_Image != nullptr && image != m_Image) {
            m_Image->SignalChanged.Disconnect(this, &PBrush::ImageChanged);
        }
        m_Description = description;
        m_Bitmap = std::move(bitmap);
        m_Image = std::move(image);
    }
}
