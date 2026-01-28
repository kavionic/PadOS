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


#include <deque>
#include <stdio.h>
#include <setjmp.h>

#include <Utils/FactoryAutoRegistrator.h>

#include "PNGTranslator.h"

using namespace os;

REGISTER_DATA_TRANSLATOR(PNGTranslatorNode);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PNGTranslator::PNGTranslator()
{
    m_PNGStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

   if (m_PNGStruct == nullptr) {
       throw std::bad_alloc();
   }

   m_PNGInfo = png_create_info_struct(m_PNGStruct);

   if (m_PNGInfo == nullptr)
   {
      png_destroy_read_struct(&m_PNGStruct, &m_PNGInfo, nullptr);
      throw std::bad_alloc();
   }

   if (setjmp(png_jmpbuf(m_PNGStruct)))
   {
      png_destroy_read_struct(&m_PNGStruct, &m_PNGInfo, nullptr);
      throw std::bad_alloc();
   }
   
   png_set_progressive_read_fn(m_PNGStruct, this, InfoCallback_s, RowCallback_s, EndCallback_s);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PNGTranslator::~PNGTranslator()
{
    if (m_PNGStruct != nullptr) {
        png_destroy_read_struct(&m_PNGStruct, &m_PNGInfo, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PNGTranslator::InfoCallback(png_infop info)
{
    png_uint_32 width;
    png_uint_32 height;
    int bitDepth;
    int colorType;
    int interlaceType;
    
    png_get_IHDR(m_PNGStruct, m_PNGInfo, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

    m_BitmapHeader.HeaderSize   = sizeof(m_BitmapHeader);
    m_BitmapHeader.DataSize     = 0;
    m_BitmapHeader.Bounds       = IRect(0, 0, width, height);
    m_BitmapHeader.FrameCount   = 1;
    m_BitmapHeader.BytesPerRow  = width * 4;
    m_BitmapHeader.ColorSpace   = EColorSpace::RGBA32;

    AddProcessedData(&m_BitmapHeader, sizeof(m_BitmapHeader), false);

    m_CurrentFrame.HeaderSize   = sizeof(m_CurrentFrame);
    m_CurrentFrame.Timestamp    = 0;
    m_CurrentFrame.ColorSpace   = m_BitmapHeader.ColorSpace;
    m_CurrentFrame.Frame        = m_BitmapHeader.Bounds;
    m_CurrentFrame.BytesPerRow  = m_CurrentFrame.Frame.Width() * 4;
    m_CurrentFrame.DataSize     = m_CurrentFrame.BytesPerRow * m_CurrentFrame.Frame.Height();
    
    if (interlaceType == PNG_INTERLACE_NONE)
    {
        AddProcessedData(&m_CurrentFrame, sizeof(m_CurrentFrame), false);
        m_IsInterlaced = false;
    }
    else
    {
        m_IsInterlaced = true;
    }
    
    // Expand paletted colors into true RGB triplets.
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_expand(m_PNGStruct);
    }

    // expand paletted or RGB images with transparency to full alpha channels
    // so the data will be available as RGBA quartets.
    if (png_get_valid(m_PNGStruct, m_PNGInfo, PNG_INFO_tRNS)) {
        png_set_expand(m_PNGStruct);
    }
    // Set the background color to draw transparent and alpha images over.
    // It is possible to set the red, green, and blue components directly
    // for paletted images instead of supplying a palette index. Note that
    // even if the PNG file supplies a background, you are not required to
    // use it - you should use the (solid) application background if it has one.

//    png_color_16* image_background;
//    if ( png_get_bKGD(m_PNGStruct, m_PNGInfo, &image_background ) ) {
//        png_set_background( m_PNGStruct, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0 );
//    }

    const double screenGamma = 2.2;  // A good guess for PC monitors.

    double imageGamma;
    if (png_get_gAMA(m_PNGStruct, m_PNGInfo, &imageGamma)) {
        png_set_gamma(m_PNGStruct, screenGamma, imageGamma);
    } else {
        png_set_gamma(m_PNGStruct, screenGamma, 0.45455);
    }

    // flip the RGB pixels to BGR (or RGBA to BGRA).
    png_set_bgr(m_PNGStruct);

    // swap bytes of 16 bit files to least significant byte first.
//  png_set_swap( m_psPNGStruct );

    // Add filler (or alpha) byte (before/after each RGB triplet).
    png_set_filler(m_PNGStruct, 0xff, PNG_FILLER_AFTER);

    png_set_gray_to_rgb(m_PNGStruct);

    // Turn on interlace handling.  REQUIRED if you are not using
    // png_read_image().  To see how to handle interlacing passes,
    // see the png_read_row() method below.

    m_NumPasses = png_set_interlace_handling(m_PNGStruct);

    // optional call to gamma correct and add the background to the palette
    // and update info structure.  REQUIRED if you are expecting libpng to
    // update the palette for you (ie you selected such a transform above).

    png_read_update_info(m_PNGStruct, m_PNGInfo);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PNGTranslator::RowCallback(int nPass, int y, png_bytep pRow)
{
    if (pRow == nullptr) {
        return;
    }
    if (m_IsInterlaced)
    {
//        if (old_row != nullptr && new_row != nullptr) {
//            png_progressive_combine_row(png_ptr, old_row, new_row);
//        }
        m_CurrentFrame.Frame.top    = y;
        m_CurrentFrame.Frame.bottom = y + 1;
        m_CurrentFrame.DataSize     = m_CurrentFrame.BytesPerRow;

        AddProcessedData(&m_CurrentFrame, sizeof(m_CurrentFrame), false);
    }
    AddProcessedData(pRow, m_BitmapHeader.BytesPerRow, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PNGTranslator::EndCallback()
{
    AddProcessedData(nullptr, 0, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t PNGTranslator::AddData(const void* data, size_t length, bool isFinal)
{
   if (setjmp(png_jmpbuf(m_PNGStruct)))
   {
      png_destroy_read_struct(&m_PNGStruct, &m_PNGInfo, nullptr);
      return -1;
   }
   png_process_data(m_PNGStruct, m_PNGInfo, (png_bytep)data, length);
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PNGTranslator::Abort()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PNGTranslator::Reset()
{
}



///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

os::EDataTranslatorStatus PNGTranslatorNode::Identify(const PString& srcType, EDataTranslatorType dstType, const void* data, size_t length) const
{
    if (dstType != EDataTranslatorType::Unknown && dstType != EDataTranslatorType::Image) {
        return EDataTranslatorStatus::UnsupportedType;
    }
    if (length < 8) {
        return EDataTranslatorStatus::NotEnoughData;
    }
    if (png_sig_cmp(static_cast<const png_byte*>(data), 0, length) == 0) {
        return EDataTranslatorStatus::Success;
    }
    return EDataTranslatorStatus::UnknownFormat;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TranslatorInfo PNGTranslatorNode::GetTranslatorInfo() const
{
    static TranslatorInfo info = { "image/png", EDataTranslatorType::Image, 1.0f };
    return info;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<DataTranslator> PNGTranslatorNode::CreateTranslator() const
{
    try
    {
        return ptr_new<PNGTranslator>();
    }
    catch (std::bad_alloc& e)
    {
        return nullptr;
    }
}
