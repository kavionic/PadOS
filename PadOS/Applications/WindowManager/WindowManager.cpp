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

#include "sam.h"

#include "WindowManager.h"
#include "System/GUI/View.h"
#include "System/GUI/LayoutNode.h"
#include "ApplicationServer/ApplicationServer.h"
#include "System/GUI/Button.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class WindowBar : public View
{
public:
    WindowBar(Ptr<View> parent) : View("wmgr_window_bar", parent)
    {
        SetLayoutNode(ptr_new<VLayoutNode>());
        GetLayoutNode()->ExtendMinSize(Point(100.0f, 0));
        GetLayoutNode()->LimitMaxSize(Point(100.0f, LAYOUT_MAX_SIZE));        
        GetLayoutNode()->ExtendMaxSize(Point(100.0f, LAYOUT_MAX_SIZE));
    }
    
    virtual void Paint(const Rect& updateRect) override
    {
        SetFgColor(100, 100, 100);
        FillRect(GetBounds());
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class WindowIcon : public Button
{
public:
    WindowIcon(Ptr<View> parent, Ptr<View> window) : Button("wicon", /*String(window->GetName(), 0, 1)*/window->GetName(), parent), m_Window(window)
    {
        SignalActivated.Connect(this, &WindowIcon::SlotClicked);
    }
    
private:
    void SlotClicked()
    {
        m_Window->ToggleDepth();
    }
    Ptr<View> m_Window;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WindowManager::WindowManager() : Application("window_manager", true)
{
    RSWindowManagerRegisterView.Connect(this, &WindowManager::SlotRegisterView);
    RSWindowManagerUnregisterView.Connect(this, &WindowManager::SlotUnregisterView);
    
    m_TopView = ptr_new<View>("wmgr_root", nullptr, ViewFlags::IGNORE_MOUSE);
    m_TopView->SetFrame(ApplicationServer::GetScreenFrame());
    m_TopView->SetLayoutNode(ptr_new<HLayoutNode>());
    
    m_SidebarView = ptr_new<WindowBar>(m_TopView);
    
    m_ClientsView = ptr_new<View>("wmgr_clients", m_TopView, ViewFlags::IGNORE_MOUSE);
    m_ClientsView->SetLayoutNode(ptr_new<LayoutNode>());
    m_ClientsView->GetLayoutNode()->ExtendMaxSize(Point(LAYOUT_MAX_SIZE, LAYOUT_MAX_SIZE));
    
    AddView(m_TopView, ViewDockType::RootLevelView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WindowManager::~WindowManager()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool WindowManager::HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    switch(code)
    {
        case AppserverProtocol::WINDOW_MANAGER_REGISTER_VIEW:
            RSWindowManagerRegisterView.Dispatch(data, length);
            return true;
        case AppserverProtocol::WINDOW_MANAGER_UNREGISTER_VIEW:
            RSWindowManagerUnregisterView.Dispatch(data, length);
            return true;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotRegisterView(handler_id viewHandle, ViewDockType dockType, const String& name, const Rect& frame)
{
    Ptr<View> view = ptr_new<View>(m_ClientsView, viewHandle, name, frame);
    
    Post<ASViewAddChild>(m_ClientsView->GetServerHandle(), viewHandle, view->GetHandle());
    AddHandler(view);

    Ptr<WindowIcon> windowIcon = ptr_new<WindowIcon>(m_SidebarView, view);
    
    m_TopView->InvalidateLayout();
    m_ClientsView->InvalidateLayout();
    m_SidebarView->InvalidateLayout();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotUnregisterView(handler_id viewHandle)
{
    
}
