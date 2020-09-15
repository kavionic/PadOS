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

#include "System/Platform.h"

#include "WindowManager.h"
#include "GUI/View.h"
#include "GUI/LayoutNode.h"
#include "ApplicationServer/ApplicationServer.h"
#include "GUI/Button.h"
#include "GUI/ViewFactory.h"
#include "VirtualKeyboardView.h"

using namespace os;

static port_id g_WindowManagerPort = -1;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

port_id os::get_window_manager_port()
{
    return g_WindowManagerPort;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class WindowBar : public View
{
public:
    WindowBar(Ptr<View> parent) : View("wmgr_window_bar", parent, ViewFlags::WillDraw)
    {
        SetLayoutNode(ptr_new<VLayoutNode>());
        SetWidthOverride(PrefSizeType::All, SizeOverride::Always, 100.0f);
        SetHeightOverride(PrefSizeType::Smallest, SizeOverride::Always, 0.0f);
        SetHeightOverride(PrefSizeType::Greatest, SizeOverride::Always, LAYOUT_MAX_SIZE);
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
    WindowIcon(Ptr<View> parent, Ptr<View> window) : Button("wicon", window->GetName(), parent), m_Window(window)
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

WindowManager::WindowManager() : Application("window_manager"), m_KeyboardAnimator(EasingCurveFunction::EaseOut), m_TargetAnimator(EasingCurveFunction::EaseOut)
{
    g_WindowManagerPort = GetPortID();
    RSWindowManagerRegisterView.Connect(this, &WindowManager::SlotRegisterView);
    RSWindowManagerUnregisterView.Connect(this, &WindowManager::SlotUnregisterView);
    RSWindowManagerEnableVKeyboard.Connect(this, &WindowManager::SlotEnableVKeyboard);
    RSWindowManagerDisableVKeyboard.Connect(this, &WindowManager::SlotDisableVKeyboard);

    m_TopView = ViewFactory::GetInstance().LoadView(nullptr, "/sdcard/Rainbow3D/System/WindowManagerLayout.xml");
    if (m_TopView != nullptr)
    {
        m_TopView->SetFrame(ApplicationServer::GetScreenFrame());
        m_TopView->SetLayoutNode(ptr_new<HLayoutNode>());

        m_SidebarView = m_TopView->FindChild("SideBar");
        m_ClientsView = m_TopView->FindChild("ClientView");

        if (m_SidebarView != nullptr)
        {
            m_SidebarView->SetEraseColor(100, 100, 100);
        }
        AddView(m_TopView, ViewDockType::RootLevelView);
    }

    m_KeyboardAnimator.SetPeriod(0.3);
    m_TargetAnimator.SetPeriod(0.25);

    m_KeyboardAnimTimer.Set(1.0 / 30.0);
    m_KeyboardAnimTimer.SignalTrigged.Connect(this, &WindowManager::SlotKeyboardAnimTimer);
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
    switch (code)
    {
    case AppserverProtocol::WINDOW_MANAGER_REGISTER_VIEW:
        RSWindowManagerRegisterView.Dispatch(data, length);
        return true;
    case AppserverProtocol::WINDOW_MANAGER_UNREGISTER_VIEW:
        RSWindowManagerUnregisterView.Dispatch(data, length);
        return true;
    case AppserverProtocol::WINDOW_MANAGER_ENABLE_VKEYBOARD:
        RSWindowManagerEnableVKeyboard.Dispatch(data, length);
        return true;
    case AppserverProtocol::WINDOW_MANAGER_DISABLE_VKEYBOARD:
        RSWindowManagerDisableVKeyboard.Dispatch(data, length);
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
    if (m_ClientsView != nullptr)
    {
        Ptr<View> view = ptr_new<View>(m_ClientsView, viewHandle, name, frame);

        Post<ASViewAddChild>(m_ClientsView->GetServerHandle(), viewHandle, view->GetHandle());
        AddHandler(view);
        if (dockType == ViewDockType::DockedWindow)
        {
            if (m_SidebarView != nullptr)
            {
                Ptr<View> prevIcon = m_SidebarView->GetChildAt(0);
                Ptr<WindowIcon> windowIcon = ptr_new<WindowIcon>(m_SidebarView, view);

                if (prevIcon != nullptr) {
                    windowIcon->AddToWidthRing(prevIcon);
                }
            }
            view->SetFrame(m_ClientsView->GetBounds());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotUnregisterView(handler_id viewHandle)
{
    for (auto i = m_ClientsView->begin(); i != m_ClientsView->end(); ++i)
    {
        Ptr<View> view = *i;
        if (view->GetHandle() == viewHandle)
        {
            m_ClientsView->RemoveChild(i);
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotEnableVKeyboard(const Rect& focusViewEditArea, bool numerical)
{
    m_IsKeyboardActive = true;

    Rect keyboardFrame;
    if (m_KeyboardView == nullptr)
    {
        m_KeyboardView = ptr_new<VirtualKeyboardView>(numerical);
        m_KeyboardView->PreferredSizeChanged();

        keyboardFrame = m_ClientsView->GetBounds();
        keyboardFrame.top = keyboardFrame.bottom;
        keyboardFrame.bottom = keyboardFrame.top + m_KeyboardView->GetPreferredSize(PrefSizeType::Smallest).y;
        m_KeyboardView->SetFrame(keyboardFrame);

        AddView(m_KeyboardView, ViewDockType::RootLevelView);
    }
    else
    {
        m_KeyboardView->SetIsNumerical(numerical);
        keyboardFrame = m_KeyboardView->GetFrame();
        keyboardFrame.bottom = keyboardFrame.top + m_KeyboardView->GetPreferredSize(PrefSizeType::Smallest).y;
    }

    const Rect screenFrame = m_ClientsView->GetBounds();
    const float finalKeyboardTop = screenFrame.bottom - keyboardFrame.Height();

    const float currentScrollOffset = m_TopView->GetScrollOffset().y;
    float targetScrollOffset = 0.0f;
    if (focusViewEditArea.bottom - currentScrollOffset >= finalKeyboardTop) {
        targetScrollOffset = (finalKeyboardTop - focusViewEditArea.Height()) * 0.5f - focusViewEditArea.top + currentScrollOffset;
    }

    float animTime = 0.3f;
    animTime *= fabsf(keyboardFrame.top - finalKeyboardTop) / keyboardFrame.Height();

    m_TargetAnimator.SetRange(currentScrollOffset, targetScrollOffset);
    m_KeyboardAnimator.SetRange(keyboardFrame.top, finalKeyboardTop);

    m_TargetAnimator.SetPeriod(animTime);
    m_KeyboardAnimator.SetPeriod(animTime * 0.95f);

    m_TargetAnimator.Start();
    m_KeyboardAnimator.Start();
    m_KeyboardAnimTimer.Start();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotDisableVKeyboard()
{
    m_IsKeyboardActive = false;
    if (m_KeyboardView != nullptr)
    {
        const Rect screenFrame   = m_ClientsView->GetBounds();
        const Rect keyboardFrame = m_KeyboardView->GetFrame();

        const float finalKeyboardTop = screenFrame.bottom;

        float animTime = 0.2f;
        animTime *= fabsf(keyboardFrame.top - finalKeyboardTop) / keyboardFrame.Height();

        m_TargetAnimator.SetRange(m_TopView->GetScrollOffset().y, 0.0f);
        m_KeyboardAnimator.SetRange(keyboardFrame.top, screenFrame.bottom);

        m_TargetAnimator.SetPeriod(animTime * 0.95f);
        m_KeyboardAnimator.SetPeriod(animTime);

        m_TargetAnimator.Start();
        m_KeyboardAnimator.Start();
        m_KeyboardAnimTimer.Start();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotKeyboardAnimTimer()
{
    Rect frame = m_KeyboardView->GetBounds() + Point(0.0f, round(m_KeyboardAnimator.GetValue()));
    m_KeyboardView->SetFrame(frame);

    m_TopView->ScrollTo(Point(0.0f, round(m_TargetAnimator.GetValue())));

    if (!m_KeyboardAnimator.IsRunning())
    {
        m_KeyboardAnimTimer.Stop();
        if (!m_IsKeyboardActive)
        {
            RemoveView(m_KeyboardView);
            m_TopView->ScrollTo(Point(0.0f, 0.0f));
            m_KeyboardView = nullptr;
        }
    }
    Sync();
}
