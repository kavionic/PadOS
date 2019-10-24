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
// Created: 18/08/25 0:24:37

#pragma once
#include "System/Threads/Thread.h"
#include "HAL/DigitalPort.h"
//#include "SystemSetup.h"

enum class sys_power_state
{
    uninitialized,
    shutdown,
    sleeping,
    running
};

class KPowerManager : public os::Thread
{
public:
    KPowerManager();
    ~KPowerManager();

    static KPowerManager& GetInstance();

    static void SetExternalReset(bool reset);

    void Initialize(TcChannel* timerChannel, const DigitalPin& pinPowerSwitch);
    static void Shutdown();
    void SetState(sys_power_state newState);

    virtual int Run() override;

private:
    static void IRQCallback(IRQn_Type irq, void* userData) { static_cast<KPowerManager*>(userData)->HandleIRQ(); }
    static void TimerIRQCallback(IRQn_Type irq, void* userData) { static_cast<KPowerManager*>(userData)->HandleTimerIRQ(); }
    void        HandleIRQ();
    void        HandleTimerIRQ();

    TcChannel*      m_TimerChannel = nullptr;    
    DigitalPin      m_PinPowerSwitch;
    sys_power_state m_State = sys_power_state::uninitialized;
    bool            m_PowerButtonState = false;
    int             m_PowerButtonPressedCycles = 0;
    KPowerManager(const KPowerManager&) = delete;
    KPowerManager& operator=(const KPowerManager&) = delete;
};
