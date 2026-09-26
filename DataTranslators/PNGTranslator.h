// This file is part of PadOS.
//
// Copyright (c) 2001-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdio.h>

#include <DataTranslation/DataTranslator.h>

#include <png.h>

class PNGTranslator : public PDataTranslator
{
public:
    PNGTranslator();
    virtual ~PNGTranslator();

    virtual status_t AddData(const void* data, size_t length, bool isFinal) override;
    virtual void     Abort() override;
    virtual void     Reset() override;

private:
    void InfoCallback(png_infop info);
    void RowCallback(int pass, int y, png_bytep row);
    void EndCallback();

    static void InfoCallback_s(png_structp pngPtr, png_infop info)
    {
        static_cast<PNGTranslator*>(png_get_progressive_ptr(pngPtr))->InfoCallback(info);
    }

    static void RowCallback_s(png_structp pngPtr, png_bytep newRow, png_uint_32 rowNum, int pass)
    {
        static_cast<PNGTranslator*>(png_get_progressive_ptr(pngPtr))->RowCallback(pass, rowNum, newRow);
    }

    static void EndCallback_s(png_structp pngPtr, png_infop info)
    {
        static_cast<PNGTranslator*>(png_get_progressive_ptr(pngPtr))->EndCallback();
    }

    PBitmapHeader        m_BitmapHeader;
    PBitmapFrameHeader   m_CurrentFrame;

    int            m_NumPasses = 1;
    bool           m_IsInterlaced = false;

    png_structp    m_PNGStruct = nullptr;
    png_infop      m_PNGInfo = nullptr;
};


class PNGTranslatorNode : public PTranslatorNode
{
public:
    virtual PDataTranslatorStatus   Identify(const PString& srcType, PDataTranslatorType dstType, const void* data, size_t length) const override;
    virtual PTranslatorInfo          GetTranslatorInfo() const override;
    virtual Ptr<PDataTranslator>     CreateTranslator() const override;
};
