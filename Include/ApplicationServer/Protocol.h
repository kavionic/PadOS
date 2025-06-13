// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 17.03.2018 22:45:33

#pragma once

#include <inttypes.h>

#include <Signals/RemoteSignal.h>
#include <Utils/MessagePort.h>
#include <GUI/GUIEvent.h>
#include <GUI/Color.h>
#include <GUI/GUIDefines.h>


namespace os
{
class Rect;
enum class Font_e : uint8_t;

port_id get_appserver_port();
port_id get_window_manager_port();

typedef int32_t app_handle;
enum class ViewDockType : int32_t;
enum class FocusKeyboardMode : uint8_t;

template<typename SIGNAL, typename... ARGS>
bool post_to_window_manager(handler_id targetHandler, ARGS&&... args)
{
    return post_to_remotesignal<SIGNAL>(get_window_manager_port(), targetHandler, TimeValMicros::infinit, args...);
}

namespace AppserverProtocol
{
    enum Type
    {
        NONE,
        // Appserver messages:
        MESSAGE_BUNDLE,
        REGISTER_APPLICATION,
        // Application messages:
        SYNC,
        CREATE_VIEW,
        DELETE_VIEW,
        FOCUS_VIEW,
        SET_KEYBOARD_FOCUS,
        CREATE_BITMAP,
        DELETE_BITMAP,

        // View messages:
        VIEW_SET_FRAME,
        VIEW_INVALIDATE,
        VIEW_ADD_CHILD,
        VIEW_SET_DRAW_REGION,
        VIEW_SET_SHAPE_REGION,
        VIEW_TOGGLE_DEPTH,
                
        VIEW_BEGIN_UPDATE,
        VIEW_END_UPDATE,
        VIEW_SHOW,
        VIEW_SET_FOCUS_KEYBOARD_MODE,
        VIEW_SET_DRAWING_MODE,
        VIEW_SET_FG_COLOR,
        VIEW_SET_BG_COLOR,
        VIEW_SET_ERASE_COLOR,
        VIEW_SET_FONT,
        VIEW_MOVE_PEN_TO,
        VIEW_SET_PEN_WIDTH,
        VIEW_DRAW_LINE1,
        VIEW_DRAW_LINE2,
        VIEW_FILL_RECT,
        VIEW_FILL_CIRCLE,
        VIEW_DRAW_STRING,
        VIEW_SCROLL_BY,
        VIEW_COPY_RECT,
        VIEW_DRAW_BITMAP,
        VIEW_DRAW_SCALED_BITMAP,
        VIEW_DEBUG_DRAW,
        
        // Appserver -> view reply messages:
        REGISTER_APPLICATION_REPLY,
        CREATE_VIEW_REPLY,
        CREATE_BITMAP_REPLY,
        PAINT_VIEW,
        VIEW_FRAME_CHANGED,
        VIEW_FOCUS_CHANGED,
        
        // Appserver <-> Window manager messages:
        WINDOW_MANAGER_REGISTER_VIEW,
        WINDOW_MANAGER_UNREGISTER_VIEW,
        WINDOW_MANAGER_ENABLE_VKEYBOARD,
        WINDOW_MANAGER_DISABLE_VKEYBOARD,
        
