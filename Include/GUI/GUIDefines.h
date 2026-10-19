// This file is part of PadOS.
//
// Copyright (c) 2020-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 07.01.2020 17:18

#pragma once

#include <map>
#include <stdint.h>
#include <utility>
#include <Utils/String.h>
#include <Utils/Logging.h>

PDEFINE_LOG_CATEGORY(LogCategoryGUITK, "GUITK", PLogSeverity::WARNING);


///////////////////////////////////////////////////////////////////////////////
/// \brief Flags controlling a View
/// \ingroup gui
/// \sa os::View
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

namespace PViewFlags
{
    static constexpr uint32_t FullUpdateOnResizeH = 0x0001;   ///< Cause the entire view to be invalidated if made wider
    static constexpr uint32_t FullUpdateOnResizeV = 0x0002;   ///< Cause the entire view to be invalidated if made higher
    static constexpr uint32_t FullUpdateOnResize  = 0x0003;   ///< Cause the entire view to be invalidated if resized
    static constexpr uint32_t IgnoreWhenHidden    = 0x0004;   ///< Ignore size when hidden.
    static constexpr uint32_t WillDraw            = 0x0008;   ///< Tell the appserver that you want to render stuff to it
    static constexpr uint32_t Transparent         = 0x0010;   ///< Allow the parent view to render in areas covered by this view
    static constexpr uint32_t ClearBackground     = 0x0020;   ///< Automatically clear new areas when windows are moved/resized
    static constexpr uint32_t DrawOnChildren      = 0x0040;   ///< Setting this flag allows the view to render atop of all its children.
    static constexpr uint32_t Eavesdropper        = 0x0080;   ///< Client-side view that is connected to a foreign server-side view.
    static constexpr uint32_t IsAttachedToScreen  = 0x0100;   ///< Set while the view is registered with the server.
    static constexpr int FirstUserBit = 16;    // Inheriting classes should shift their flags this much to the left to avoid collisions.

    extern const std::map<PString, uint32_t> FlagMap;
}

enum class PViewHitMode : uint8_t
{
    HitTest,
    IgnoreThis,
    IgnoreRecursive
};

namespace PViewDebugDrawFlags
{
    enum Type
    {
        ViewFrame    = 0x01,
        DrawRegion   = 0x02,
        DamageRegion = 0x04,
        FullRegion   = 0x08
    };
}

enum class PViewDockType : int32_t
{
    TopLevelView,
    RootLevelView,
    ChildView,
    PopupWindow,
    DockedWindow,
    FullscreenWindow,
    StatusBarIcon
};

namespace PTransparentColors
{
static constexpr uint8_t    CMAP8 = 0xff;
static constexpr uint16_t   RGB16 = (0x1f << 11) | (0x01 << 5) | 0x1f; // (255, 4, 255)
static constexpr uint32_t   RGB32 = 0xffffffff;
};

enum class PDrawingMode : uint8_t
{
    Copy,
    Overlay,
    Invert,
    Erase,
    Blend,
    Add,
    Subtract,
    Min,
    Max,
    Select
};

enum class PTriangleMode : uint8_t
{
    Fan,
    Strip
};

enum class PCapStyle : uint8_t
{
    Square,
    Flat,
    Round
};

enum class PJointStyle : uint8_t
{
    Bevel,
    Miter,
    Round
};

enum class PStandardColorID : int32_t
{
    None,
    DefaultBackground,
    Shine,
    Shadow,
    WindowBorderActive,
    WindowBorderInactive,
    ButtonBackground,
    ButtonBackgroundHover,
    ButtonLabelNormal,
    ButtonLabelDisabled,
    MenuText,
    MenuTextSelected,
    MenuBackground,
    MenuBackgroundSelected,
    ScrollBarBackground,
    ScrollBarKnob,
    SliderKnobNormal,
    SliderKnobPressed,
    SliderKnobShadow,
    SliderKnobDisabled,
    SliderTrackNormal,
    SliderTrackDisabled,
    ListViewTab,
    ListViewTabText,
    Count
};


enum
{
    FRAME_RECESSED = 0x000008,
    FRAME_RAISED = 0x000010,
    FRAME_THIN = 0x000020,
    FRAME_WHIDE = 0x000040,
    FRAME_ETCHED = 0x000080,
    FRAME_FLAT = 0x000100,
    FRAME_DISABLED = 0x000200,
    FRAME_TRANSPARENT = 0x010000
};

static constexpr float LAYOUT_MAX_SIZE = 100000.0f;
static constexpr float COORD_MAX = 16000000.0f;


enum class PAlignment : uint8_t
{
    Left,
    Right,
    Top,
    Bottom,
    Center,
    Stretch
};

enum class POrientation : uint8_t
{
    Horizontal,
    Vertical
};

