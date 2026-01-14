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


#include <Utils/ANSIEscapeCodeParser.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PANSIControlCode PANSIEscapeCodeParser::ProcessCharacter(char character)
{
    if (character == 0x08 || character == 0x7f) {
        return PANSIControlCode::Backspace;
    }

    switch (m_ControlState)
    {
        case EControlState::None:
            if (character == 0x1b)
            {
                m_ControlState = EControlState::WaitingForStart;
                m_CodeArgs = { 0 };
                return PANSIControlCode::Pending;
            }
            return PANSIControlCode::None;
        case EControlState::WaitingForStart:
            if (character == '[')
            {
                m_ControlState = EControlState::WaitingForEnd;
                return PANSIControlCode::Pending;
            }
            m_ControlState = EControlState::None;
            return PANSIControlCode::None;
        case EControlState::WaitingForEnd:
            if (character >= 0x40 && character <= 0x7e)
            {
                m_ControlState = EControlState::None;
                return PANSIControlCode(character);
            }
            else if (character >= 0x30 && character <= 0x3F)
            {
                if (character == ';') {
                    m_CodeArgs.push_back(0);
                } else if (std::isdigit(character)) {
                    m_CodeArgs.back() = m_CodeArgs.back() * 10 + character - '0';
                }
                return PANSIControlCode::Pending;
            }
            else if (character >= 0x20 && character <= 0x2F)
            {
                // Intermediate.
                return PANSIControlCode::Pending;
            }
            m_ControlState = EControlState::None;
            return PANSIControlCode::None;

    }
    return PANSIControlCode::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PANSIEscapeCodeParser::FormatANSICode(PANSIControlCode code, std::vector<int> args)
{
    PString text = "\033[";
    bool first = true;
    for (int arg : args)
    {
        if (!first) {
            text += ";";
        }
        first = false;
        char numStr[16];
        text += itoa(arg, numStr, 10);
    }
    text += char(code);
    return text;
}
