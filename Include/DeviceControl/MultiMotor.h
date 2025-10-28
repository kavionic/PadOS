// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 19.10.2025 22:30

#pragma once

#include <sys/pados_types.h>
#include <DeviceControl/DeviceControlInvoker.h>
#include <Kernel/HAL/STM32/PeripheralMapping_STM32H7.h>

struct StepperIOSetup
{
    HWTimerID       TimerID;
    uint32_t        TimerClkFrequency;
    DigitalPinID    EnablePin;
    DigitalPinID    DirectionPin;
    PinMuxTarget    StepPin;
};

struct TMC2209IOSetup : StepperIOSetup
{
    DigitalPinID    DiagPinID;
    uint32_t        ChipAddress;
};

class PMultiMotor : public PDeviceControlInterface
{
public:
    PMultiMotor()
        : CreateMotor(*this)
        , DeleteMotor(*this)
        , SetJerk(*this)
        , SetReverse(*this)
        , GetReverse(*this)
        , SetStepsPerMillimeter(*this)
        , GetStepsPerMillimeter(*this)
        , SetWakeupOnFullStep(*this)
        , GetWakeupOnFullStep(*this)
        , SetSpeed(*this)
        , StopAtPos(*this)
        , SyncMove(*this)
        , QueueMotion(*this)
        , StepForward(*this)
        , StepBackward(*this)
        , EnableMotor(*this)
        , IsMotorEnabled(*this)
        , Wait(*this)
        , GetCurrentStopDistance(*this)
        , IsRunning(*this)
        , StartStopTimer(*this)
        , ClearMotion(*this)
        , GetStepPosition(*this)
        , GetPosition(*this)
        , ResetPosition(*this)
        , GetCurrentSpeed(*this)
        , GetCurrentDirection(*this)
        , SetCurrent(*this)
        , GetRunCurrent(*this)
        , SetRunCurrent(*this)
        , SetHoldCurrent(*this)
        , GetHoldCurrent(*this)
        , GetCurrentFadeTime(*this)
        , SetCurrentFadeTime(*this)
        , SetPowerDownTime(*this)
        , SetMicrosteps(*this)
        , GetStepTicks(*this)
        , GetStepTime(*this)
        , SetMaxStealthChopSpeed(*this)
        , SetMinStallGuardSpeed(*this)
        , SetStallGuardThreshold(*this)
        , GetStallGuardResult(*this)
        , SetHaltOnStall(*this)
        , ClearHaltedFlag(*this)
        , HasHalted(*this)
        , StartMultipleMotors(*this)
        , SyncStartMultipleMotors(*this)
        , WaitMultipleMotors(*this)
        , AddMotorToWaitGroup(*this)
    {}

    PDeviceControlInvoker< 0, handle_id      (handle_id motorID, const TMC2209IOSetup& setup)>                                   CreateMotor;
    PDeviceControlInvoker< 1, void           (handle_id motorID)>                                                                DeleteMotor;