enum class PPrefSizeType : uint8_t
{
    Smallest,
    Greatest,
    Count,
    All = Count
};

enum class PSizeOverride : uint8_t
{
    None,
    Always,
    Extend,
    Limit
};

enum class PScrollDirection : uint8_t
{
    Left,
    Right,
    Up,
    Down
};

// Physical key identities, independent of the selected keyboard layout.
// Letter and punctuation names use the US layout as a reference for key positions.
// Values match USB HID Keyboard/Keypad codes; other drivers translate their native codes into this numbering.
// Text and logical key symbols are carried separately in keyboard events.
enum class PKeyCodes : uint16_t
{
    NONE = 0x00,

    A = 0x04,
    B = 0x05,
    C = 0x06,
    D = 0x07,
    E = 0x08,
    F = 0x09,
    G = 0x0a,
    H = 0x0b,
    I = 0x0c,
    J = 0x0d,
    K = 0x0e,
    L = 0x0f,
    M = 0x10,
    N = 0x11,
    O = 0x12,
    P = 0x13,
    Q = 0x14,
    R = 0x15,
    S = 0x16,
    T = 0x17,
    U = 0x18,
    V = 0x19,
    W = 0x1a,
    X = 0x1b,
    Y = 0x1c,
    Z = 0x1d,

    NUM_1 = 0x1e,
    NUM_2 = 0x1f,
    NUM_3 = 0x20,
    NUM_4 = 0x21,
    NUM_5 = 0x22,
    NUM_6 = 0x23,
    NUM_7 = 0x24,
    NUM_8 = 0x25,
    NUM_9 = 0x26,
    NUM_0 = 0x27,

    ENTER = 0x28,
    ESCAPE = 0x29,
    BACKSPACE = 0x2a,
    TAB = 0x2b,
    SPACE = 0x2c,
    MINUS = 0x2d,
    EQUAL = 0x2e,
    LEFT_BRACKET = 0x2f,
    RIGHT_BRACKET = 0x30,
    BACKSLASH = 0x31,
    NON_US_HASH = 0x32,
    SEMICOLON = 0x33,
    APOSTROPHE = 0x34,
    GRAVE = 0x35,
    COMMA = 0x36,
    PERIOD = 0x37,
    SLASH = 0x38,
    CAPS_LOCK = 0x39,

    F1 = 0x3a,
    F2 = 0x3b,
    F3 = 0x3c,
    F4 = 0x3d,
    F5 = 0x3e,
    F6 = 0x3f,
    F7 = 0x40,
    F8 = 0x41,
    F9 = 0x42,
    F10 = 0x43,
    F11 = 0x44,
    F12 = 0x45,

    PRINT_SCREEN = 0x46,
    SCROLL_LOCK = 0x47,
    PAUSE = 0x48,
    INSERT = 0x49,
    HOME = 0x4a,
    PAGE_UP = 0x4b,
    DELETE = 0x4c,
    END = 0x4d,
    PAGE_DOWN = 0x4e,
    RIGHT_ARROW = 0x4f,
    LEFT_ARROW = 0x50,
    DOWN_ARROW = 0x51,
    UP_ARROW = 0x52,
    NUM_LOCK = 0x53,

    KEYPAD_DIVIDE = 0x54,
    KEYPAD_MULTIPLY = 0x55,
    KEYPAD_SUBTRACT = 0x56,
    KEYPAD_ADD = 0x57,
    KEYPAD_ENTER = 0x58,

    KEYPAD_1 = 0x59,
    KEYPAD_2 = 0x5a,
    KEYPAD_3 = 0x5b,
    KEYPAD_4 = 0x5c,
    KEYPAD_5 = 0x5d,
    KEYPAD_6 = 0x5e,
    KEYPAD_7 = 0x5f,
    KEYPAD_8 = 0x60,
    KEYPAD_9 = 0x61,
    KEYPAD_0 = 0x62,
    KEYPAD_PERIOD = 0x63,

    NON_US_BACKSLASH = 0x64,
    APPLICATION = 0x65,
    POWER = 0x66,
    KEYPAD_EQUAL = 0x67,

    F13 = 0x68,
    F14 = 0x69,
    F15 = 0x6a,
    F16 = 0x6b,
    F17 = 0x6c,
    F18 = 0x6d,
    F19 = 0x6e,
    F20 = 0x6f,
    F21 = 0x70,
    F22 = 0x71,
    F23 = 0x72,
    F24 = 0x73,

    EXECUTE = 0x74,
    HELP = 0x75,
    MENU = 0x76,
    SELECT = 0x77,
    STOP = 0x78,
    AGAIN = 0x79,
    UNDO = 0x7a,
    CUT = 0x7b,
    COPY = 0x7c,
    PASTE = 0x7d,
    FIND = 0x7e,
    MUTE = 0x7f,
    VOLUME_UP = 0x80,
    VOLUME_DOWN = 0x81,
    LOCKING_CAPS_LOCK = 0x82,
    LOCKING_NUM_LOCK = 0x83,
    LOCKING_SCROLL_LOCK = 0x84,
    KEYPAD_COMMA = 0x85,
    KEYPAD_EQUAL_AS400 = 0x86,

