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
    
    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;
    virtual void Idle() override;
    static IRect GetScreenIFrame();
    static Rect GetScreenFrame() { return Rect(GetScreenIFrame()); }
    
    bool AddView(Ptr<View> view, ViewDockType dockType);
    bool RemoveView(Ptr<View> view);
    
    Ptr<View> FindView(handler_id handle) { return ptr_static_cast<View>(FindHandler(handle)); }

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args) { SIGNAL::Sender::Emit(this, &Application::AllocMessageBuffer, args...); }
     
    void Flush();
    void Sync();
    
private:
    friend class View;
 
    
    void DetachView(Ptr<View> view);
    
    void* AllocMessageBuffer(int32_t messageID, size_t size);

    void RegisterViewForLayout(Ptr<View> view) { m_ViewsNeedingLayout.insert(view); }
        
    MessagePort m_ReplyPort;
    handler_id m_ServerHandle = -1;

    uint8_t m_SendBuffer[APPSERVER_MSG_BUFFER_SIZE]; 
    int32_t m_UsedSendBufferSize = 0;

    std::set<Ptr<View>> m_ViewsNeedingLayout;


    Application(const Application &) = delete;
    Application& operator=(const Application &) = delete;
};

}