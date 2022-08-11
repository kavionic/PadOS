// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.06.2022 18:00

#pragma once

namespace kernel
{

struct USBEndpointState
{
    bool Claim()
    {
        if (Busy || Claimed) {
            return false;
        }
        Claimed = true;
        return true;
    }
    bool Release()
    {
        if (Busy || !Claimed) {
            return false;
        }
        Claimed = false;
        return true;
    }
    void Reset()
    {
        Busy    = false;
        Stalled = false;
        Claimed = false;
    }
    bool    Busy = false;
    bool    Stalled = false;
    bool    Claimed = false;
};

} // namespace kernel
