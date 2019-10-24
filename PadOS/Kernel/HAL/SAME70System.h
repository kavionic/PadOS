// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 29.10.2017 19:51:50

#pragma once

#include "sam.h"
#include "System/Utils/Utils.h"

class SAME70System
{
public:
    static SAME70System Instance;
    static void SetupClock(uint32_t frequencyCrystal, uint32_t frequencyCore, uint32_t frequencyPeripheral);
    static void SetupFrequencies(uint32_t frequencyCrystal, uint32_t frequencyCore, uint32_t frequencyPeripheral);
 
    static void ResetWatchdog() { WDT->WDT_CR = WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk; }
    
    static void EnablePeripheralClock(int perifID);
    static void DisablePeripheralClock(int perifID);
    
    static uint32_t GetFrequencyCrystal();
    static uint32_t GetFrequencyCore();
    static uint32_t GetFrequencyPeripheral();
    
private:
    static uint32_t s_FrequencyCrystal;
    static uint32_t s_FrequencyCore;
    static uint32_t s_FrequencyPeripheral;
};
