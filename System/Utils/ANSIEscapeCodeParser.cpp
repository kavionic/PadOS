// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 13.01.2026 22:00

#include <cctype>

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

void PANSIEscapeCodeParser::Reset()
{
    m_ControlState = EControlState::None;
    m_CodeArgs.clear();
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
