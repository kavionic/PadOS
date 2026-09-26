// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.05.2022 16:30

#pragma once
#include <stdint.h>
#include <Kernel/HAL/DigitalPort.h>

enum class HWTimerID : int32_t;

namespace kernel
{
enum class IRQResult : int;


class PiezoBuzzer_STM32
{
public:
    bool Setup(HWTimerID timerID, PinMuxTarget beeperPin);

    void Beep(float time);

private:
    static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IRQResult HandleIRQ();

    MCU_Timer16_t* m_Timer = nullptr;
    DigitalPin      m_BeeperPin;
    PinMuxTarget    m_BeeperPinMux;
    uint32_t        m_TimerFrequency = 0;
    uint32_t        m_BeepFrequency = 2000;
    int32_t         m_RemainingCycles = 0;
    bool            m_IsInitialized = false;
};

} //namespace kernel
