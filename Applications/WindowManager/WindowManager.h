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

#include <App/Application.h>
#include <Threads/EventTimer.h>
#include <Utils/Utils.h>
#include <Utils/ValueAnimator.h>
#include <Utils/EasingCurve.h>


class PVirtualKeyboardView;

class WindowManager : public PApplication
{
public:
    WindowManager();
    ~WindowManager();

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;

private:
    void SlotRegisterView(handler_id viewHandle, PViewDockType dockType, const PString& name, const PRect& frame);
    void SlotUnregisterView(handler_id viewHandle);
    void SlotEnableVKeyboard(const PRect& focusViewEditArea, bool numerical);
    void SlotDisableVKeyboard();
    void SlotKeyboardAnimTimer();

    ASWindowManagerRegisterView::Receiver       RSWindowManagerRegisterView;
    ASWindowManagerUnregisterView::Receiver     RSWindowManagerUnregisterView;
    ASWindowManagerEnableVKeyboard::Receiver    RSWindowManagerEnableVKeyboard;
    ASWindowManagerDisableVKeyboard::Receiver   RSWindowManagerDisableVKeyboard;

    Ptr<PView> m_TopView;
    Ptr<PView> m_SidebarView;
    Ptr<PView> m_ClientsView;

    Ptr<PVirtualKeyboardView>        m_KeyboardView;
    bool                                m_IsKeyboardActive = false;
    PEventTimer                          m_KeyboardAnimTimer;
    PValueAnimator<float, PEasingCurve>   m_KeyboardAnimator;
    PValueAnimator<float, PEasingCurve>   m_TargetAnimator;

    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
};
