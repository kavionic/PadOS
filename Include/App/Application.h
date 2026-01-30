// This file is part of PadOS.
//
// Copyright (C) 2017-2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.11.2017 23:22:03

#pragma once

#include <set>

#include <Math/Rect.h>
#include <Threads/Looper.h>
#include <Threads/EventTimer.h>
#include <ApplicationServer/Protocol.h>

class PView;


class PApplication : public PLooper, public SignalTarget
{
public:
    PApplication(const PString& name);
    ~PApplication();

    static PApplication* GetDefaultApplication();
    static void SetDefaultApplication(PApplication* application);

    static PApplication* GetCurrentApplication() { return dynamic_cast<PApplication*>(GetCurrentThread()); }

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;
    virtual void Idle() override;
    static PIRect    GetScreenIFrame();
    static PRect     GetScreenFrame() { return PRect(GetScreenIFrame()); }
    
    bool AddView(Ptr<PView> view, PViewDockType dockType, size_t index = INVALID_INDEX);
    bool AddChildView(Ptr<PView> parent, Ptr<PView> view, size_t index = INVALID_INDEX);
    bool RemoveView(Ptr<PView> view);
    
    Ptr<PView> FindView(handler_id handle);


    void SetFocusView(PMouseButton button, Ptr<PView> view, bool focus);
    Ptr<PView> GetFocusView(PMouseButton button) const;

    void SetKeyboardFocus(Ptr<PView> view, bool focus, bool notifyServer);
    Ptr<PView> GetKeyboardFocus() const;

    bool CreateBitmap(int width, int height, PEColorSpace colorSpace, uint32_t flags, handle_id& outHandle, uint8_t*& inOutFramebuffer, size_t& inOutBytesPerRow);
    void DeleteBitmap(handle_id bitmapHandle);

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args) { SIGNAL::Sender::Emit(this, &PApplication::AllocMessageBuffer, SIGNAL::GetID(), args...); }
     
    void Flush();
    void Sync();

    uint32_t GetQualifiers() const { return 0; }
private:
    friend class PView;
 
    
    void DetachView(Ptr<PView> view);
    
    void* AllocMessageBuffer(int32_t messageID, size_t size);

    bool CreateServerView(Ptr<PView> view, handler_id parentHandle, PViewDockType dockType, size_t index);
    void RegisterViewForLayout(Ptr<PView> view, bool recursive = false);

    void      SetMouseDownView(PMouseButton button, Ptr<PView> view, const PMotionEvent& motionEvent);
    Ptr<PView> GetMouseDownView(PMouseButton button) const;

    void LayoutViews();

    void SlotLongPressTimer();

    static PApplication* s_DefaultApplication;
    PMessagePort m_ReplyPort;
    handler_id m_ServerHandle = -1;

    uint8_t m_SendBuffer[PAPPSERVER_MSG_BUFFER_SIZE]; 
    int32_t m_UsedSendBufferSize = 0;

    std::map<int, PView*>    m_MouseViewMap;    // Maps pointing device or touch point to view last hit.
    std::map<int, PView*>    m_MouseFocusMap;   // Map of focused view per pointing device or touch point.
    PMotionEvent             m_LastClickEvent;
    PView*                   m_KeyboardFocusView = nullptr;

    PEventTimer              m_LongPressTimer;

    std::set<Ptr<PView>>     m_ViewsNeedingLayout;


    PApplication(const PApplication &) = delete;
    PApplication& operator=(const PApplication &) = delete;
};
