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
#include <Ptr/PtrTarget.h>

namespace os
{

class Window;
class View;
class Application;

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
///     out by the application (requires the \b SHARE_FRAMEBUFFER flag
///     to be set as well) or it can be blited into other views.
///
/// \sa View, Window
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class Bitmap : public PtrTarget
{
public:
    enum { ACCEPT_VIEWS = 0x0001, SHARE_FRAMEBUFFER = 0x0002, CUSTOM_FRAMEBUFFER = 0x0004, READ_ONLY = 0x0008 };
    Bitmap(int width, int height, EColorSpace colorSpace, uint32_t flags = SHARE_FRAMEBUFFER, Application* application = nullptr);
    Bitmap(int width, int height, EColorSpace colorSpace, void* raster, size_t bytesPerRow, uint32_t flags = 0, Application* application = nullptr);
    Bitmap(int width, int height, EColorSpace colorSpace, const void* raster, size_t bytesPerRow, uint32_t flags = 0, Application* application = nullptr);

    virtual ~Bitmap();

    bool            IsValid() const;

    EColorSpace     GetColorSpace() const;
    IRect           GetBounds() const;
    int             GetBytesPerRow() const;
    virtual void    AddChild(View* view);
    virtual bool    RemoveChild(View* view);
    View*           FindView(const char* name) const;

    void            Sync();
    void            Flush();

    uint8_t*        LockRaster() { return m_Raster; }
    void            UnlockRaster() {}

private:
    void Initialize(int width, int height, EColorSpace colorSpace, uint8_t* raster, size_t bytesPerRow, uint32_t flags, Application* application);

    friend class View;
    friend class Window;
    friend class Sprite;

    Application*    m_Application   = nullptr;
    Window*         m_Window        = nullptr;
    handle_id       m_Handle        = INVALID_HANDLE;
    EColorSpace     m_ColorSpace    = EColorSpace::NO_COLOR_SPACE;
    size_t          m_BytesPerRow   = 0;
    IRect           m_Bounds;
    uint8_t*        m_Raster        = nullptr;
};



inline int BitsPerPixel(EColorSpace colorSpace)
{
    switch(colorSpace)
    {
    case EColorSpace::RGB32:
    case EColorSpace::RGBA32:
        return 32;
    case EColorSpace::RGB24:
        return 24;
    case EColorSpace::RGB16:
    case EColorSpace::RGB15:
    case EColorSpace::RGBA15:
        return 16;
    case EColorSpace::CMAP8:
    case EColorSpace::GRAY8:
        return 8;
    case EColorSpace::MONO1:
        return 1;
    default:
        p_system_log(LogCategoryGUITK, PLogSeverity::ERROR, "BitsPerPixel() invalid color space {}", int(colorSpace));
        return 8;
    }
}

} // namespace os
