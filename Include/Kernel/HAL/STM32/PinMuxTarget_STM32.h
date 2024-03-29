// This file is part of PadOS.
//
// Copyright (C) 2023 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#if defined(STM32H7)
#include <Kernel/HAL/STM32/PinMuxTarget_STM32H7.h>
#elif defined(STM32G0)
#include <Kernel/HAL/STM32/PinMuxTarget_STM32G030xx.h>
#else
#error Unknown platform.
#endif

