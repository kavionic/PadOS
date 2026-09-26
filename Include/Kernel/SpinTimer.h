// This file is part of PadOS.
//
// Copyright (c) 2016-2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>
#include <System/Platform.h>
#include <System/Sections.h>


namespace kernel
{


class SpinTimer
{
public:
    static void Initialize();

    static void SleepuS(uint32_t delay);
    static void SleepMS(uint32_t delay);
private:
    static uint32_t   s_TicksPerMicroSec;
};

} // namespace
