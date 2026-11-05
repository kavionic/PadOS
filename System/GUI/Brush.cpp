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
    if (application == nullptr) {
        application = PApplication::GetCurrentApplication();
    }
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    m_Application = ptr_weak_cast(application);
    m_Description.Color = color;
    m_Description.IsOpaque = color.GetAlpha() == 255;
    m_Handle = application->CreateBrush(m_Description, INVALID_HANDLE);
}

PBrush::PBrush(Ptr<const PBitmap> bitmap, const PIRect& sourceRect, PBrushType type, PBrushBorders borders)
    : m_Bitmap(std::move(bitmap))
{
    if (m_Bitmap == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    m_Description = DescribeBitmap(*m_Bitmap, sourceRect, type, borders);
    PApplication* application = m_Bitmap->GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    m_Application = ptr_weak_cast(application);
    m_Handle = application->CreateBrush(m_Description, m_Bitmap->GetHandle());
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
    if (m_Description.Type != PBrushType::Solid || m_Description.Color != color)
    {
        application->SetBrushColor(m_Handle, color);
        m_Description.SetColor(color);
        m_Bitmap = nullptr;
    }
}

void PBrush::SetBitmap(Ptr<const PBitmap> bitmap, const PIRect& sourceRect, PBrushType type, PBrushBorders borders)
{
    if (bitmap == nullptr || bitmap->GetApplication() != GetApplication()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const PBrushDescription description = DescribeBitmap(*bitmap, sourceRect, type, borders);
    SetProperties(description, std::move(bitmap));
}

// Validate and classify artwork once when its properties change. Drawing never
// rescans alpha values or repeats the source-rectangle validation.
PBrushDescription PBrush::DescribeBitmap(const PBitmap& bitmap, const PIRect& sourceRect, PBrushType type, PBrushBorders borders)
{
    PBrushDescription description;
    if (!bitmap.IsValid() || !sourceRect.IsValid() || (sourceRect & bitmap.GetBounds()) != sourceRect
        || (type != PBrushType::BitmapStretch && type != PBrushType::BitmapTile && type != PBrushType::NineSlice)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const PEColorSpace colorSpace = bitmap.GetColorSpace();
    if (colorSpace != PEColorSpace::RGB16 && colorSpace != PEColorSpace::RGB32 && colorSpace != PEColorSpace::RGBA32) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (borders.Left < 0 || borders.Top < 0 || borders.Right < 0 || borders.Bottom < 0
        || int64_t(borders.Left) + borders.Right >= sourceRect.Width() || int64_t(borders.Top) + borders.Bottom >= sourceRect.Height()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    description.SourceRect = sourceRect;
    description.Borders = borders;
    description.Type = type;
    if (colorSpace == PEColorSpace::RGBA32)
    {
        description.OpaqueSlices = 0;
        const size_t sliceCount = (type == PBrushType::NineSlice) ? 9 : 1;
        const std::array<int32_t, 4> horizontal = PBrushUtils::SliceAxis(sourceRect.Width(), borders.Left, borders.Right);
        const std::array<int32_t, 4> vertical = PBrushUtils::SliceAxis(sourceRect.Height(), borders.Top, borders.Bottom);
        for (size_t index = 0; index < sliceCount; ++index)
        {
            const size_t column = index % 3;
            const size_t row = index / 3;
            const PIRect slice = (type == PBrushType::NineSlice)
                ? PIRect(horizontal[column], vertical[row], horizontal[column + 1], vertical[row + 1]) + sourceRect.TopLeft() : sourceRect;
            bool opaque = true;
            for (int32_t y = slice.top; y < slice.bottom && opaque; ++y)
            {
                const uint32_t* pixels = reinterpret_cast<const uint32_t*>(bitmap.GetRaster() + size_t(y) * bitmap.GetBytesPerRow());
                for (int32_t x = slice.left; x < slice.right; ++x)
                {
                    if (PColor::FromRGB32A(pixels[x]).GetAlpha() != 255)
                    {
                        opaque = false;
                        break;
                    }
                }
            }
            if (opaque) {
                description.OpaqueSlices |= uint16_t(0x01 << index);
            }
        }
        description.IsOpaque = description.OpaqueSlices == ((type == PBrushType::NineSlice) ? 0x01ff : 0x01);
    }
    return description;
}

void PBrush::SetProperties(const PBrushDescription& description, Ptr<const PBitmap> bitmap)
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    if (description != m_Description || bitmap != m_Bitmap)
    {
        application->UpdateBrush(m_Handle, description, (bitmap != nullptr) ? bitmap->GetHandle() : INVALID_HANDLE);
        m_Description = description;
        m_Bitmap = std::move(bitmap);
    }
}
