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


namespace os
{
class VirtualKeyboardView;

class WindowManager : public os::Application
{
public:
    WindowManager();
    ~WindowManager();

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;

private:
    void SlotRegisterView(handler_id viewHandle, os::ViewDockType dockType, const os::String& name, const os::Rect& frame);
    void SlotUnregisterView(handler_id viewHandle);
    void SlotEnableVKeyboard(const Rect& focusViewEditArea, bool numerical);
    void SlotDisableVKeyboard();
    void SlotKeyboardAnimTimer();

    os::ASWindowManagerRegisterView::Receiver       RSWindowManagerRegisterView;
    os::ASWindowManagerUnregisterView::Receiver     RSWindowManagerUnregisterView;
    os::ASWindowManagerEnableVKeyboard::Receiver    RSWindowManagerEnableVKeyboard;
    os::ASWindowManagerDisableVKeyboard::Receiver   RSWindowManagerDisableVKeyboard;

    Ptr<os::View> m_TopView;
    Ptr<os::View> m_SidebarView;
    Ptr<os::View> m_ClientsView;

    Ptr<os::VirtualKeyboardView>        m_KeyboardView;
    bool                                m_IsKeyboardActive = false;
    EventTimer                          m_KeyboardAnimTimer;
    ValueAnimator<float, EasingCurve>   m_KeyboardAnimator;
    ValueAnimator<float, EasingCurve>   m_TargetAnimator;

    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
};


} // namespace os
