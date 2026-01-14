// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 13.01.2026 22:00

#pragma once

enum class PANSIControlCode
{
    None,
    Pending,
    Backspace,
    VT_Keycode = '~',
    XTerm_Up = 'A',
    XTerm_Down = 'B',
    XTerm_Right = 'C',
    XTerm_Left = 'D',
    XTerm_End = 'F',
    XTerm_Keypad_5 = 'G',
    XTerm_Home = 'H',
    XTerm_F1 = 'P',   // With '1' as an argument.
    XTerm_F2 = 'Q',   // With '1' as an argument.
    XTerm_F3 = 'R',   // With '1' as an argument.
    XTerm_F4 = 'S',   // With '1' as an argument.
    XTerm_XTWINOPS = 't',
    EraseDisplay = 'J'
};

enum class PANSI_VT_KeyCodes
{
    Home_1 = 1,
    Insert = 2,
    Delete = 3,
    End_4 = 4,
    PgUp = 5,
    PgDn = 6,
    Home_7 = 7,
    End_8 = 8,
    F0 = 10,
    F1 = 11,
    F2 = 12,
    F3 = 13,
    F4 = 14,
    F5 = 15,
    F6 = 17,
    F7 = 18,
    F8 = 19,
    F9 = 20,
    F10 = 21,
    F11 = 23,
    F12 = 24,
    F13 = 25,
    F14 = 26,
    F15 = 28,
    F16 = 29,
    F17 = 31,
    F18 = 32,
    F19 = 33,
    F20 = 34
};

static constexpr int XTerm_XTWINOPS_ReportTextAreaSize = 18;

class PANSIEscapeCodeParser
{
public:
    PANSIControlCode ProcessCharacter(char character);
    const std::vector<int>& GetArguments() const { return m_CodeArgs; }

    PString FormatANSICode(PANSIControlCode code, std::vector<int> args);

    template<typename... TArgTypes>
    PString FormatANSICode(PANSIControlCode code, TArgTypes ...args)
    {
        return FormatANSICode(code, { args... });
    }

private:
    enum class EControlState { None, WaitingForStart, WaitingForEnd };

    EControlState    m_ControlState = EControlState::None;
    std::vector<int> m_CodeArgs;

};
