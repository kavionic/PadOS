// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.09.2020 22:30

#pragma once

#include <stdint.h>

#include <GUI/View.h>
#include <Threads/EventTimer.h>
#include <Utils/UTF8Utils.h>

namespace os
{


struct KeyButton
{
    KeyButton(const Point& relativePos, float relativeWidth, KeyCodes keyCode) : m_Width(relativeWidth), m_RelativePos(relativePos), m_KeyCode(keyCode) {}

    Rect        m_Frame;
    float       m_Width;
    Point       m_RelativePos;
    KeyCodes    m_KeyCode;
};

enum class CapsLockMode : uint8_t
{
    Off,
    Single,
    Locked
};

class KeyboardView : public View
{
public:
    KeyboardView(const String& name, Ptr<View> parent = nullptr, uint32_t flags = 0);

    // From View:
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;
    virtual void FrameSized(const Point& delta) override;
    virtual void Paint(const Rect& updateRect) override;

    virtual bool OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;

    Signal<void, KeyCodes, const String&, KeyboardView*> SignalKeyPressed;
private:
    String  GetKeyText(const KeyButton& button) const;
    void    DrawButton(const KeyButton& button, bool pressed);
    void    SetPressedButton(size_t index);
    size_t  GetButtonIndex(const Point& position) const;

    Ptr<Bitmap>             m_KeysBitmap;
    EventTimer              m_RepeatTimer;

    float                   m_KeyHeight = 0.0f;

    std::vector<KeyButton>  m_KeyButtons;
    MouseButton_e           m_HitButton = MouseButton_e::None;
    Point                   m_HitPos;
    size_t                  m_PressedButton = INVALID_INDEX;
    CapsLockMode            m_CapsLockMode = CapsLockMode::Single;
    bool                    m_SymbolsActive = false;

};

} // namespace os

