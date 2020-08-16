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

#pragma once

#include <GUI/GUIDefines.h>
#include <Math/Rect.h>

namespace os
{

class Window;
class View;

///////////////////////////////////////////////////////////////////////////////
/// Container for bitmap-image data.
/// \ingroup gui
/// \par Description:
///     The Bitmap class make it possible to render bitmap graphics
///     into view's and to make view's render into an offscreen buffer
///     to implement things like double-buffering.
///
///     The bitmap class have two different ways to communicate with
///     the application server. If the \b SHARE_FRAMEBUFFER flag is
///     set the bitmaps raster memory is created in a memory-area
///     shared between the application and the appserver. This makes
///     it possible for the appserver to blit graphics written
///     directly to the bitmaps raster buffer by the application into
///     views on the screen (or inside other bitmaps). If the
///     \b ACCEPT_VIEWS flag is set the bitmap will accept views to be
///     added much like a os::Window object. All rendering performed
///     by the views will then go into the bitmap's offscreen buffer
///     rather than the screen. The rendered image can then be read
///     out by the application (requiers the \b SHARE_FRAMEBUFFER flag
///     to be set aswell) or it can be blited into other views.
///
/// \sa View, Window
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class Bitmap
{
public:
    enum { ACCEPT_VIEWS = 0x0001, SHARE_FRAMEBUFFER = 0x0002 };
    Bitmap( int nWidth, int nHeight, color_space colorSpace, uint32_t nFlags = SHARE_FRAMEBUFFER );
    virtual ~Bitmap();

    bool        IsValid( void ) const;

    color_space GetColorSpace() const;
    Rect        GetBounds( void ) const;
    int     GetBytesPerRow() const;
    virtual void    AddChild( View* pcView );
    virtual bool    RemoveChild( View* pcView );
    View*       FindView( const char* pzName ) const;

    void        Sync( void );
    void        Flush( void );

    uint8_t*    LockRaster( void ) { return( m_Raster ); }
    void        UnlockRaster() {}

private:
    friend class View;
    friend class Window;
    friend class Sprite;
  
    Window*     m_Window;
    int         m_Handle;
    color_space m_ColorSpace;
    Rect        m_Bounds;
    uint8_t*    m_Raster;
};



inline int BitsPerPixel( color_space colorSpace )
{
    switch(colorSpace)
    {
    case CS_RGB32:
    case CS_RGBA32:
        return 32;
    case CS_RGB24:
        return 24;
    case CS_RGB16:
    case CS_RGB15:
    case CS_RGBA15:
        return 16;
    case CS_CMAP8:
    case CS_GRAY8:
        return 8;
    case CS_GRAY1:
        return 1;
    default:
        printf( "BitsPerPixel() invalid color space %d\n", colorSpace);
        return 8;
    }
}

} // namespace os
