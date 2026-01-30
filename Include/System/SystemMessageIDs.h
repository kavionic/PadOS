// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 17.03.2018 20:30:19
 
#pragma once


enum class PMessageID : uint32_t
{
    NONE = 0,
    FIRST_SIGNAL_ID = std::numeric_limits<int32_t>::max() - 200000,
    FIRST_SYSTEM_ID = std::numeric_limits<int32_t>::max() - 100000,
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE,
    KEY_DOWN,
    KEY_UP,
    QUIT
};
