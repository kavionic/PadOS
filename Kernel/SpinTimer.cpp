// This file is part of PadOS.
//
// Copyright (C) 2016-2018 Kurt Skauen <http://kavionic.com/>
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

#include "Kernel/SpinTimer.h"
#include "Kernel/Kernel.h"
#include "System/SysTime.h"

using namespace kernel;

uint32_t   SpinTimer::s_TicksPerMicroSec;

void SpinTimer::Initialize()
{
    s_TicksPerMicroSec = Kernel::GetFrequencyCore() / TimeValMicros::TicksPerSecond;
}
