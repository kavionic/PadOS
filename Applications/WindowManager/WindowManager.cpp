// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include <System/AppDefinition.h>

#include "WindowManager.h"
#include "VirtualKeyboardView.h"

#include <GUI/View.h>
#include <GUI/LayoutNode.h>
#include <GUI/Widgets/Button.h>
#include <GUI/ViewFactory.h>
#include <ApplicationServer/ApplicationServer.h>
#include <Storage/StandardPaths.h>


static port_id g_WindowManagerPort = -1;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

port_id p_get_window_manager_port()
{
    return g_WindowManagerPort;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class WindowBar : public PView
{
public:
    WindowBar(Ptr<PView> parent) : PView("wmgr_window_bar", parent, PViewFlags::WillDraw)
    {
        SetLayoutNode(ptr_new<PVLayoutNode>());
        SetWidthOverride(PPrefSizeType::All, PSizeOverride::Always, 100.0f);
        SetHeightOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 0.0f);
        SetHeightOverride(PPrefSizeType::Greatest, PSizeOverride::Always, LAYOUT_MAX_SIZE);
    }
    
    virtual void OnPaint(const PRect& updateRect) override
    {
        SetFgColor(100, 100, 100);
        FillRect(GetBounds());
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class WindowIcon : public PButton
{
public:
    WindowIcon(Ptr<PView> parent, Ptr<PView> window) : PButton("wicon", window->GetName(), parent), m_Window(window)
    {
        SignalActivated.Connect(this, &WindowIcon::SlotClicked);
    }
    
private:
    void SlotClicked()
    {
        m_Window->ToggleDepth();
    }
    Ptr<PView> m_Window;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WindowManager::WindowManager() : PApplication("window_manager"), m_KeyboardAnimator(PEasingCurveFunction::EaseOut), m_TargetAnimator(PEasingCurveFunction::EaseOut)
{
    g_WindowManagerPort = GetPortID();
    RSWindowManagerRegisterView.Connect(this, &WindowManager::SlotRegisterView);
    RSWindowManagerUnregisterView.Connect(this, &WindowManager::SlotUnregisterView);
    RSWindowManagerEnableVKeyboard.Connect(this, &WindowManager::SlotEnableVKeyboard);
    RSWindowManagerDisableVKeyboard.Connect(this, &WindowManager::SlotDisableVKeyboard);

    m_TopView = PViewFactory::Get().LoadView(nullptr, PStandardPaths::GetPath(PStandardPath::System, "WindowManagerLayout.xml"));
    if (m_TopView != nullptr)
    {
        m_TopView->SetFrame(ApplicationServer::GetScreenFrame());
        m_TopView->SetLayoutNode(ptr_new<PHLayoutNode>());

        m_SidebarView = m_TopView->FindChild("SideBar");
        m_ClientsView = m_TopView->FindChild("ClientView");

        if (m_SidebarView != nullptr)
        {
            m_SidebarView->SetEraseColor(100, 100, 100);
        }
        AddView(m_TopView, PViewDockType::RootLevelView);
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
    case PAppserverProtocol::WINDOW_MANAGER_REGISTER_VIEW:
        RSWindowManagerRegisterView.Dispatch(data, length);
        return true;
    case PAppserverProtocol::WINDOW_MANAGER_UNREGISTER_VIEW:
        RSWindowManagerUnregisterView.Dispatch(data, length);
        return true;
    case PAppserverProtocol::WINDOW_MANAGER_ENABLE_VKEYBOARD:
        RSWindowManagerEnableVKeyboard.Dispatch(data, length);
        return true;
    case PAppserverProtocol::WINDOW_MANAGER_DISABLE_VKEYBOARD:
        RSWindowManagerDisableVKeyboard.Dispatch(data, length);
        return true;
    default:
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotRegisterView(handler_id viewHandle, PViewDockType dockType, const PString& name, const PRect& frame)
{
    if (m_ClientsView != nullptr)
    {
        Ptr<PView> view = ptr_new<PView>(m_ClientsView, viewHandle, name, frame);

        Post<ASViewAddChild>(INVALID_INDEX, m_ClientsView->GetServerHandle(), viewHandle, view->GetHandle());

        if (dockType == PViewDockType::DockedWindow)
        {
            if (m_SidebarView != nullptr)
            {
                Ptr<PView> prevIcon = m_SidebarView->GetChildAt(0);
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
        Ptr<PView> view = *i;
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

void WindowManager::SlotEnableVKeyboard(const PRect& focusViewEditArea, bool numerical)
{
    m_IsKeyboardActive = true;

    PRect keyboardFrame;
    if (m_KeyboardView == nullptr)
    {
        m_KeyboardView = ptr_new<PVirtualKeyboardView>(numerical);
        m_KeyboardView->PreferredSizeChanged();

        keyboardFrame = m_ClientsView->GetBounds();
        keyboardFrame.top = keyboardFrame.bottom;
        keyboardFrame.bottom = keyboardFrame.top + m_KeyboardView->GetPreferredSize(PPrefSizeType::Smallest).y;
        m_KeyboardView->SetFrame(keyboardFrame);

        AddView(m_KeyboardView, PViewDockType::RootLevelView);
    }
    else
    {
        m_KeyboardView->SetIsNumerical(numerical);
        keyboardFrame = m_KeyboardView->GetFrame();
        keyboardFrame.bottom = keyboardFrame.top + m_KeyboardView->GetPreferredSize(PPrefSizeType::Smallest).y;
    }

    const PRect screenFrame = m_ClientsView->GetBounds();
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
        const PRect screenFrame   = m_ClientsView->GetBounds();
        const PRect keyboardFrame = m_KeyboardView->GetFrame();

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
    PRect frame = m_KeyboardView->GetBounds() + PPoint(0.0f, std::round(m_KeyboardAnimator.GetValue()));
    m_KeyboardView->SetFrame(frame);

    m_TopView->ScrollTo(PPoint(0.0f, std::round(m_TargetAnimator.GetValue())));

    if (!m_KeyboardAnimator.IsRunning())
    {
        m_KeyboardAnimTimer.Stop();
        if (!m_IsKeyboardActive)
        {
            RemoveView(m_KeyboardView);
            m_TopView->ScrollTo(PPoint(0.0f, 0.0f));
            m_KeyboardView = nullptr;
        }
    }
    Sync();
}

int windowmanager_main(int argc, char* argv[])
{
    WindowManager* windowManager = new WindowManager();
    windowManager->Adopt();
    return 0;
}

static PAppDefinition g_WindowManagerDef("windowmanager", "Server managing top-level GUI layout.", windowmanager_main);
