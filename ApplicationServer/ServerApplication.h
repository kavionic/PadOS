// This file is part of PadOS.
//
// Copyright (C) 2018-2021 Kurt Skauen <http://kavionic.com/>
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


#include "Threads/EventHandler.h"
#include "Utils/MessagePort.h"
#include "Math/Rect.h"

#include "ApplicationServer/ApplicationServer.h"
#include "ServerView.h"


class ServerApplication : public PEventHandler, public SignalTarget
{
public:
    ServerApplication(ApplicationServer* server, const PString& name, port_id clientPort);
    ~ServerApplication();
    
    virtual bool HandleMessage(int32_t code, const void* data, size_t length) override;
    
    Ptr<PSrvBitmap> GetBitmap(handle_id bitmapHandle) const;

private:
    void ProcessMessage(int32_t code, const void* data, size_t length);
    void UpdateRegions();
    void UpdateLowestInvalidView(Ptr<PServerView> view);

    void SlotCreateView(port_id clientPort,
                        port_id replyPort,
                        handler_id replyTarget,
                        handler_id parentHandle,
                        PViewDockType dockType,
                        size_t index,
                        const PString& name,
                        const PRect& frame,
                        const PPoint& scrollOffset,
                        uint32_t flags,
                        int32_t hideCount,
                        PFocusKeyboardMode focusKeyboardMode,
                        PDrawingMode drawingMode,
                        float       penWidth,
                        PFontID      fontID,
                        PColor eraseColor,
                        PColor bgColor,
                        PColor fgColor);
    void SlotDeleteView(handler_id clientHandle);
    void SlotFocusView(handler_id clientHandle, PMouseButton button, bool focus);
    void SlotSetKeyboardFocus(handler_id clientHandle, bool focus);
    void SlotCreateBitmap(port_id replyPort, int width, int height, PEColorSpace colorSpace, void* raster, size_t bytesPerRow, uint32_t flags);
    void SlotDeleteBitmap(handle_id bitmapHandle);
    void SlotViewSetFrame(handler_id clientHandle, const PRect& frame, handler_id requestingClient);
    void SlotViewInvalidate(handler_id clientHandle, const PIRect& frame);
    void SlotViewAddChild(size_t index, handler_id viewHandle, handler_id childHandle, handler_id managerHandle);
    void SlotSync(port_id replyPort)                                                        { p_post_to_remotesignal<ASSyncReply>(PMessagePort(replyPort), INVALID_HANDLE, TimeValNanos::zero); }
    void SlotViewToggleDepth(handler_id viewHandle)                                         { ForwardToView(viewHandle, &PServerView::ToggleDepth); }
    void SlotViewBeginUpdate(handler_id viewHandle)                                         { ForwardToView(viewHandle, &PServerView::BeginUpdate); }
    void SlotViewEndUpdate(handler_id viewHandle)                                           { ForwardToView(viewHandle, &PServerView::EndUpdate); }
    void SlotViewShow(handler_id viewHandle, bool show);
    void SlotViewSetFgColor(handler_id viewHandle, PColor color)                             { ForwardToView(viewHandle, &PServerView::SetFgColor, color); }
    void SlotViewSetBgColor(handler_id viewHandle, PColor color)                             { ForwardToView(viewHandle, &PServerView::SetBgColor, color); }
    void SlotViewSetEraseColor(handler_id viewHandle, PColor color)                          { ForwardToView(viewHandle, &PServerView::SetEraseColor, color); }
    void SlotViewSetFocusKeyboardMode(handler_id viewHandle, PFocusKeyboardMode mode)        { ForwardToView(viewHandle, &PServerView::SetFocusKeyboardMode, mode); }
    void SlotViewSetDrawingMode(handler_id viewHandle, PDrawingMode mode)                    { ForwardToView(viewHandle, &PServerView::SetDrawingMode, mode); }
    void SlotViewSetFont(handler_id viewHandle, int fontHandle)                             { ForwardToView(viewHandle, &PServerView::SetFont, fontHandle); }
    void SlotViewSetPenWidth(handler_id viewHandle, float width)                            { ForwardToView(viewHandle, &PServerView::SetPenWidth, width); }
    void SlotViewMovePenTo(handler_id viewHandle, const PPoint& pos)                         { ForwardToView(viewHandle, &PServerView::MovePenTo, pos); }
    void SlotViewDrawLine1(handler_id viewHandle, const PPoint& toPoint)                     { ForwardToView(viewHandle, &PServerView::DrawLineTo, toPoint); }
    void SlotViewDrawLine2(handler_id viewHandle, const PPoint& fromPnt, const PPoint& toPnt) { ForwardToView(viewHandle, &PServerView::DrawLine, fromPnt, toPnt); }
    void SlotViewFillRect(handler_id viewHandle, const PRect& rect, PColor color)             { ForwardToView(viewHandle, &PServerView::FillRect, rect, color); }
    void SlotViewFillCircle(handler_id viewHandle, const PPoint& position, float radius)     { ForwardToView(viewHandle, &PServerView::FillCircle, position, radius); }
    void SlotViewDrawString(handler_id viewHandle, const PString& string)                   { ForwardToView(viewHandle, &PServerView::DrawString, string); }
    void SlotViewScrollBy(handler_id viewHandle, const PPoint& delta)                        { ForwardToView(viewHandle, &PServerView::ScrollBy, delta); }
    void SlotViewCopyRect(handler_id viewHandle, const PRect& srcRect, const PPoint& dstPos)  { ForwardToView(viewHandle, &PServerView::CopyRect, srcRect, dstPos); }
    void SlotViewDrawBitmap(handler_id viewHandle, handle_id bitmapHandle, const PRect& srcRect, const PPoint& dstPos)        { ForwardToView(viewHandle, &PServerView::DrawBitmap, GetBitmap(bitmapHandle), srcRect, dstPos); }
    void SlotViewDrawScaledBitmap(handler_id viewHandle, handle_id bitmapHandle, const PRect& srcRect, const PRect& dstRect)  { ForwardToView(viewHandle, &PServerView::DrawScaledBitmap, GetBitmap(bitmapHandle), srcRect, dstRect); }
    void SlotViewDebugDraw(handler_id viewHandle, PColor color, uint32_t drawFlags)          { ForwardToView(viewHandle, &PServerView::DebugDraw, color, drawFlags); }

