// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2022 18:00

#pragma once

namespace kernel
{

struct USBEndpointState
{
    bool Claim()
    {
        if (Busy || Claimed || Stalled) {
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
