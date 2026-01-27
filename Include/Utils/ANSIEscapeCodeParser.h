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

enum class PANSI_ControlCode
{
    None,
    Pending,
    Disconnect,
    Break,
    Escape,
    Backspace,
    Tab,
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
    EraseDisplay = 'J',
    SetRenderProperty = 'm'
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

enum class PANSI_RenderProperty : int
{
    // --- Text attributes ---
    Reset                       = 0,
    Bold                        = 1,
    Faint                       = 2,
    Italic                      = 3,
    Underline                   = 4,
    SlowBlink                   = 5,
    RapidBlink                  = 6,
    Reverse                     = 7,
    Conceal                     = 8,
    CrossedOut                  = 9,

    PrimaryFont                 = 10,
    AltFont1                    = 11,
    AltFont2                    = 12,
    AltFont3                    = 13,
    AltFont4                    = 14,
    AltFont5                    = 15,
    AltFont6                    = 16,
    AltFont7                    = 17,
    AltFont8                    = 18,
    AltFont9                    = 19,

    Fraktur                     = 20,
    DoubleUnderline             = 21,
    NormalIntensity             = 22,
    NotItalic                   = 23,
    NotUnderlined               = 24,
    NotBlinking                 = 25,
    ProportionalSpacing         = 26,
    NotReversed                 = 27,
    Reveal                      = 28,
    NotCrossedOut               = 29,

    // --- Foreground colors ---
    FgColor_Black               = 30,
    FgColor_Red                 = 31,
    FgColor_Green               = 32,
    FgColor_Yellow              = 33,
    FgColor_Blue                = 34,
    FgColor_Magenta             = 35,
    FgColor_Cyan                = 36,
    FgColor_White               = 37,

    FgColor_Extended            = 38, // 5;n or 2;r;g;b
    FgColor_Default             = 39,

    // --- Background colors ---
    BgColor_Black               = 40,
    BgColor_Red                 = 41,
    BgColor_Green               = 42,
    BgColor_Yellow              = 43,
    BgColor_Blue                = 44,
    BgColor_Magenta             = 45,
    BgColor_Cyan                = 46,
    BgColor_White               = 47,

    BgColor_Extended            = 48, // 5;n or 2;r;g;b
    BgColor_Default             = 49,

    // --- Additional attributes ---
    DisableProportionalSpacing  = 50,
    Framed                      = 51,
    Encircled                   = 52,
    Overlined                   = 53,
    NotFramedOrEncircled        = 54,
    NotOverlined                = 55,

    UnderlineColor_Extended     = 58, // 5;n or 2;r;g;b
    UnderlineColor_Default      = 59,

    IdeogramUnderline           = 60,
    IdeogramDoubleUnderline     = 61,
    IdeogramOverline            = 62,
    IdeogramDoubleOverline      = 63,
    IdeogramStressMarking       = 64,
    NoIdeogramAttributes        = 65,

    Superscript                 = 73,
    Subscript                   = 74,
    NotSuperOrSubscript         = 75,

    // --- Bright foreground colors ---
    FgColor_BrightBlack         = 90,
    FgColor_BrightRed           = 91,
    FgColor_BrightGreen         = 92,
    FgColor_BrightYellow        = 93,
    FgColor_BrightBlue          = 94,
    FgColor_BrightMagenta       = 95,
    FgColor_BrightCyan          = 96,
    FgColor_BrightWhite         = 97,

    // --- Bright background colors ---
    BgColor_BrightBlack         = 100,
    BgColor_BrightRed           = 101,
    BgColor_BrightGreen         = 102,
    BgColor_BrightYellow        = 103,
    BgColor_BrightBlue          = 104,
    BgColor_BrightMagenta       = 105,
    BgColor_BrightCyan          = 106,
    BgColor_BrightWhite         = 107
};

static constexpr int XTerm_XTWINOPS_ReportTextAreaSize = 18;

class PANSIEscapeCodeParser
{
public:
    PANSI_ControlCode ProcessCharacter(char character);
    const std::vector<int>& GetArguments() const { return m_CodeArgs; }

    static PString FormatANSICode(PANSI_ControlCode code, std::vector<int> args);

    template<typename... TArgTypes>
    static PString FormatANSICode(PANSI_ControlCode code, TArgTypes ...args)
    {
        return FormatANSICode(code, { args... });
    }

private:
    enum class EControlState { None, WaitingForStart, WaitingForEnd };

    EControlState    m_ControlState = EControlState::None;
    std::vector<int> m_CodeArgs;

};
