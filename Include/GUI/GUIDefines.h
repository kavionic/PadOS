// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 07.01.2020 17:18

#pragma once

#include <map>
#include <stdint.h>
#include <Utils/String.h>
#include <Utils/Logging.h>

namespace os
{
DEFINE_KERNEL_LOG_CATEGORY(LogCategoryGUITK);


///////////////////////////////////////////////////////////////////////////////
/// \brief Flags controlling a View
/// \ingroup gui
/// \sa os::View
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

namespace ViewFlags
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
    static constexpr uint32_t IgnoreMouse         = 0x0100;   ///< Make the view invisible to mouse/touch events.
    static constexpr uint32_t ForceHandleMouse    = 0x0200;   ///< Handle the mouse/touch event even if a child view is under the mouse.
    static constexpr uint32_t IsAttachedToScreen  = 0x0400;   ///< Set while the view is registered with the server.
    static constexpr int FirstUserBit = 16;    // Inheriting classes should shift their flags this much to the left to avoid collisions.

    extern const std::map<String, uint32_t> FlagMap;
}

namespace ViewDebugDrawFlags
{
    enum Type
    {
        ViewFrame    = 0x01,
        DrawRegion   = 0x02,
        DamageRegion = 0x04
    };
}

enum class ViewDockType : int32_t
{
    TopLevelView,
    RootLevelView,
    ChildView,
    PopupWindow,
    DockedWindow,
    FullscreenWindow,
    StatusBarIcon
};

namespace TransparentColors
{
static constexpr uint8_t    CMAP8 = 0xff;
static constexpr uint16_t   RGB16 = (0x1f << 11) | (0x01 << 5) | 0x1f; // (255, 4, 255)
static constexpr uint32_t   RGB32 = 0xffffffff;
};

enum class DrawingMode : uint8_t
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

enum class StandardColorID : int32_t
{
    None,
    DefaultBackground,
    Shine,
    Shadow,
    WindowBorderActive,
    WindowBorderInactive,
    ButtonBackground,
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


enum class Alignment : uint8_t
{
    Left,
    Right,
    Top,
    Bottom,
    Center,
    Stretch
};

enum class Orientation : uint8_t
{
    Horizontal,
    Vertical
};

enum class PrefSizeType : uint8_t
{
    Smallest,
    Greatest,
    Count,
    All = Count
};

enum class SizeOverride : uint8_t
{
    None,
    Always,
    Extend,
    Limit
};

enum class ScrollDirection : uint8_t
{
    Left,
    Right,
    Up,
    Down
};

enum class KeyCodes : uint32_t
{
    NONE = 0,
    CURSOR_LEFT = 1,
    CURSOR_RIGHT = 2,
    CURSOR_UP = 3,
    CURSOR_DOWN = 4,
    HOME = 5,
    END = 6,
    DELETE = 7,
    BACKSPACE = 8,
    TAB = 9,
    ENTER = 10,
    SHIFT = 11,
    CTRL = 12,
    ALT = 13,
    SYMBOLS = 14,
    ESCAPE  = 27,
    LAST_SPECIAL = SYMBOLS,
    SPACE = ' ',
    NUM_0 = '0',
    NUM_1 = '1',
    NUM_2 = '2',
    NUM_3 = '3',
    NUM_4 = '4',
    NUM_5 = '5',
    NUM_6 = '6',
    NUM_7 = '7',
    NUM_8 = '8',
    NUM_9 = '9',
    A = 'A',
    B = 'B',
    C = 'C',
    D = 'D',
    E = 'E',
    F = 'F',
    G = 'G',
    H = 'H',
    I = 'I',
    J = 'J',
    K = 'K',
    L = 'L',
    M = 'M',
    N = 'N',
    O = 'O',
    P = 'P',
    Q = 'Q',
    R = 'R',
    S = 'S',
    T = 'T',
    U = 'U',
    V = 'V',
    W = 'W',
    X = 'X',
    Y = 'Y',
    Z = 'Z',
    AE = 0xc6,
    OE = 0xd8,
    AA = 0xc5
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
    QUAL_LSHIFT = 0x01,                     //!< Left <SHIFT> key.
    QUAL_RSHIFT = 0x02,                     //!< Right <SHIFT> key.
    QUAL_SHIFT = QUAL_LSHIFT | QUAL_RSHIFT, //!< Any <SHIFT> key.

    QUAL_LCTRL = 0x04,                      //!< Left <CONTROL> key.
    QUAL_RCTRL = 0x08,                      //!< Right <CONTROL> key.
    QUAL_CTRL = QUAL_LCTRL | QUAL_RCTRL,    //!< Any <CONTROL> key.

    QUAL_LALT = 0x10,                       //!< Left <ALT> key.
    QUAL_RALT = 0x20,                       //!< Right <ALT> key.
    QUAL_ALT = QUAL_LALT | QUAL_RALT,       //!< Any <ALT> key.

    QUAL_REPEAT = 0x40                      //!< Set if the key-down event was caused by key repeating.
};

enum class EColorSpace : uint32_t
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
static constexpr float BEGIN_DRAG_OFFSET = 20.0f;

} // namespace