    PDeviceControlInvoker< 2, void           (handle_id motorID, float jerk)>                                                    SetJerk;
    PDeviceControlInvoker< 3, void           (handle_id motorID, bool reverse)>                                                  SetReverse;
    PDeviceControlInvoker< 4, bool           (handle_id motorID)>                                                                GetReverse;
    PDeviceControlInvoker< 5, void           (handle_id motorID, float steps)>                                                   SetStepsPerMillimeter;
    PDeviceControlInvoker< 6, float          (handle_id motorID)>                                                                GetStepsPerMillimeter;
    PDeviceControlInvoker< 7, void           (handle_id motorID, bool value)>                                                    SetWakeupOnFullStep;
    PDeviceControlInvoker< 8, bool           (handle_id motorID)>                                                                GetWakeupOnFullStep;
    PDeviceControlInvoker< 9, void           (handle_id motorID, float speedMMS, float accelerationMMS)>                         SetSpeed;
    PDeviceControlInvoker<10, void           (handle_id motorID, float position, float speed, float acceleration)>               StopAtPos;
    PDeviceControlInvoker<11, void           (handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS)>       SyncMove;
    PDeviceControlInvoker<12, void           (handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS)>       QueueMotion;
    PDeviceControlInvoker<13, void           (handle_id motorID)>                                                                StepForward;
    PDeviceControlInvoker<14, void           (handle_id motorID)>                                                                StepBackward;
    PDeviceControlInvoker<15, void           (handle_id motorID, bool enable)>                                                   EnableMotor;
    PDeviceControlInvoker<16, bool           (handle_id motorID)>                                                                IsMotorEnabled;
    PDeviceControlInvoker<17, void           (handle_id motorID)>                                                                Wait;
    PDeviceControlInvoker<18, float          (handle_id motorID, float acceleration)>                                            GetCurrentStopDistance;
    PDeviceControlInvoker<19, bool           (handle_id motorID)>                                                                IsRunning;
    PDeviceControlInvoker<20, void           (handle_id motorID, bool doRun)>                                                    StartStopTimer;
    PDeviceControlInvoker<21, void           (handle_id motorID)>                                                                ClearMotion;
    PDeviceControlInvoker<22, int32_t        (handle_id motorID)>                                                                GetStepPosition;
    PDeviceControlInvoker<23, float          (handle_id motorID)>                                                                GetPosition;
    PDeviceControlInvoker<24, void           (handle_id motorID, float position)>                                                ResetPosition;
    PDeviceControlInvoker<25, float          (handle_id motorID)>                                                                GetCurrentSpeed;
    PDeviceControlInvoker<26, bool           (handle_id motorID)>                                                                GetCurrentDirection;
    PDeviceControlInvoker<27, void           (handle_id motorID, float holdCurrent, float runCurrent, TimeValMillis fadeTime)>   SetCurrent;
    PDeviceControlInvoker<28, float          (handle_id motorID)>                                                                GetRunCurrent;
    PDeviceControlInvoker<29, void           (handle_id motorID, float current)>                                                 SetRunCurrent;
    PDeviceControlInvoker<30, void           (handle_id motorID, float current)>                                                 SetHoldCurrent;
    PDeviceControlInvoker<31, float          (handle_id motorID)>                                                                GetHoldCurrent;
    PDeviceControlInvoker<32, TimeValMillis  (handle_id motorID)>                                                                GetCurrentFadeTime;
    PDeviceControlInvoker<33, void           (handle_id motorID, TimeValMillis fadeTime)>                                        SetCurrentFadeTime;
    PDeviceControlInvoker<34, void           (handle_id motorID, TimeValMillis time)>                                            SetPowerDownTime;
    PDeviceControlInvoker<35, void           (handle_id motorID, int32_t steps)>                                                 SetMicrosteps;
    PDeviceControlInvoker<36, uint32_t       (handle_id motorID)>                                                                GetStepTicks;
    PDeviceControlInvoker<37, float          (handle_id motorID)>                                                                GetStepTime;
    PDeviceControlInvoker<38, void           (handle_id motorID, float maxSpeed)>                                                SetMaxStealthChopSpeed;
    PDeviceControlInvoker<39, void           (handle_id motorID, float minSpeed)>                                                SetMinStallGuardSpeed;
    PDeviceControlInvoker<40, void           (handle_id motorID, float threshold)>                                               SetStallGuardThreshold;
    PDeviceControlInvoker<41, float          (handle_id motorID)>                                                                GetStallGuardResult;
    PDeviceControlInvoker<42, void           (handle_id motorID, bool doHalt)>                                                   SetHaltOnStall;
    PDeviceControlInvoker<43, void           (handle_id motorID)>                                                                ClearHaltedFlag;
    PDeviceControlInvoker<44, bool           (handle_id motorID)>                                                                HasHalted;
    PDeviceControlInvoker<45, void           (uint32_t motorIDMask)>                                                             StartMultipleMotors;
    PDeviceControlInvoker<46, void           (bool doWait, uint32_t motorIDMask)>                                                SyncStartMultipleMotors;
    PDeviceControlInvoker<47, void           (uint32_t motorIDMask)>                                                             WaitMultipleMotors;
    PDeviceControlInvoker<48, void           (handle_id motorID, handle_id waitGroupHandle)>                                     AddMotorToWaitGroup;

    template<typename TFirstMotorID>
    static uint32_t MakeMotorsMask(TFirstMotorID firstMotorID)
    {
        return 1 << firstMotorID;
    }

    template<typename TFirstMotorID, typename... TMotorIDs>
    static uint32_t MakeMotorsMask(TFirstMotorID firstMotorID, TMotorIDs... motorIDs)
    {
        return (1 << firstMotorID) | MakeMotorsMask(motorIDs...);
    }

    template<typename TFirstMotorID, typename... TMotorIDs>
    void StartMotors(TFirstMotorID firstMotorID, TMotorIDs... motorIDs)
    {
        StartMultipleMotors(MakeMotorsMask(firstMotorID, motorIDs...));
    }

    template<typename TFirstMotorID, typename... TMotorIDs>
    void SyncStartMotors(bool doWait, TFirstMotorID firstMotorID, TMotorIDs... motorIDs)
    {
        SyncStartMultipleMotors(doWait, MakeMotorsMask(doWait, firstMotorID, motorIDs...));
    }

    template<typename TFirstMotorID, typename... TMotorIDs>
    void WaitMotors(TFirstMotorID firstMotorID, TMotorIDs... motorIDs)
    {
        WaitMultipleMotors(MakeMotorsMask(firstMotorID, motorIDs...));
    }
};