    template<typename CB, typename... ARGS>
    void ForwardToView(handler_id viewHandle, CB callback, ARGS&&... args)
    {
        Ptr<PServerView> view = m_Server->FindView(viewHandle);
        if (view != nullptr) {
            (*view.*callback)(args...);
        } else {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerView::ForwardToView() failed to find view {}.", viewHandle);
        }
    }

    ApplicationServer*  m_Server;
    port_id             m_ClientPort;
    
//    Ptr<ServerView> m_LowestInvalidView;
//    int             m_LowestInvalidLevel = std::numeric_limits<int>::max();
    bool            m_HaveInvalidRegions = false;

    handle_id                           m_NextBitmapHandle = 1;
    std::map<handle_id, Ptr<PSrvBitmap>> m_BitmapMap;

    ASSync                      RSSync;
    ASCreateView                RSCreateView;
    ASDeleteView                RSDeleteView;
    ASFocusView                 RSFocusView;
    ASSetKeyboardFocus          RSSetKeyboardFocus;
    ASCreateBitmap              RSCreateBitmap;
    ASDeleteBitmap              RSDeleteBitmap;
    ASViewSetFrame              RSViewSetFrame;
    ASViewInvalidate            RSViewInvalidate;
    ASViewAddChild              RSViewAddChild;
    ASViewToggleDepth           RSViewToggleDepth;
    ASViewBeginUpdate           RSViewBeginUpdate;
    ASViewEndUpdate             RSViewEndUpdate;
    ASViewShow                  RSViewShow;
    ASViewSetFgColor            RSViewSetFgColor;
    ASViewSetBgColor            RSViewSetBgColor;
    ASViewSetEraseColor         RSViewSetEraseColor;
    ASViewSetFocusKeyboardMode  RSViewSetFocusKeyboardMode;
    ASViewSetDrawingMode        RSViewSetDrawingMode;
    ASViewSetFont               RSViewSetFont;
    ASViewSetPenWidth           RSViewSetPenWidth;
    ASViewMovePenTo             RSViewMovePenTo;
    ASViewDrawLine1             RSViewDrawLine1;
    ASViewDrawLine2             RSViewDrawLine2;
    ASViewFillRect              RSViewFillRect;
    ASViewFillCircle            RSViewFillCircle;
    ASViewDrawString            RSViewDrawString;
    ASViewScrollBy              RSViewScrollBy;
    ASViewCopyRect              RSViewCopyRect;
    ASViewDrawBitmap            RSViewDrawBitmap;
    ASViewDrawScaledBitmap      RSViewDrawScaledBitmap;
    ASViewDebugDraw             RSViewDebugDraw;
    
    ServerApplication(const ServerApplication&) = delete;
    ServerApplication& operator=(const ServerApplication&) = delete;
};