    INTERNATIONAL_1 = 0x87,
    INTERNATIONAL_2 = 0x88,
    INTERNATIONAL_3 = 0x89,
    INTERNATIONAL_4 = 0x8a,
    INTERNATIONAL_5 = 0x8b,
    INTERNATIONAL_6 = 0x8c,
    INTERNATIONAL_7 = 0x8d,
    INTERNATIONAL_8 = 0x8e,
    INTERNATIONAL_9 = 0x8f,

    LANGUAGE_1 = 0x90,
    LANGUAGE_2 = 0x91,
    LANGUAGE_3 = 0x92,
    LANGUAGE_4 = 0x93,
    LANGUAGE_5 = 0x94,
    LANGUAGE_6 = 0x95,
    LANGUAGE_7 = 0x96,
    LANGUAGE_8 = 0x97,
    LANGUAGE_9 = 0x98,

    ALTERNATE_ERASE = 0x99,
    SYSTEM_REQUEST = 0x9a,
    CANCEL = 0x9b,
    CLEAR = 0x9c,
    PRIOR = 0x9d,
    RETURN = 0x9e,
    SEPARATOR = 0x9f,
    OUT = 0xa0,
    OPER = 0xa1,
    CLEAR_AGAIN = 0xa2,
    CURSOR_SELECT = 0xa3,
    EXTEND_SELECT = 0xa4,

    KEYPAD_00 = 0xb0,
    KEYPAD_000 = 0xb1,
    THOUSANDS_SEPARATOR = 0xb2,
    DECIMAL_SEPARATOR = 0xb3,
    CURRENCY_UNIT = 0xb4,
    CURRENCY_SUBUNIT = 0xb5,
    KEYPAD_LEFT_PARENTHESIS = 0xb6,
    KEYPAD_RIGHT_PARENTHESIS = 0xb7,
    KEYPAD_LEFT_BRACE = 0xb8,
    KEYPAD_RIGHT_BRACE = 0xb9,
    KEYPAD_TAB = 0xba,
    KEYPAD_BACKSPACE = 0xbb,
    KEYPAD_A = 0xbc,
    KEYPAD_B = 0xbd,
    KEYPAD_C = 0xbe,
    KEYPAD_D = 0xbf,
    KEYPAD_E = 0xc0,
    KEYPAD_F = 0xc1,
    KEYPAD_XOR = 0xc2,
    KEYPAD_CARET = 0xc3,
    KEYPAD_PERCENT = 0xc4,
    KEYPAD_LESS_THAN = 0xc5,
    KEYPAD_GREATER_THAN = 0xc6,
    KEYPAD_AMPERSAND = 0xc7,
    KEYPAD_DOUBLE_AMPERSAND = 0xc8,
    KEYPAD_VERTICAL_BAR = 0xc9,
    KEYPAD_DOUBLE_VERTICAL_BAR = 0xca,
    KEYPAD_COLON = 0xcb,
    KEYPAD_HASH = 0xcc,
    KEYPAD_SPACE = 0xcd,
    KEYPAD_AT = 0xce,
    KEYPAD_EXCLAMATION = 0xcf,
    KEYPAD_MEMORY_STORE = 0xd0,
    KEYPAD_MEMORY_RECALL = 0xd1,
    KEYPAD_MEMORY_CLEAR = 0xd2,
    KEYPAD_MEMORY_ADD = 0xd3,
    KEYPAD_MEMORY_SUBTRACT = 0xd4,
    KEYPAD_MEMORY_MULTIPLY = 0xd5,
    KEYPAD_MEMORY_DIVIDE = 0xd6,
    KEYPAD_PLUS_MINUS = 0xd7,
    KEYPAD_CLEAR = 0xd8,
    KEYPAD_CLEAR_ENTRY = 0xd9,
    KEYPAD_BINARY = 0xda,
    KEYPAD_OCTAL = 0xdb,
    KEYPAD_DECIMAL = 0xdc,
    KEYPAD_HEXADECIMAL = 0xdd,

    LEFT_CONTROL = 0xe0,
    LEFT_SHIFT = 0xe1,
    LEFT_ALT = 0xe2,
    LEFT_GUI = 0xe3,
    RIGHT_CONTROL = 0xe4,
    RIGHT_SHIFT = 0xe5,
    RIGHT_ALT = 0xe6,
    RIGHT_GUI = 0xe7
};

