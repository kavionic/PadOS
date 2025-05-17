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
#include <DataTranslation/DataTranslator.h>

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

BitmapView::BitmapView(const String& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::WillDraw)
{
    SetDrawingMode(DrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

BitmapView::BitmapView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : View(context, parent, xmlData)
{
    MergeFlags(ViewFlags::WillDraw);
    SetDrawingMode(DrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void BitmapView::OnPaint(const Rect& updateRect)
{
    if (m_Bitmap != nullptr) {
        DrawBitmap(m_Bitmap, GetNormalizedBounds(), Point(0.0f, 0.0f));
    } else {
        EraseRect(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void BitmapView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    if (m_Bitmap != nullptr)
    {
        *minSize = Point(m_Bitmap->GetBounds().Size());
        *maxSize = *minSize;
    }
    else
    {
        View::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool BitmapView::LoadBitmap(StreamableIO& file)
{
    m_Bitmap = nullptr;

    std::vector<uint8_t> buffer;

    ssize_t positionIn = 0;

    Ptr<DataTranslator> translator;

    for (;;)
    {
        if (positionIn + 1024 > 32768) {
            return false; // Failed to identify file type.
        }

        buffer.resize(positionIn + 1024);

        const size_t  bytesToRead = buffer.size() - positionIn;
        const ssize_t bytesRead = file.Read(buffer.data() + positionIn, bytesToRead);

        if (bytesRead < 0) {
            return false;
        }

        positionIn += bytesRead;

        const bool finalRead = bytesRead < bytesToRead;

        if (translator == nullptr)
        {
            TranslatorFactory& factory = TranslatorFactory::Get();

            EDataTranslatorStatus status;
            translator = factory.FindTranslator(String::zero, EDataTranslatorType::Image, buffer.data(), bytesRead, status);

            if (translator == nullptr)
            {
                if (finalRead) {
                    return false;
                }
                continue;
            }
            translator->VFDataReady.Connect(this, &BitmapView::SlotImageDataReady);
        }
        translator->AddData(buffer.data(), bytesRead, finalRead);
        buffer.resize(1024);
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

void BitmapView::SetBitmap(Ptr<Bitmap> bitmap)
{
    m_Bitmap = bitmap;
    PreferredSizeChanged();
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Bitmap> BitmapView::GetBitmap() const
{
    return m_Bitmap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool BitmapView::SlotImageDataReady(const void* data, size_t length, bool isFinal)
{
    if (m_Bitmap == nullptr)
    {
        BitmapHeader bmHeader;
        if (length < sizeof(bmHeader)) {
            return false;
        }
        memcpy(&bmHeader, data, sizeof(bmHeader));

        m_Bitmap = ptr_new<Bitmap>(bmHeader.Bounds.Width(), bmHeader.Bounds.Height(), bmHeader.ColorSpace);
        memset(m_Bitmap->LockRaster(), -1, m_Bitmap->GetBytesPerRow() * m_Bitmap->GetBounds().Height());

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
            if (m_BytesAddedToRow == m_CurrentFrame.BytesPerRow) {
                m_BytesAddedToFrame += m_BytesAddedToRow;
                m_BytesAddedToRow = 0;
            }
        }
    }
    return true;
}

} // namespace os
