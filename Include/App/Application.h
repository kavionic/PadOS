// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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

#include "Threads/Looper.h"
#include "ApplicationServer/Protocol.h"

namespace os
{

class View;
    
class Application : public Looper, public SignalTarget
{
public:
    Application(const String& name);
    ~Application();

    static Application* GetCurrentApplication() { return dynamic_cast<Application*>(GetCurrentThread()); }

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;
    virtual void Idle() override;
    static IRect    GetScreenIFrame();
    static Rect     GetScreenFrame() { return Rect(GetScreenIFrame()); }
    
    bool AddView(Ptr<View> view, ViewDockType dockType);
    bool RemoveView(Ptr<View> view);
    
    Ptr<View> FindView(handler_id handle);


    void SetFocusView(MouseButton_e button, Ptr<View> view, bool focus);
    Ptr<View> GetFocusView(MouseButton_e button) const;

    bool CreateBitmap(int width, int height, ColorSpace colorSpace, uint32_t flags, handle_id* outHandle, uint8_t** outFramebuffer);
    void DeleteBitmap(handle_id bitmapHandle);

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args) { SIGNAL::Sender::Emit(this, &Application::AllocMessageBuffer, args...); }
     
    void Flush();
    void Sync();

    uint32_t GetQualifiers() const { return 0; }
private:
    friend class View;
 
    
    void DetachView(Ptr<View> view);
    
    void* AllocMessageBuffer(int32_t messageID, size_t size);

    void RegisterViewForLayout(Ptr<View> view);

    void      SetMouseDownView(MouseButton_e button, Ptr<View> view);
    Ptr<View> GetMouseDownView(MouseButton_e button) const;

    MessagePort m_ReplyPort;
    handler_id m_ServerHandle = -1;

    uint8_t m_SendBuffer[APPSERVER_MSG_BUFFER_SIZE]; 
    int32_t m_UsedSendBufferSize = 0;

    std::map<int, View*>   m_MouseViewMap;    // Maps pointing device or touch point to view last hit.
    std::map<int, View*>   m_MouseFocusMap;   // Map of focused view per pointing device or touch point.

    std::set<Ptr<View>> m_ViewsNeedingLayout;


    Application(const Application &) = delete;
    Application& operator=(const Application &) = delete;
};

}