// The exclusive index limit includes reserved slots; error/status codes are not physical keys.
static constexpr size_t KEY_CODE_LIMIT = 256;
static constexpr size_t KEY_FIRST_MODIFIER_INDEX = std::to_underlying(PKeyCodes::LEFT_CONTROL);
static constexpr size_t KEY_LAST_MODIFIER_INDEX = std::to_underlying(PKeyCodes::RIGHT_GUI);


// Layout-dependent key meanings owned by PadOS. Character symbols use Unicode scalar values,
// for example PKeySymbol(U'a'). Special symbols occupy a separate range above Unicode.
// Keypad and main-keyboard actions share a symbol; PKeyCodes preserves their physical identity.
// Modifier symbols preserve their logical left/right identity after remapping.
// Symbols describe individual keys. Composed UTF-8 text is delivered separately.
enum class PKeySymbol : uint32_t
{
    NONE = 0,
    FIRST_SPECIAL = 0x110000,

    BACKSPACE = FIRST_SPECIAL,
    TAB,
    ENTER,
    ESCAPE,
    INSERT,
    DELETE,
    HOME,
    END,
    LEFT_ARROW,
    RIGHT_ARROW,
    UP_ARROW,
    DOWN_ARROW,
    PAGE_UP,
    PAGE_DOWN,
    BEGIN,
    CLEAR,
    SELECT,
    EXECUTE,
    CANCEL,
    HELP,
    UNDO,
    REDO,
    FIND,
    CUT,
    COPY,
    PASTE,
    MENU,
    PRINT_SCREEN,
    SYSTEM_REQUEST,
    PAUSE,
    BREAK,
    POWER,
    MUTE,
    VOLUME_UP,
    VOLUME_DOWN,
    COMPOSE,
    LEFT_SHIFT,
    RIGHT_SHIFT,
    LEFT_CONTROL,
    RIGHT_CONTROL,
    LEFT_ALT,
    RIGHT_ALT,
    ALT_GR,
    LEFT_SUPER,
    RIGHT_SUPER,
    LEFT_META,
    RIGHT_META,
    LEFT_HYPER,
    RIGHT_HYPER,
    LEVEL5_SHIFT,
    CAPS_LOCK,
    SHIFT_LOCK,
    NUM_LOCK,
    SCROLL_LOCK,

    F1 = 0x110100,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,
    F26,
    F27,
    F28,
    F29,
    F30,
    F31,
    F32,
    F33,
    F34,
    F35
};


/// \anchor os_gui_qualifiers
/// \par Description:
///     Bit-masks for the various qualifier keys. This defines can be used
///     to check whether a given qualifier (or group of qualifiers) are
///     pressed or not. The current qualifier mask is passed into various
///     hook members dealing with keyboard input in the View class and it
///     can also be retrieved asyncronously with the View::GetQualifiers()
///     member.
///////////////////////////////////////////////////////////////////////////////

enum
{
    QUAL_LSHIFT = 0x01,                     //!< Left \<SHIFT\> key.
    QUAL_RSHIFT = 0x02,                     //!< Right \<SHIFT\> key.
    QUAL_SHIFT = QUAL_LSHIFT | QUAL_RSHIFT, //!< Any \<SHIFT\> key.

    QUAL_LCTRL = 0x04,                      //!< Left \<CONTROL\> key.
    QUAL_RCTRL = 0x08,                      //!< Right \<CONTROL\> key.
    QUAL_CTRL = QUAL_LCTRL | QUAL_RCTRL,    //!< Any \<CONTROL\> key.

    QUAL_LALT = 0x10,                       //!< Left \<ALT\> key.
    QUAL_RALT = 0x20,                       //!< Right \<ALT\> key.
    QUAL_ALT = QUAL_LALT | QUAL_RALT,       //!< Any \<ALT\> key.

    QUAL_REPEAT = 0x40,                     //!< Set if the key-down event was caused by key repeating.
    QUAL_SUPER = 0x80,
    QUAL_CAPS_LOCK = 0x100,
    QUAL_NUM_LOCK = 0x200,
    QUAL_ALT_GR = 0x400
};

enum class PEColorSpace : uint32_t
{
    NO_COLOR_SPACE,

    RGB32,
    RGBA32,
    RGB24,
    RGB16,
    RGB15,
    RGBA15,
    CMAP8,
    GRAY8,
    MONO1,
    YUV422,
    YUV411,
    YUV420,
    YUV444,
    YUV9,
    YUV12
};

static constexpr float KEYREPEAT_DELAY  = 0.3f;
static constexpr float KEYREPEAT_REPEAT = 0.08f;

static constexpr float LONG_PRESS_DELAY = 0.5f;
static constexpr float TAP_MAX_DURATION = 0.25f;
static constexpr float BEGIN_DRAG_OFFSET = 20.0f;
static constexpr float POINTER_CAPTURE_LOCK_THRESHOLD = 8.0f;
