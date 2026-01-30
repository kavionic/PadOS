// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 09.05.2025 22:30

#include <GUI/Widgets/BitmapView.h>
#include <GUI/Bitmap.h>
#include <Storage/StreamableIO.h>
#include <Storage/File.h>
#include <Storage/Path.h>
#include <DataTranslation/DataTranslator.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBitmapView::PBitmapView(const PString& name, Ptr<PView> parent, uint32_t flags) : PView(name, parent, flags | PViewFlags::WillDraw)
{
    SetDrawingMode(PDrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBitmapView::PBitmapView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PView(context, parent, xmlData)
{
    MergeFlags(PViewFlags::WillDraw);
    SetDrawingMode(PDrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBitmapView::OnPaint(const PRect& updateRect)
{
    if (m_Bitmap != nullptr)
    {
        if (m_IsScaled) {
            DrawBitmap(m_Bitmap, m_Bitmap->GetBounds(), GetNormalizedBounds());
        } else {
            DrawBitmap(m_Bitmap, m_Bitmap->GetBounds(), PPoint(0.0f, 0.0f));
        }
    }
    else
    {
        EraseRect(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBitmapView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    if (m_Bitmap != nullptr)
    {
        *minSize = PPoint(m_Bitmap->GetBounds().Size()) * m_Scale;
        *maxSize = *minSize;
    }
    else
    {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBitmapView::OnFrameSized(const PPoint& delta)
{
    PView::OnFrameSized(delta);
    UpdateIsScaled();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapView::LoadBitmap(PStreamableIO& file)
{
    constexpr size_t BUFFER_SIZE = 4096;
    m_Bitmap = nullptr;

    std::vector<uint8_t> buffer;

    ssize_t positionIn = 0;

    Ptr<PDataTranslator> translator;

    for (;;)
    {
        if (positionIn + BUFFER_SIZE > 32768) {
            return false; // Failed to identify file type.
        }

        buffer.resize(positionIn + BUFFER_SIZE);

        const size_t  bytesToRead = buffer.size() - positionIn;
        const ssize_t bytesRead = file.Read(buffer.data() + positionIn, bytesToRead);

        if (bytesRead < 0) {
            return false;
        }

        positionIn += bytesRead;

        const bool finalRead = bytesRead < bytesToRead;

        if (translator == nullptr)
        {
            PTranslatorFactory& factory = PTranslatorFactory::Get();

            PDataTranslatorStatus status;
            translator = factory.FindTranslator(PString::zero, PDataTranslatorType::Image, buffer.data(), bytesRead, status);

            if (translator == nullptr)
            {
                if (finalRead) {
                    return false;
                }
                continue;
            }
            translator->VFDataReady.Connect(this, &PBitmapView::SlotImageDataReady);
        }
        translator->AddData(buffer.data(), bytesRead, finalRead);
        buffer.resize(BUFFER_SIZE);
        positionIn = 0;

        if (finalRead)
        {
            if (translator == NULL)
            {
                return false;;
            }
            else
            {
                translator = nullptr;
                Invalidate();
                return true;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapView::LoadBitmap(const PPath& path)
{
    PFile file(path);
    if (file.IsValid()) {
        return LoadBitmap(file);
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBitmapView::SetBitmap(Ptr<PBitmap> bitmap)
{
    m_Bitmap = bitmap;
    UpdateIsScaled();
    PreferredSizeChanged();
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PBitmap> PBitmapView::GetBitmap() const
{
    return m_Bitmap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBitmapView::ClearBitmap()
{
    SetBitmap(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBitmapView::SetScale(const PPoint& scale)
{
    if (scale != m_Scale)
    {
        m_Scale = scale;
        if (m_Bitmap != nullptr) {
            PreferredSizeChanged();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PBitmapView::UpdateIsScaled()
{
    m_IsScaled = m_Bitmap != nullptr && PIRect(m_Bitmap->GetBounds()).Size() != PIRect(GetBounds()).Size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapView::SlotImageDataReady(const void* data, size_t length, bool isFinal)
{
    if (m_Bitmap == nullptr)
    {
        PBitmapHeader bmHeader;
        if (length < sizeof(bmHeader)) {
            return false;
        }
        memcpy(&bmHeader, data, sizeof(bmHeader));

        m_Bitmap = ptr_new<PBitmap>(bmHeader.Bounds.Width(), bmHeader.Bounds.Height(), bmHeader.ColorSpace);
        memset(m_Bitmap->LockRaster(), -1, m_Bitmap->GetBytesPerRow() * m_Bitmap->GetBounds().Height());

        UpdateIsScaled();
        PreferredSizeChanged();

        m_CurrentFrameByteSize = -1;
    }
    else if (m_CurrentFrameByteSize < 0)
    {
        if (length < sizeof(m_CurrentFrame)) {
            return false;
        }
        memcpy(&m_CurrentFrame, data, sizeof(m_CurrentFrame));
        m_CurrentFrameByteSize = m_CurrentFrame.DataSize;
        m_BytesAddedToFrame = 0;
        m_BytesAddedToRow = 0;
    }
    else
    {
        uint8_t* const dstRaster = m_Bitmap->LockRaster();

        size_t bytesInBuffer = std::min(length, m_CurrentFrameByteSize - m_BytesAddedToFrame);

        while (bytesInBuffer > 0)
        {
            const size_t rowBytes = std::min(bytesInBuffer, m_CurrentFrame.BytesPerRow - m_BytesAddedToRow);
            memcpy(dstRaster + m_BytesAddedToFrame + m_BytesAddedToRow, data, rowBytes);
            m_BytesAddedToRow += rowBytes;
            bytesInBuffer -= rowBytes;
            if (m_BytesAddedToRow == m_CurrentFrame.BytesPerRow)
            {
                m_BytesAddedToFrame += m_BytesAddedToRow;
                m_BytesAddedToRow = 0;
            }
        }
    }
    return true;
}
