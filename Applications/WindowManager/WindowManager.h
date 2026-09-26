// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
