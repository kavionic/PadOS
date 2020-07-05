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
// Created: 07.01.2020 17:18

#pragma once

#include <stdint.h>

namespace os
{


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
    static constexpr uint32_t WillDraw            = 0x0004;   ///< Tell the appserver that you want to render stuff to it
    static constexpr uint32_t Transparent         = 0x0008;   ///< Allow the parent view to render in areas covered by this view
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
    RootLevelView,
    ChildView,
    PopupWindow,
    DockedWindow,
    FullscreenWindow,
    StatusBarIcon
};

enum drawing_mode
{
    DM_COPY,
    DM_OVER,
    DM_INVERT,
    DM_ERASE,
    DM_BLEND,
    DM_ADD,
    DM_SUBTRACT,
    DM_MIN,
    DM_MAX,
    DM_SELECT
};

enum class StandardColorID : int32_t
{
    NORMAL,
    SHINE,
    SHADOW,
    SELECTED_WND_BORDER,
    NORMAL_WND_BORDER,
    MENU_TEXT,
    SELECTED_MENU_TEXT,
    MENU_BACKGROUND,
    SELECTED_MENU_BACKGROUND,
    SCROLLBAR_BG,
    SCROLLBAR_KNOB,
    LISTVIEW_TAB,
    LISTVIEW_TAB_TEXT,
    COUNT
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


enum class Alignment : uint8_t
{
	Left,
	Right,
	Top,
	Bottom,
	Center
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

} // namespace