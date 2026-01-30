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

#include <System/SystemMessageIDs.h>
#include <GUI/GUIDefines.h>


enum class PMouseButton : uint32_t
{
    None,
    Left,
    Middle,
    Right,
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

enum class PMotionToolType : uint32_t
{
    Mouse,
    Finger,
    Stylus,
    Eraser
};

static constexpr PMotionToolType GetMotionToolType(PMouseButton button) { return button < PMouseButton::FirstTouchID ? PMotionToolType::Mouse : PMotionToolType::Finger; }

struct PInputEvent
{
    TimeValNanos    Timestamp;
    PMessageID       EventID;

};

struct PMotionEvent : PInputEvent
{
    PMotionToolType  ToolType;
    PMouseButton   ButtonID;
    PPoint           Position;
};

struct PKeyEvent : PInputEvent
{
    static constexpr size_t MAX_TEXT_LENGTH = 11;

    PKeyCodes    m_KeyCode;
    char        m_Text[MAX_TEXT_LENGTH + 1];
};
