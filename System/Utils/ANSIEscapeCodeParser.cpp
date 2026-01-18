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

PANSI_ControlCode PANSIEscapeCodeParser::ProcessCharacter(char character)
{
    if (character == 0x03) {
        return PANSI_ControlCode::Break;
    } else if (character == 0x04) {
        return PANSI_ControlCode::Disconnect;
    } else if (character == 0x08 || character == 0x7f) {
        return PANSI_ControlCode::Backspace;
    } else if (character == 0x09) {
        return PANSI_ControlCode::Tab;
    }

    switch (m_ControlState)
    {
        case EControlState::None:
            if (character == 0x1b)
            {
                m_ControlState = EControlState::WaitingForStart;
                m_CodeArgs = { 0 };
                return PANSI_ControlCode::Pending;
            }
            return PANSI_ControlCode::None;
        case EControlState::WaitingForStart:
            if (character == '[')
            {
                m_ControlState = EControlState::WaitingForEnd;
                return PANSI_ControlCode::Pending;
            }
            else if (character == 0x1b)
            {
                m_ControlState = EControlState::None;
                return PANSI_ControlCode::Escape;
            }
            m_ControlState = EControlState::None;
            return PANSI_ControlCode::None;
        case EControlState::WaitingForEnd:
            if (character >= 0x40 && character <= 0x7e)
            {
                m_ControlState = EControlState::None;
                return PANSI_ControlCode(character);
            }
            else if (character >= 0x30 && character <= 0x3F)
            {
                if (character == ';') {
                    m_CodeArgs.push_back(0);
                } else if (std::isdigit(character)) {
                    m_CodeArgs.back() = m_CodeArgs.back() * 10 + character - '0';
                }
                return PANSI_ControlCode::Pending;
            }
            else if (character >= 0x20 && character <= 0x2F)
            {
                // Intermediate.
                return PANSI_ControlCode::Pending;
            }
            m_ControlState = EControlState::None;
            return PANSI_ControlCode::None;

    }
    return PANSI_ControlCode::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PANSIEscapeCodeParser::FormatANSICode(PANSI_ControlCode code, std::vector<int> args)
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
