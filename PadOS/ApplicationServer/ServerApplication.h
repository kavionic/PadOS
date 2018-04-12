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
// Created: 28.03.2018 20:48:36

#pragma once


#include "System/Threads/EventHandler.h"
#include "System/Utils/MessagePort.h"
#include "System/Math/Rect.h"

#include "ApplicationServer.h"
#include "ServerView.h"

namespace os
{


class ServerApplication : public EventHandler, public SignalTarget
{
public:
    ServerApplication(ApplicationServer* server, const String& name, port_id clientPort);
    ~ServerApplication();
    
    virtual bool HandleMessage(int32_t code, const void* data, size_t length) override;
    
private:
    void ProcessMessage(int32_t code, const void* data, size_t length);
    void UpdateRegions();
    void UpdateLowestInvalidView(Ptr<ServerView> view);

    void SlotCreateView(port_id clientPort, port_id replyPort, handler_id replyTarget, handler_id parentHandle, ViewDockType dockType, const String& name, const Rect& frame, const Point& scrollOffset, uint32_t flags, int32_t hideCount, Color eraseColor, Color bgColor, Color fgColor);
    void SlotDeleteView(handler_id clientHandle);
    void SlotViewSetFrame(handler_id clientHandle, const Rect& frame, handler_id requestingClient);
    void SlotViewInvalidate(handler_id clientHandle, const IRect& frame);
    void SlotViewAddChild(handler_id viewHandle, handler_id childHandle, handler_id managerHandle);
    void SlotSync(port_id replyPort)                                                        { ASSyncReply::Sender::Emit(MessagePort(replyPort), -1, 0); }
    void SlotViewToggleDepth(handler_id viewHandle)                                         { ForwardToView(viewHandle, &ServerView::ToggleDepth); }
    void SlotViewBeginUpdate(handler_id viewHandle)                                         { ForwardToView(viewHandle, &ServerView::BeginUpdate); }
    void SlotViewEndUpdate(handler_id viewHandle)                                           { ForwardToView(viewHandle, &ServerView::EndUpdate); }
    void SlotViewSetFgColor(handler_id viewHandle, Color color)                             { ForwardToView(viewHandle, &ServerView::SetFgColor, color); }
    void SlotViewSetBgColor(handler_id viewHandle, Color color)                             { ForwardToView(viewHandle, &ServerView::SetBgColor, color); }
    void SlotViewSetEraseColor(handler_id viewHandle, Color color)                          { ForwardToView(viewHandle, &ServerView::SetEraseColor, color); }
    void SlotViewSetFont(handler_id viewHandle, int fontHandle)                             { ForwardToView(viewHandle, &ServerView::SetFont, fontHandle); }
    void SlotViewMovePenTo(handler_id viewHandle, const Point& pos)                         { ForwardToView(viewHandle, &ServerView::MovePenTo, pos); }
    void SlotViewDrawLine1(handler_id viewHandle, const Point& toPoint)                     { ForwardToView(viewHandle, &ServerView::DrawLineTo, toPoint); }
    void SlotViewDrawLine2(handler_id viewHandle, const Point& fromPnt, const Point& toPnt) { ForwardToView(viewHandle, &ServerView::DrawLine, fromPnt, toPnt); }
    void SlotViewFillRect(handler_id viewHandle, const Rect& rect, Color color)             { ForwardToView(viewHandle, &ServerView::FillRect, rect, color); }
    void SlotViewFillCircle(handler_id viewHandle, const Point& position, float radius)     { ForwardToView(viewHandle, &ServerView::FillCircle, position, radius); }
    void SlotViewDrawString(handler_id viewHandle, const String& string, float maxWidth, uint8_t flags)     { ForwardToView(viewHandle, &ServerView::DrawString, string, maxWidth, flags); }
    void SlotViewScrollBy(handler_id viewHandle, const Point& delta)                        { ForwardToView(viewHandle, &ServerView::ScrollBy, delta); }
    void SlotViewCopyRect(handler_id viewHandle, const Rect& srcRect, const Point& dstPos)  { ForwardToView(viewHandle, &ServerView::CopyRect, srcRect, dstPos); }

    template<typename CB, typename... ARGS>
    void ForwardToView(handler_id viewHandle, CB callback, ARGS&&... args)
    {
        Ptr<ServerView> view = m_Server->FindView(viewHandle);
        if (view != nullptr) {
            (*view.*callback)(args...);
        } else {
            printf("ERROR: ServerView::ForwardToView() failed to find view %d\n", viewHandle);
        }
    }

    ApplicationServer* m_Server;

    port_id m_ClientPort;
    
    
    Ptr<ServerView> m_LowestInvalidView;
    int             m_LowestInvalidLevel = std::numeric_limits<int>::max();
    
    ASSync::Receiver              RSSync;
    ASCreateView::Receiver        RSCreateView;
    ASDeleteView::Receiver        RSDeleteView;
    ASViewSetFrame::Receiver      RSViewSetFrame;
    ASViewInvalidate::Receiver    RSViewInvalidate;
    ASViewAddChild::Receiver      RSViewAddChild;
    ASViewToggleDepth::Receiver   RSViewToggleDepth;
    ASViewBeginUpdate::Receiver   RSViewBeginUpdate;
    ASViewEndUpdate::Receiver     RSViewEndUpdate;
    ASViewSetFgColor::Receiver    RSViewSetFgColor;
    ASViewSetBgColor::Receiver    RSViewSetBgColor;
    ASViewSetEraseColor::Receiver RSViewSetEraseColor;
    ASViewSetFont::Receiver       RSViewSetFont;
    ASViewMovePenTo::Receiver     RSViewMovePenTo;
    ASViewDrawLine1::Receiver     RSViewDrawLine1;
    ASViewDrawLine2::Receiver     RSViewDrawLine2;
    ASViewFillRect::Receiver      RSViewFillRect;
    ASViewFillCircle::Receiver    RSViewFillCircle;
    ASViewDrawString::Receiver    RSViewDrawString;
    ASViewScrollBy::Receiver      RSViewScrollBy;
    ASViewCopyRect::Receiver      RSViewCopyRect;
    
    ServerApplication(const ServerApplication&) = delete;
    ServerApplication& operator=(const ServerApplication&) = delete;
};

}
