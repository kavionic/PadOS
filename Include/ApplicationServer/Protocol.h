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

class PString;

class PRect;

enum class PViewDockType : int32_t;
enum class PFocusKeyboardMode : uint8_t;

enum class PFontID : uint8_t;

port_id p_get_appserver_port();
port_id p_get_window_manager_port();

typedef int32_t app_handle;

template<typename SIGNAL, typename... ARGS>
bool p_post_to_window_manager(handler_id targetHandler, ARGS&&... args)
{
    return p_post_to_remotesignal<SIGNAL>(p_get_window_manager_port(), targetHandler, TimeValNanos::infinit, args...);
}

namespace PAppserverProtocol
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

static const int PAPPSERVER_MSG_BUFFER_SIZE = 1024 * 8;

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

using ASRegisterApplication = PRemoteSignal<PAppserverProtocol::REGISTER_APPLICATION, void(port_id replyPort, port_id clientPort, const PString& name)>;
using ASSync                = PRemoteSignal<PAppserverProtocol::SYNC,                 void(port_id replyPort)>;

using ASCreateView = PRemoteSignal<PAppserverProtocol::CREATE_VIEW,
    void(
        port_id           clientPort,
        port_id           replyPort,
        handler_id        replyTarget,
        handler_id        parent,
        PViewDockType      dockType,
        size_t            index,
        const PString&    name,
        const PRect&       frame,
        const PPoint&      scrollOffset,
        uint32_t          flags,
        int32_t           hideCount,
        PFocusKeyboardMode focusKeyboardMode,
        PDrawingMode       drawingMode,
        float             penWidth,
        PFontID            fontID,
        PColor             eraseColor,
        PColor             bgColor,
        PColor             fgColor
    )
>;
                                                               
using ASDeleteView = PRemoteSignal<PAppserverProtocol::DELETE_VIEW, void(handler_id viewHandle)>;

using ASFocusView = PRemoteSignal<PAppserverProtocol::FOCUS_VIEW , void(handler_id viewHandle, PMouseButton button, bool)>; // 'true' for set, 'false' for clear focus.

using ASSetKeyboardFocus = PRemoteSignal<PAppserverProtocol::SET_KEYBOARD_FOCUS,
    void(
        handler_id viewHandle,
        bool focus // 'true' for set, 'false' for clear focus.
        )
>; 

using ASCreateBitmap = PRemoteSignal<PAppserverProtocol::CREATE_BITMAP,
    void(
        port_id     replyPort,
        int32_t     width,
        int32_t     height,
        PEColorSpace colorSpace,
        void*       raster,
        size_t      bytesPerRow,
        uint32_t    flags
        )
>;

using ASDeleteBitmap        = PRemoteSignal<PAppserverProtocol::DELETE_BITMAP,        void(handler_id handle)>;
using ASViewSetFrame        = PRemoteSignal<PAppserverProtocol::VIEW_SET_FRAME,       void(handler_id viewHandle, const PRect& frame, handler_id requestingClient)>;
using ASViewInvalidate      = PRemoteSignal<PAppserverProtocol::VIEW_INVALIDATE,      void(handler_id viewHandle, const PIRect& frame)>;
using ASViewAddChild        = PRemoteSignal<PAppserverProtocol::VIEW_ADD_CHILD,       void(size_t index, handler_id viewHandle, handler_id childHandle, handler_id managerHandle)>;
using ASViewToggleDepth     = PRemoteSignal<PAppserverProtocol::VIEW_TOGGLE_DEPTH,    void(handler_id viewHandle)>;
using ASViewBeginUpdate     = PRemoteSignal<PAppserverProtocol::VIEW_BEGIN_UPDATE,    void(handler_id viewHandle)>;
using ASViewEndUpdate       = PRemoteSignal<PAppserverProtocol::VIEW_END_UPDATE,      void(handler_id viewHandle)>;
using ASViewShow            = PRemoteSignal<PAppserverProtocol::VIEW_SHOW,            void(handler_id viewHandle, bool show)>;
using ASViewSetFocusKeyboardMode= PRemoteSignal<PAppserverProtocol::VIEW_SET_FOCUS_KEYBOARD_MODE, void(handler_id viewHandle, PFocusKeyboardMode mode)>;
using ASViewSetDrawingMode  = PRemoteSignal<PAppserverProtocol::VIEW_SET_DRAWING_MODE,    void(handler_id viewHandle, PDrawingMode Mode)>;
using ASViewSetFgColor      = PRemoteSignal<PAppserverProtocol::VIEW_SET_FG_COLOR,        void(handler_id viewHandle, PColor color)>;
using ASViewSetBgColor      = PRemoteSignal<PAppserverProtocol::VIEW_SET_BG_COLOR,        void(handler_id viewHandle, PColor color)>;
using ASViewSetEraseColor   = PRemoteSignal<PAppserverProtocol::VIEW_SET_ERASE_COLOR,     void(handler_id viewHandle, PColor color)>;
using ASViewSetFont         = PRemoteSignal<PAppserverProtocol::VIEW_SET_FONT,            void(handler_id viewHandle, int fontHandle)>;
using ASViewMovePenTo       = PRemoteSignal<PAppserverProtocol::VIEW_MOVE_PEN_TO,         void(handler_id viewHandle, const PPoint pos)>;
using ASViewSetPenWidth     = PRemoteSignal<PAppserverProtocol::VIEW_SET_PEN_WIDTH,       void(handler_id viewHandle, float width)>;

