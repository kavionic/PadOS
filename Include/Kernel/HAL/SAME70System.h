// This file is part of PadOS.
//
// Copyright (c) 2017-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 29.10.2017 19:51:50

#pragma once

#include "System/Platform.h"
#include "Utils/Utils.h"

class SAME70System
{
public:
    static SAME70System Instance;
    static void SetupClock(uint32_t frequencyCrystal, uint32_t frequencyCore, uint32_t frequencyPeripheral);
    
    static void EnablePeripheralClock(int perifID);
    static void DisablePeripheralClock(int perifID);
      
private:
    static uint32_t s_FrequencyCrystal;
};
