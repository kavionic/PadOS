// This file is part of PadOS.
//
// Copyright (C) 2001-2025 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include <stdio.h>

#include <DataTranslation/DataTranslator.h>

#include <png.h>

class PNGTranslator : public os::DataTranslator
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

    os::BitmapHeader        m_BitmapHeader;
    os::BitmapFrameHeader   m_CurrentFrame;

    int            m_NumPasses = 1;
    bool           m_IsInterlaced = false;

    png_structp    m_PNGStruct = nullptr;
    png_infop      m_PNGInfo = nullptr;
};


class PNGTranslatorNode : public os::TranslatorNode
{
public:
    virtual os::EDataTranslatorStatus   Identify(const os::String& srcType, os::EDataTranslatorType dstType, const void* data, size_t length) const override;
    virtual os::TranslatorInfo          GetTranslatorInfo() const override;
    virtual Ptr<os::DataTranslator>     CreateTranslator() const override;
};
