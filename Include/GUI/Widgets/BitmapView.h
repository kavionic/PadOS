// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
