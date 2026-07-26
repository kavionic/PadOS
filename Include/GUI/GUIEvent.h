// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 30.01.2014 22:23:14

#pragma once

#include <stdint.h>
#include <utility>

#include <System/SystemMessageIDs.h>
#include <Math/Point.h>
#include <GUI/GUIDefines.h>


enum class PMouseButton : uint32_t
{
    None,
    Left,
    Middle,
    Right,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,
    FirstTouchID = 100,
    Touch0 = FirstTouchID,
    Touch1,
    Touch2,
    Touch3,
    Touch4,
    Touch5,
    Touch6,
    Touch7,
    Touch8,
    Touch9,
    LastTouchID = Touch9
};

using PPointerID = uint32_t;
using PPointerCaptureID = uint32_t;
using PPointerCaptureRequestID = uint32_t;
using PPointerButtonMask = uint32_t;

static constexpr PPointerID PInvalidPointerID = UINT32_MAX;
static constexpr PPointerID PMousePointerID = 0;
static constexpr PPointerID PFirstTouchPointerID = 1;
static constexpr PPointerButtonMask PPointerButtonMaskNone = 0;
static constexpr PPointerCaptureID PInvalidPointerCaptureID = 0;
static constexpr PPointerCaptureID PFirstPointerCaptureID = 1;
static constexpr PPointerCaptureRequestID PInvalidPointerCaptureRequestID = 0;
static constexpr PPointerCaptureRequestID PFirstPointerCaptureRequestID = 1;

enum class PPointerCaptureMode : uint8_t
{
    Preemptible,
    Locked
};

enum class PPointerCaptureLostReason : uint8_t
{
    Released,
    PointerUp,
    PointerCancel,
    Stolen,
    ViewDetached,
    Rejected
};

enum class PEventPhase : uint8_t
{
    Capture,
    Target,
    Bubble
};

static constexpr uint8_t PEventPhaseMask(PEventPhase phase)
{
    return uint8_t(1U << std::to_underlying(phase));
}

static constexpr PPointerID GetPointerID(PMouseButton button)
{
    return (button < PMouseButton::FirstTouchID)
        ? PMousePointerID
        : PPointerID(std::to_underlying(button) - std::to_underlying(PMouseButton::FirstTouchID) + PFirstTouchPointerID);
}

static constexpr PPointerID GetTouchPointerID(uint32_t touchID)
{
    return PFirstTouchPointerID + touchID;
}

static constexpr PPointerButtonMask GetPointerButtonMask(PMouseButton button)
{
    return (button >= PMouseButton::Left && button <= PMouseButton::Button8)
        ? PPointerButtonMask(1U << (std::to_underlying(button) - std::to_underlying(PMouseButton::Left)))
        : PPointerButtonMaskNone;
}

enum class PMotionToolType : uint32_t
{
    Mouse,
    Finger,
    Stylus,
    Eraser
};

static constexpr PMotionToolType GetMotionToolType(PMouseButton button) { return (button < PMouseButton::FirstTouchID) ? PMotionToolType::Mouse : PMotionToolType::Finger; }

enum class PInputEventType : uint16_t
{
    InputEvent,
    MouseEvent,
    TouchEvent,
    KeyEvent
};

enum class PInputClass : uint16_t
{
    Keyboard,
    Mouse,
    TouchScreen
};

enum class PInputEventID : uint32_t
{
    MouseDown,
    MouseUp,
    MouseMove,
    TouchDown,
    TouchUp,
    TouchMove,
    KeyDown,
    KeyUp,
    MouseWheel
};

struct PInputEvent
{
    uint32_t        EventSize;
    PInputEventType EventType;
    PInputClass     ClassID;
    TimeValNanos    Timestamp;
    PInputEventID   EventID;
    int32_t         SourceID;
};

static_assert(sizeof(PInputEvent) == 24);

struct PMouseEvent : PInputEvent
{
    PMouseButton        Button;
    PPointerButtonMask  Buttons;
    PPoint              Position;
};

struct PTouchEvent : PInputEvent
{
    uint32_t            TouchID;
    PMotionToolType     ToolType;
    float               Pressure;
    PPoint              Position;
};

enum class PPointerEventType
{
    Invalid,
    Down,
    Up,
    Move,
    Cancel,
    Enter,
    Leave,
    Over,
    Out
};

struct PPointerEvent
{
    PPointerID          PointerID = PInvalidPointerID;
    PPointerEventType   EventType = PPointerEventType::Invalid;
    TimeValNanos        Timestamp;
    PMotionToolType     ToolType = PMotionToolType::Mouse;
    bool                SupportsHover = false;
    PMouseButton        Button = PMouseButton::None;
    PPointerButtonMask  Buttons = PPointerButtonMaskNone;
    float               Pressure = 0.0f;
    PPoint              ScreenPosition;
    PPoint              ViewPosition;
};

struct PKeyEvent : PInputEvent
{
    static constexpr size_t MAX_TEXT_LENGTH = 11;

    PKeyCodes    m_KeyCode;
    char        m_Text[MAX_TEXT_LENGTH + 1];
};
