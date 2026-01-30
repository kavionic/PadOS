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

class PApplication;
class PWindow;
class PView;


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

class PBitmap : public PtrTarget
{
public:
    enum { ACCEPT_VIEWS = 0x0001, SHARE_FRAMEBUFFER = 0x0002, CUSTOM_FRAMEBUFFER = 0x0004, READ_ONLY = 0x0008 };
    PBitmap(int width, int height, PEColorSpace colorSpace, uint32_t flags = SHARE_FRAMEBUFFER, PApplication* application = nullptr);
    PBitmap(int width, int height, PEColorSpace colorSpace, void* raster, size_t bytesPerRow, uint32_t flags = 0, PApplication* application = nullptr);
    PBitmap(int width, int height, PEColorSpace colorSpace, const void* raster, size_t bytesPerRow, uint32_t flags = 0, PApplication* application = nullptr);

    virtual ~PBitmap();

    bool            IsValid() const;

    PEColorSpace     GetColorSpace() const;
    PIRect           GetBounds() const;
    int             GetBytesPerRow() const;
    virtual void    AddChild(PView* view);
    virtual bool    RemoveChild(PView* view);
    PView*           FindView(const char* name) const;

    void            Sync();
    void            Flush();

    uint8_t*        LockRaster() { return m_Raster; }
    void            UnlockRaster() {}

private:
    void Initialize(int width, int height, PEColorSpace colorSpace, uint8_t* raster, size_t bytesPerRow, uint32_t flags, PApplication* application);

    friend class PView;
    friend class PWindow;
    friend class Sprite;

    PApplication*    m_Application   = nullptr;
    PWindow*         m_Window        = nullptr;
    handle_id       m_Handle        = INVALID_HANDLE;
    PEColorSpace     m_ColorSpace    = PEColorSpace::NO_COLOR_SPACE;
    size_t          m_BytesPerRow   = 0;
    PIRect           m_Bounds;
    uint8_t*        m_Raster        = nullptr;
};



inline int BitsPerPixel(PEColorSpace colorSpace)
{
    switch(colorSpace)
    {
    case PEColorSpace::RGB32:
    case PEColorSpace::RGBA32:
        return 32;
    case PEColorSpace::RGB24:
        return 24;
    case PEColorSpace::RGB16:
    case PEColorSpace::RGB15:
    case PEColorSpace::RGBA15:
        return 16;
    case PEColorSpace::CMAP8:
    case PEColorSpace::GRAY8:
        return 8;
    case PEColorSpace::MONO1:
        return 1;
    default:
        p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "BitsPerPixel() invalid color space {}", int(colorSpace));
        return 8;
    }
}
