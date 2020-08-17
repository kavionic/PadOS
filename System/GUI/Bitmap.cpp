// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/Bitmap.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap(int width, int height, color_space colorSpace, uint32_t nFlags)
{
    //    area_id   hArea;
    m_Bounds = Rect(0, 0, float(width), float(height));

    m_Handle = -1;
    m_Raster = NULL;
    m_ColorSpace = colorSpace;

    //    int nError = Application::GetInstance()->CreateBitmap( width, height, colorSpace, nFlags, &m_Handle, &hArea );

    //    if ( nError < 0 ) {
    //  throw( GeneralFailure( "Application server failed to create bitmap", -nError ) );
    //    }

    //    if ( nFlags & SHARE_FRAMEBUFFER ) {
    //  m_hArea = clone_area( "bm_clone", (void**) &m_Raster, AREA_FULL_ACCESS, AREA_NO_LOCK, hArea );
    //    } else {
    //  m_hArea = -1;
    //    }
//    if (nFlags & ACCEPT_VIEWS) {
//        m_pcWindow = new Window(this);
//        m_pcWindow->Unlock();
//    } else {
//        m_pcWindow = NULL;
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Bitmap::~Bitmap()
{
//    if (m_hArea != -1) {
//        delete_area(m_hArea);
//    }
//    delete m_pcWindow;
//    Application::GetInstance()->DeleteBitmap(m_Handle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

color_space Bitmap::GetColorSpace() const
{
    return m_ColorSpace;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Bitmap::AddChild(View* pcView)
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->AddChild(pcView);
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Bitmap::RemoveChild(View* pcView)
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->RemoveChild(pcView);
//        return true;
//    } else {
        return false;
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View* Bitmap::FindView(const char* pzName) const
{
//    if (NULL != m_pcWindow) {
//        return m_pcWindow->FindView(pzName);
//    } else {
        return nullptr;
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Bitmap::Flush(void)
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->Flush();
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Bitmap::Sync(void)
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->Sync();
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect Bitmap::GetBounds( void ) const
{
    return m_Bounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Bitmap::GetBytesPerRow() const
{
    int nBitsPerPix = BitsPerPixel(m_ColorSpace);
    if (nBitsPerPix == 15) {
        nBitsPerPix = 16;
    }
    return int(m_Bounds.right - m_Bounds.left + 1.0f) * nBitsPerPix / 8;
}
