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

#pragma once

#include <GUI/View.h>
#include <DataTranslation/DataTranslator.h>

namespace os
{

class StreamableIO;
class Path;
class Bitmap;

class BitmapView : public View
{
public:
    BitmapView(const String& name = String::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    BitmapView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    virtual void OnPaint(const Rect& updateRect) override;
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void OnFrameSized(const Point& delta) override;

    bool LoadBitmap(const Path& path);
    bool LoadBitmap(StreamableIO& file);

    void SetBitmap(Ptr<Bitmap> bitmap);
    Ptr<Bitmap> GetBitmap() const;

    void ClearBitmap();

    void SetScale(const Point& scale);
    Point GetScale() const { return m_Scale; }

private:
    void UpdateIsScaled();
    bool SlotImageDataReady(const void* data, size_t length, bool isFinal);

    os::BitmapFrameHeader m_CurrentFrame;
    ssize_t     m_CurrentFrameByteSize = -1;
    size_t      m_BytesAddedToRow = 0;
    size_t      m_BytesAddedToFrame = 0;

    Point m_Scale = Point(1.0f, 1.0f);
    bool  m_IsScaled = false;

    Ptr<Bitmap> m_Bitmap;
};

} // namespace os
