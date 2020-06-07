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

#include "Signals/RemoteSignal.h"
#include "Utils/MessagePort.h"
#include "GUI/GUIEvent.h"
#include "GUI/Color.h"

class Rect;

namespace os
{

port_id get_appserver_port();
port_id get_window_manager_port();

typedef int32_t app_handle;
enum class ViewDockType : int32_t;

namespace AppserverProtocol
{
    enum Type
    {
        // Appserver messages:
        MESSAGE_BUNDLE,
        REGISTER_APPLICATION,
        // Application messages:
        SYNC,
        CREATE_VIEW,
        DELETE_VIEW,
        SHOW_VIEW,

        // View messages:
        VIEW_SET_FRAME,
        VIEW_INVALIDATE,
        VIEW_ADD_CHILD,
        VIEW_SET_DRAW_REGION,
        VIEW_SET_SHAPE_REGION,
        VIEW_TOGGLE_DEPTH,
                
        VIEW_BEGIN_UPDATE,
        VIEW_END_UPDATE,
        VIEW_SET_FG_COLOR,
        VIEW_SET_BG_COLOR,
        VIEW_SET_ERASE_COLOR,
        VIEW_SET_FONT,
        VIEW_MOVE_PEN_TO,
        VIEW_DRAW_LINE1,
        VIEW_DRAW_LINE2,
        VIEW_FILL_RECT,
        VIEW_FILL_CIRCLE,
        VIEW_DRAW_STRING,
        VIEW_SCROLL_BY,
        VIEW_COPY_RECT,
        VIEW_DEBUG_DRAW,
        
        // Appserver -> view reply messages:
        REGISTER_APPLICATION_REPLY,
        CREATE_VIEW_REPLY,
        PAINT_VIEW,
        VIEW_FRAME_CHANGED,
        
        // Appserver <-> Window manager messages:
        WINDOW_MANAGER_REGISTER_VIEW,
        WINDOW_MANAGER_UNREGISTER_VIEW,
        
        
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

class String;

typedef RemoteSignal<AppserverProtocol::REGISTER_APPLICATION
                                        , port_id       // replyPort
                                        , port_id       // clientPort
                                        , const String& // name
                                        > ASRegisterApplication;

typedef RemoteSignal<AppserverProtocol::SYNC
                                        ,port_id // replyPort
                                        > ASSync;

typedef RemoteSignal<AppserverProtocol::CREATE_VIEW
                                        , port_id       // clientPort
                                        , port_id       // replyPort
                                        , handler_id    // replyTarget
                                        , handler_id    // parent
                                        , ViewDockType  // dockType
                                        , const String& // name
                                        , const Rect&   // frame
                                        , const Point&  // scrollOffset
                                        , uint32_t      // flags
                                        , int32_t       // hideCount
                                        , Color         // eraseColor
                                        , Color         // bgColor
                                        , Color         // fgColor
                                        > ASCreateView;
                                                               
typedef RemoteSignal<AppserverProtocol::DELETE_VIEW
                                        , handler_id // viewHandle
                                        > ASDeleteView;

typedef RemoteSignal<AppserverProtocol::VIEW_SET_FRAME
                                        , handler_id  // viewHandle
                                        , const Rect& // frame
                                        , handler_id  // requestingClient
                                        > ASViewSetFrame;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_INVALIDATE
                                        , handler_id   // viewHandle
                                        , const IRect& // frame
                                        > ASViewInvalidate;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_ADD_CHILD
                                        , handler_id // viewHandle
                                        , handler_id //childHandle
                                        , handler_id // managerHandle
                                        > ASViewAddChild;

typedef RemoteSignal<AppserverProtocol::VIEW_TOGGLE_DEPTH
                                        , handler_id // viewHandle
                                        > ASViewToggleDepth;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_BEGIN_UPDATE
                                        , handler_id // viewHandle
                                        > ASViewBeginUpdate;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_END_UPDATE
                                        , handler_id // viewHandle
                                        > ASViewEndUpdate;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_SET_FG_COLOR
                                        , handler_id // viewHandle
                                        , Color      // color
                                        > ASViewSetFgColor;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_SET_BG_COLOR
                                        , handler_id // viewHandle
                                        , Color      // color
                                        > ASViewSetBgColor;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_SET_ERASE_COLOR
                                        , handler_id // viewHandle
                                        , Color      // color
                                        > ASViewSetEraseColor;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_SET_FONT
                                        , handler_id // viewHandle
                                        , int        // fontHandle
                                        > ASViewSetFont;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_MOVE_PEN_TO
                                        , handler_id  // viewHandle
                                        , const Point // pos
                                        > ASViewMovePenTo;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_DRAW_LINE1
                                        , handler_id   // viewHandle
                                        , const Point& // position
                                        > ASViewDrawLine1;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_DRAW_LINE2
                                        , handler_id   // viewHandle
                                        , const Point& // pos1
                                        , const Point& // pos2
                                        > ASViewDrawLine2;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_FILL_RECT
                                        , handler_id  // viewHandle
                                        , const Rect& // rect
                                        , Color       // color
                                        > ASViewFillRect;

typedef RemoteSignal<AppserverProtocol::VIEW_FILL_CIRCLE
                                        , handler_id   // viewHandle
                                        , const Point& // position
                                        , float        // radius
                                        > ASViewFillCircle;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_DRAW_STRING
                                        , handler_id    // viewHandle
                                        , const String& // string
                                        > ASViewDrawString;
                                        
typedef RemoteSignal<AppserverProtocol::VIEW_SCROLL_BY
                                        , handler_id // viewHandle
                                        , const Point& // delta
                                        > ASViewScrollBy;

typedef RemoteSignal<AppserverProtocol::VIEW_COPY_RECT
                                        , handler_id   // viewHandle
                                        , const Rect&  // srcRect
                                        , const Point& // dstPos
                                        > ASViewCopyRect;

typedef RemoteSignal<AppserverProtocol::VIEW_DEBUG_DRAW
                                        , handler_id   // viewHandle
                                        , Color        // renderColor
                                        , uint32_t     // drawFlags
                                        > ASViewDebugDraw;

typedef RemoteSignal<AppserverProtocol::PAINT_VIEW
                                        , const Rect& // frame
                                        > ASPaintView;

typedef RemoteSignal<AppserverProtocol::VIEW_FRAME_CHANGED
                                        , const Rect& // frame
                                        > ASViewFrameChanged;

typedef RemoteSignal<AppserverProtocol::WINDOW_MANAGER_REGISTER_VIEW
                                        , handler_id    // viewHandle
                                        , ViewDockType  // dockType
                                        , const String& // name
                                        , const Rect&   // frame
                                        > ASWindowManagerRegisterView;

typedef RemoteSignal<AppserverProtocol::WINDOW_MANAGER_UNREGISTER_VIEW
                                        , handler_id // viewHandle
                                        > ASWindowManagerUnregisterView;

typedef RemoteSignal<AppserverProtocol::SYNC_REPLY
                                        > ASSyncReply;
                                        
typedef RemoteSignal<AppserverProtocol::HANDLE_MOUSE_DOWN
                                        , MouseButton_e // button
                                        , const Point&  // pos
                                        > ASHandleMouseDown;
                                        
typedef RemoteSignal<AppserverProtocol::HANDLE_MOUSE_UP
                                        , MouseButton_e // button
                                        , const Point&  // pos
                                        > ASHandleMouseUp;
                                        
typedef RemoteSignal<AppserverProtocol::HANDLE_MOUSE_MOVE
                                        , MouseButton_e // button
                                        , const Point&  // pos
                                        > ASHandleMouseMove;
                                        


}