        // Appserver -> application messages:
        SYNC_REPLY,
        HANDLE_MOUSE_DOWN,
        HANDLE_MOUSE_UP,
        HANDLE_MOUSE_MOVE
    };
}

static const int APPSERVER_MSG_BUFFER_SIZE = 1024 * 8;

struct AppserverMessage
{
    int32_t    m_Length;
    handler_id m_TargetHandler;
    int32_t    m_Code;
};

struct MsgRegisterApplicationReply
{
    handler_id m_ServerHandle;
};

struct MsgCreateViewReply
{
    handler_id m_ViewHandle;
};

struct MsgCreateBitmapReply
{
    handler_id  m_BitmapHandle;
    uint8_t*    m_Framebuffer;
    size_t      m_BytesPerRow;
};

class String;

using ASRegisterApplication = RemoteSignal<AppserverProtocol::REGISTER_APPLICATION, void(port_id replyPort, port_id clientPort, const String& name)>;
using ASSync                = RemoteSignal<AppserverProtocol::SYNC,                 void(port_id replyPort)>;

using ASCreateView = RemoteSignal<AppserverProtocol::CREATE_VIEW,
    void(
        port_id           clientPort,
        port_id           replyPort,
        handler_id        replyTarget,
        handler_id        parent,
        ViewDockType      dockType,
        size_t            index,
        const String&     name,
        const Rect&       frame,
        const Point&      scrollOffset,
        uint32_t          flags,
        int32_t           hideCount,
        FocusKeyboardMode focusKeyboardMode,
        DrawingMode       drawingMode,
        float             penWidth,
        Font_e            fontID,
        Color             eraseColor,
        Color             bgColor,
        Color             fgColor
    )
>;
                                                               
using ASDeleteView = RemoteSignal<AppserverProtocol::DELETE_VIEW, void(handler_id viewHandle)>;

using ASFocusView = RemoteSignal<AppserverProtocol::FOCUS_VIEW , void(handler_id viewHandle, MouseButton_e button, bool)>; // 'true' for set, 'false' for clear focus.

using ASSetKeyboardFocus = RemoteSignal<AppserverProtocol::SET_KEYBOARD_FOCUS,
    void(
        handler_id viewHandle,
        bool focus // 'true' for set, 'false' for clear focus.
        )
>; 

using ASCreateBitmap = RemoteSignal<AppserverProtocol::CREATE_BITMAP,
    void(
        port_id     replyPort,
        int32_t     width,
        int32_t     height,
        EColorSpace colorSpace,
        void*       raster,
        size_t      bytesPerRow,
        uint32_t    flags
        )
>;

using ASDeleteBitmap        = RemoteSignal<AppserverProtocol::DELETE_BITMAP,        void(handler_id handle)>;
using ASViewSetFrame        = RemoteSignal<AppserverProtocol::VIEW_SET_FRAME,       void(handler_id viewHandle, const Rect& frame, handler_id requestingClient)>;
using ASViewInvalidate      = RemoteSignal<AppserverProtocol::VIEW_INVALIDATE,      void(handler_id viewHandle, const IRect& frame)>;
using ASViewAddChild        = RemoteSignal<AppserverProtocol::VIEW_ADD_CHILD,       void(size_t index, handler_id viewHandle, handler_id childHandle, handler_id managerHandle)>;
using ASViewToggleDepth     = RemoteSignal<AppserverProtocol::VIEW_TOGGLE_DEPTH,    void(handler_id viewHandle)>;
using ASViewBeginUpdate     = RemoteSignal<AppserverProtocol::VIEW_BEGIN_UPDATE,    void(handler_id viewHandle)>;
using ASViewEndUpdate       = RemoteSignal<AppserverProtocol::VIEW_END_UPDATE,      void(handler_id viewHandle)>;
using ASViewShow            = RemoteSignal<AppserverProtocol::VIEW_SHOW,            void(handler_id viewHandle, bool show)>;
using ASViewSetFocusKeyboardMode= RemoteSignal<AppserverProtocol::VIEW_SET_FOCUS_KEYBOARD_MODE, void(handler_id viewHandle, FocusKeyboardMode mode)>;
using ASViewSetDrawingMode  = RemoteSignal<AppserverProtocol::VIEW_SET_DRAWING_MODE,    void(handler_id viewHandle, DrawingMode Mode)>;
using ASViewSetFgColor      = RemoteSignal<AppserverProtocol::VIEW_SET_FG_COLOR,        void(handler_id viewHandle, Color color)>;
using ASViewSetBgColor      = RemoteSignal<AppserverProtocol::VIEW_SET_BG_COLOR,        void(handler_id viewHandle, Color color)>;
using ASViewSetEraseColor   = RemoteSignal<AppserverProtocol::VIEW_SET_ERASE_COLOR,     void(handler_id viewHandle, Color color)>;
using ASViewSetFont         = RemoteSignal<AppserverProtocol::VIEW_SET_FONT,            void(handler_id viewHandle, int fontHandle)>;
using ASViewMovePenTo       = RemoteSignal<AppserverProtocol::VIEW_MOVE_PEN_TO,         void(handler_id viewHandle, const Point pos)>;
using ASViewSetPenWidth     = RemoteSignal<AppserverProtocol::VIEW_SET_PEN_WIDTH,       void(handler_id viewHandle, float width)>;

using ASViewDrawLine1       = RemoteSignal<AppserverProtocol::VIEW_DRAW_LINE1,          void(handler_id viewHandle, const Point& position)>;
using ASViewDrawLine2       = RemoteSignal<AppserverProtocol::VIEW_DRAW_LINE2,          void(handler_id viewHandle, const Point& pos1, const Point& pos2)>;
using ASViewFillRect        = RemoteSignal<AppserverProtocol::VIEW_FILL_RECT,           void(handler_id viewHandle, const Rect& rect, Color color)>;
using ASViewFillCircle      = RemoteSignal<AppserverProtocol::VIEW_FILL_CIRCLE,         void(handler_id viewHandle, const Point& position, float radius)>;
using ASViewDrawString      = RemoteSignal<AppserverProtocol::VIEW_DRAW_STRING,         void(handler_id viewHandle, const String& string)>;
using ASViewScrollBy        = RemoteSignal<AppserverProtocol::VIEW_SCROLL_BY,           void(handler_id viewHandle, const Point& delta)>;
using ASViewCopyRect        = RemoteSignal<AppserverProtocol::VIEW_COPY_RECT,           void(handler_id viewHandle, const Rect& srcRect, const Point& dstPos)>;
using ASViewDrawBitmap      = RemoteSignal<AppserverProtocol::VIEW_DRAW_BITMAP,         void(handler_id viewHandle, handle_id bitmapHandle, const Rect& srcRect, const Point& dstPos)>;
using ASViewDrawScaledBitmap= RemoteSignal<AppserverProtocol::VIEW_DRAW_SCALED_BITMAP,  void (handler_id viewHandle, handle_id bitmapHandle, const Rect& srcRect, const Rect& dstRect)>;
using ASViewDebugDraw       = RemoteSignal<AppserverProtocol::VIEW_DEBUG_DRAW,          void(handler_id viewHandle, Color renderColor, uint32_t drawFlags)>;
using ASPaintView           = RemoteSignal<AppserverProtocol::PAINT_VIEW,               void(const Rect& frame)>;
using ASViewFrameChanged    = RemoteSignal<AppserverProtocol::VIEW_FRAME_CHANGED,       void(const Rect& frame)>;
using ASViewFocusChanged    = RemoteSignal<AppserverProtocol::VIEW_FOCUS_CHANGED,       void(bool hasFocus)>;


using ASWindowManagerRegisterView       = RemoteSignal<AppserverProtocol::WINDOW_MANAGER_REGISTER_VIEW,     void(handler_id viewHandle, ViewDockType dockType, const String& name, const Rect& frame)>;
using ASWindowManagerUnregisterView     = RemoteSignal<AppserverProtocol::WINDOW_MANAGER_UNREGISTER_VIEW,   void(handler_id viewHandle)>;
using ASWindowManagerEnableVKeyboard    = RemoteSignal<AppserverProtocol::WINDOW_MANAGER_ENABLE_VKEYBOARD,  void(const Rect& focusViewEditArea, bool numerical)>;
using ASWindowManagerDisableVKeyboard   = RemoteSignal<AppserverProtocol::WINDOW_MANAGER_DISABLE_VKEYBOARD>;

using ASSyncReply = RemoteSignal<AppserverProtocol::SYNC_REPLY>;
                                        
using ASHandleMouseDown = RemoteSignal<AppserverProtocol::HANDLE_MOUSE_DOWN,    void(MouseButton_e button, const Point& position, const MotionEvent& mouseEvent)>;
using ASHandleMouseUp   = RemoteSignal<AppserverProtocol::HANDLE_MOUSE_UP,      void(MouseButton_e button, const Point& position, const MotionEvent& mouseEvent)>;
using ASHandleMouseMove = RemoteSignal<AppserverProtocol::HANDLE_MOUSE_MOVE,    void(MouseButton_e button, const Point& position, const MotionEvent& mouseEvent)>;
                                        
} // namespace os
