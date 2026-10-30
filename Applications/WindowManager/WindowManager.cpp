// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 09.04.2018 22:56:33

#include <System/AppDefinition.h>

#include "WindowManager.h"
#include "VirtualKeyboardView.h"

#include <GUI/View.h>
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

WindowManager::WindowManager() : PApplication("window_manager"), m_KeyboardAnimator(PEasingCurveFunction::EaseOut), m_TargetAnimator(PEasingCurveFunction::EaseOut)
{
    g_WindowManagerPort = GetPortID();
    RSWindowManagerRegisterView.Connect(this, &WindowManager::SlotRegisterView);
    RSWindowManagerUnregisterView.Connect(this, &WindowManager::SlotUnregisterView);
    RSWindowManagerEnableVKeyboard.Connect(this, &WindowManager::SlotEnableVKeyboard);
    RSWindowManagerDisableVKeyboard.Connect(this, &WindowManager::SlotDisableVKeyboard);

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

void WindowManager::ThreadStarted()
{
    m_TopView = PViewFactory::Get().LoadView(nullptr, PStandardPaths::GetPath(PStandardPath::System, "WindowManagerLayout.xml"));
    if (m_TopView != nullptr)
    {
        m_TopView->SetFrame(ApplicationServer::GetScreenFrame());

        m_DesktopView = m_TopView->FindChild("DesktopView");
        m_SidebarView = m_TopView->FindChild("SideBar");
        m_ClientsView = m_TopView->FindChild("ClientView");

        if (m_DesktopView != nullptr) {
            m_DesktopView->SetFrame(m_TopView->GetBounds());
        }
        if (m_SidebarView != nullptr) {
            m_SidebarView->SetEraseColor(100, 100, 100);
        }
        AddView(m_TopView, PViewDockType::RootLevelView);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool WindowManager::HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    PApplication::HandleMessage(targetHandler, code, data, length);

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
    Ptr<PView> parent;
    if (dockType == PViewDockType::DockedWindow) {
        parent = m_ClientsView;
    } else if (dockType == PViewDockType::StatusBarIcon) {
        parent = m_SidebarView;
    } else if (dockType == PViewDockType::PopupWindow) {
        parent = m_TopView;
    }
    if (parent != nullptr)
    {
        const Ptr<PView> view = ptr_new<PView>(viewHandle, name, frame);
        parent->AddChild(view);
        Post<ASViewAddChild>(INVALID_INDEX, parent->GetServerHandle(), viewHandle, view->GetHandle());
        if (dockType != PViewDockType::PopupWindow) {
            view->SetFrame(parent->GetBounds());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WindowManager::SlotUnregisterView(handler_id viewHandle)
{
    if (m_TopView != nullptr)
    {
        const Ptr<PView> view = m_TopView->FindChildIf(
            [viewHandle](Ptr<PView> child) { return child->GetServerHandle() == viewHandle; });
        if (view != nullptr)
        {
            view->RemoveThis();
            RemoveView(view);
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

        keyboardFrame = m_TopView->GetBounds();
        keyboardFrame.top = keyboardFrame.bottom;
        keyboardFrame.bottom = keyboardFrame.top + m_KeyboardView->GetPreferredSize(PPrefSizeType::Smallest).y;
        m_KeyboardView->SetFrame(keyboardFrame);

        m_TopView->AddChild(m_KeyboardView);
    }
    else
    {
        m_KeyboardView->SetIsNumerical(numerical);
        keyboardFrame = m_KeyboardView->GetFrame();
        keyboardFrame.bottom = keyboardFrame.top + m_KeyboardView->GetPreferredSize(PPrefSizeType::Smallest).y;
        m_KeyboardView->SetFrame(keyboardFrame);
    }

    const PRect screenFrame = m_TopView->GetBounds();
    const float finalKeyboardTop = screenFrame.bottom - keyboardFrame.Height();

    const float currentScrollOffset = m_DesktopView->GetScrollOffset().y;
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
        const PRect screenFrame   = m_TopView->GetBounds();
        const PRect keyboardFrame = m_KeyboardView->GetFrame();

        const float finalKeyboardTop = screenFrame.bottom;

        float animTime = 0.2f;
        animTime *= fabsf(keyboardFrame.top - finalKeyboardTop) / keyboardFrame.Height();

        m_TargetAnimator.SetRange(m_DesktopView->GetScrollOffset().y, 0.0f);
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

    m_DesktopView->ScrollTo(PPoint(0.0f, std::round(m_TargetAnimator.GetValue())));

    if (!m_KeyboardAnimator.IsRunning() && !m_TargetAnimator.IsRunning())
    {
        m_KeyboardAnimTimer.Stop();
        if (!m_IsKeyboardActive)
        {
            m_KeyboardView->RemoveThis();
            m_DesktopView->ScrollTo(PPoint(0.0f, 0.0f));
            m_KeyboardView = nullptr;
        }
    }
    Sync();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int windowmanager_main(int argc, char* argv[])
{
    WindowManager* windowManager = new WindowManager();
    windowManager->Adopt();
    return 0;
}

static PAppDefinition g_WindowManagerDef("windowmanager", "Server managing top-level GUI layout.", windowmanager_main);