using ASViewDrawLine1       = PRemoteSignal<PAppserverProtocol::VIEW_DRAW_LINE1,          void(handler_id viewHandle, const PPoint& position)>;
using ASViewDrawLine2       = PRemoteSignal<PAppserverProtocol::VIEW_DRAW_LINE2,          void(handler_id viewHandle, const PPoint& pos1, const PPoint& pos2)>;
using ASViewFillRect        = PRemoteSignal<PAppserverProtocol::VIEW_FILL_RECT,           void(handler_id viewHandle, const PRect& rect, PColor color)>;
using ASViewFillCircle      = PRemoteSignal<PAppserverProtocol::VIEW_FILL_CIRCLE,         void(handler_id viewHandle, const PPoint& position, float radius)>;
using ASViewDrawString      = PRemoteSignal<PAppserverProtocol::VIEW_DRAW_STRING,         void(handler_id viewHandle, const PString& string)>;
using ASViewScrollBy        = PRemoteSignal<PAppserverProtocol::VIEW_SCROLL_BY,           void(handler_id viewHandle, const PPoint& delta)>;
using ASViewCopyRect        = PRemoteSignal<PAppserverProtocol::VIEW_COPY_RECT,           void(handler_id viewHandle, const PRect& srcRect, const PPoint& dstPos)>;
using ASViewDrawBitmap      = PRemoteSignal<PAppserverProtocol::VIEW_DRAW_BITMAP,         void(handler_id viewHandle, handle_id bitmapHandle, const PRect& srcRect, const PPoint& dstPos)>;
using ASViewDrawScaledBitmap= PRemoteSignal<PAppserverProtocol::VIEW_DRAW_SCALED_BITMAP,  void (handler_id viewHandle, handle_id bitmapHandle, const PRect& srcRect, const PRect& dstRect)>;
using ASViewDebugDraw       = PRemoteSignal<PAppserverProtocol::VIEW_DEBUG_DRAW,          void(handler_id viewHandle, PColor renderColor, uint32_t drawFlags)>;
using ASPaintView           = PRemoteSignal<PAppserverProtocol::PAINT_VIEW,               void(const PRect& frame)>;
using ASViewFrameChanged    = PRemoteSignal<PAppserverProtocol::VIEW_FRAME_CHANGED,       void(const PRect& frame)>;
using ASViewFocusChanged    = PRemoteSignal<PAppserverProtocol::VIEW_FOCUS_CHANGED,       void(bool hasFocus)>;


using ASWindowManagerRegisterView       = PRemoteSignal<PAppserverProtocol::WINDOW_MANAGER_REGISTER_VIEW,     void(handler_id viewHandle, PViewDockType dockType, const PString& name, const PRect& frame)>;
using ASWindowManagerUnregisterView     = PRemoteSignal<PAppserverProtocol::WINDOW_MANAGER_UNREGISTER_VIEW,   void(handler_id viewHandle)>;
using ASWindowManagerEnableVKeyboard    = PRemoteSignal<PAppserverProtocol::WINDOW_MANAGER_ENABLE_VKEYBOARD,  void(const PRect& focusViewEditArea, bool numerical)>;
using ASWindowManagerDisableVKeyboard   = PRemoteSignal<PAppserverProtocol::WINDOW_MANAGER_DISABLE_VKEYBOARD>;

using ASSyncReply = PRemoteSignal<PAppserverProtocol::SYNC_REPLY>;
                                        
using ASHandleMouseDown = PRemoteSignal<PAppserverProtocol::HANDLE_MOUSE_DOWN,    void(PMouseButton button, const PPoint& position, const PMotionEvent& mouseEvent)>;
using ASHandleMouseUp   = PRemoteSignal<PAppserverProtocol::HANDLE_MOUSE_UP,      void(PMouseButton button, const PPoint& position, const PMotionEvent& mouseEvent)>;
using ASHandleMouseMove = PRemoteSignal<PAppserverProtocol::HANDLE_MOUSE_MOVE,    void(PMouseButton button, const PPoint& position, const PMotionEvent& mouseEvent)>;
