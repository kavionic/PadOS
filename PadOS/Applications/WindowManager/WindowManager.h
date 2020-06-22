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
// Created: 09.04.2018 22:56:33

#pragma once

#include "App/Application.h"



class WindowManager : public os::Application
{
public:
    WindowManager();
    ~WindowManager();

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;

private:
    void SlotRegisterView(handler_id viewHandle, os::ViewDockType dockType, const os::String& name, const os::Rect& frame);
    void SlotUnregisterView(handler_id viewHandle);

    os::ASWindowManagerRegisterView::Receiver RSWindowManagerRegisterView;
    os::ASWindowManagerUnregisterView::Receiver RSWindowManagerUnregisterView;
    
    Ptr<os::View> m_TopView;
    Ptr<os::View> m_SidebarView;
    Ptr<os::View> m_ClientsView;
    
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
};


