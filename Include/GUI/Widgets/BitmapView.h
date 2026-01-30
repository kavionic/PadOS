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


class PBitmap;
class PStreamableIO;
class PPath;


class PBitmapView : public PView
{
public:
    PBitmapView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PBitmapView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    virtual void OnPaint(const PRect& updateRect) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void OnFrameSized(const PPoint& delta) override;

    bool LoadBitmap(const PPath& path);
    bool LoadBitmap(PStreamableIO& file);

    void SetBitmap(Ptr<PBitmap> bitmap);
    Ptr<PBitmap> GetBitmap() const;

    void ClearBitmap();

    void SetScale(const PPoint& scale);
    PPoint GetScale() const { return m_Scale; }

private:
    void UpdateIsScaled();
    bool SlotImageDataReady(const void* data, size_t length, bool isFinal);

    PBitmapFrameHeader m_CurrentFrame;
    ssize_t     m_CurrentFrameByteSize = -1;
    size_t      m_BytesAddedToRow = 0;
    size_t      m_BytesAddedToFrame = 0;

    PPoint m_Scale = PPoint(1.0f, 1.0f);
    bool   m_IsScaled = false;

    Ptr<PBitmap> m_Bitmap;
};
