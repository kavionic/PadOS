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

#include "System/Signals/RemoteSignal.h"
#include "System/Utils/MessagePort.h"
#include "System/GUI/GUIEvent.h"
#include "System/GUI/Color.h"

class Rect;

namespace os
{

extern MessagePort g_AppserverPort;

typedef int32_t app_handle;

namespace AppserverProtocol
{
    enum Type
    {
        // Appserver messages:
        MESSAGE_BUNDLE,
        REGISTER_APPLICATION,
        // Application messages:
        CREATE_VIEW,
        DELETE_VIEW,
        SHOW_VIEW,
        SET_VIEW_FRAME,
        INVALIDATE_VIEW,

        // View messages:
        VIEW_SET_DRAW_REGION,
        VIEW_SET_SHAPE_REGION,
        
        VIEW_SYNC,
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
        
        // Appserver -> view reply messages:
        REGISTER_APPLICATION_REPLY,
        CREATE_VIEW_REPLY,
        VIEW_SYNC_REPLY,
        PAINT_VIEW,
        
        // Appserver -> application messages:
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


class String;

typedef RemoteSignal<AppserverProtocol::REGISTER_APPLICATION, void, port_id /*clientPort*/, const String& /*name*/> ASRegisterApplication;

typedef RemoteSignal<AppserverProtocol::CREATE_VIEW,       void, port_id /*replyPort*/, handler_id /*replyTarget*/, handler_id /*parent*/, const String& /*name*/, const Rect& /*frame*/> ASCreateView;
typedef RemoteSignal<AppserverProtocol::DELETE_VIEW,       void, handler_id /*viewHandle*/>                         ASDeleteView;
typedef RemoteSignal<AppserverProtocol::SET_VIEW_FRAME,    void, handler_id /*viewHandle*/, const Rect& /*frame*/>  ASSetViewFrame;
typedef RemoteSignal<AppserverProtocol::INVALIDATE_VIEW,   void, handler_id /*viewHandle*/, const IRect& /*frame*/> ASInvalidateView;

typedef RemoteSignal<AppserverProtocol::VIEW_SYNC,            void, handler_id /*viewHandle*/>                                               ASViewSync;
typedef RemoteSignal<AppserverProtocol::VIEW_BEGIN_UPDATE,    void, handler_id /*viewHandle*/>                                               ASViewBeginUpdate;
typedef RemoteSignal<AppserverProtocol::VIEW_END_UPDATE,      void, handler_id /*viewHandle*/>                                               ASViewEndUpdate;
typedef RemoteSignal<AppserverProtocol::VIEW_SET_FG_COLOR,    void, handler_id /*viewHandle*/, Color /*color*/>                              ASViewSetFgColor;
typedef RemoteSignal<AppserverProtocol::VIEW_SET_BG_COLOR,    void, handler_id /*viewHandle*/, Color /*color*/>                              ASViewSetBgColor;
typedef RemoteSignal<AppserverProtocol::VIEW_SET_ERASE_COLOR, void, handler_id /*viewHandle*/, Color /*color*/>                              ASViewSetEraseColor;
typedef RemoteSignal<AppserverProtocol::VIEW_SET_FONT,        void, handler_id /*viewHandle*/, int /*fontHandle*/>                           ASViewSetFont;
typedef RemoteSignal<AppserverProtocol::VIEW_MOVE_PEN_TO,     void, handler_id /*viewHandle*/ ,const Point /*pos*/>                          ASViewMovePenTo;
typedef RemoteSignal<AppserverProtocol::VIEW_DRAW_LINE1,      void, handler_id /*viewHandle*/ ,const Point& /*position*/>                    ASViewDrawLine1;
typedef RemoteSignal<AppserverProtocol::VIEW_DRAW_LINE2,      void, handler_id /*viewHandle*/ ,const Point& /*pos1*/, const Point& /*pos2*/> ASViewDrawLine2;
typedef RemoteSignal<AppserverProtocol::VIEW_FILL_RECT,       void, handler_id /*viewHandle*/ ,const Rect& /*rect*/>                         ASViewFillRect;
typedef RemoteSignal<AppserverProtocol::VIEW_FILL_CIRCLE,     void, handler_id /*viewHandle*/ ,const Point& /*position*/, float /*radius*/>  ASViewFillCircle;
typedef RemoteSignal<AppserverProtocol::VIEW_DRAW_STRING,     void, handler_id /*viewHandle*/ ,const String& /*string*/, float /*maxWidth*/, uint8_t /*flags*/>  ASViewDrawString;
typedef RemoteSignal<AppserverProtocol::VIEW_SCROLL_BY,       void, handler_id /*viewHandle*/ ,const Point& /*delta*/>                       ASViewScrollBy;

typedef RemoteSignal<AppserverProtocol::REGISTER_APPLICATION_REPLY, void, handler_id /*serverHandle*/>              ASRegisterApplicationReply;
typedef RemoteSignal<AppserverProtocol::CREATE_VIEW_REPLY,          void, handler_id, handler_id>                   ASCreateViewReply;
typedef RemoteSignal<AppserverProtocol::VIEW_SYNC_REPLY,            void>                                           ASViewSyncReply;
typedef RemoteSignal<AppserverProtocol::PAINT_VIEW,                 void, const Rect& /*frame*/>                    ASPaintView;

typedef RemoteSignal<AppserverProtocol::HANDLE_MOUSE_DOWN, void, MouseButton_e /*button*/, const Point& /*pos*/>    ASHandleMouseDown;
typedef RemoteSignal<AppserverProtocol::HANDLE_MOUSE_UP,   void, MouseButton_e /*button*/, const Point& /*pos*/>    ASHandleMouseUp;
typedef RemoteSignal<AppserverProtocol::HANDLE_MOUSE_MOVE, void, MouseButton_e /*button*/, const Point& /*pos*/>    ASHandleMouseMove;

}
