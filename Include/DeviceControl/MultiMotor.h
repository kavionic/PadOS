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
#include <Kernel/VFS/KDriverParametersBase.h>
#include <Kernel/HAL/STM32/PeripheralMapping_STM32H7.h>

struct StepperIOSetup
{
    HWTimerID       TimerID;
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
        , EnableMotorsPower(*this)
        , IsMotorsPowerEnabled(*this)
        , SetJerk(*this)
        , SetReverse(*this)
        , GetReverse(*this)
        , SetStepsPerMillimeter(*this)
        , GetStepsPerMillimeter(*this)
        , SetWakeupOnFullStep(*this)
        , GetWakeupOnFullStep(*this)
        , SetSpeed(*this)
        , SetSpeedMultiMask(*this)
        , StopAtOffset(*this)
        , StopAtOffsetMultiMask(*this)
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

    PDeviceControlInvoker< 0, handle_id     (handle_id motorID, const TMC2209IOSetup& setup)>                                   CreateMotor;
    PDeviceControlInvoker< 1, void          (handle_id motorID)>                                                                DeleteMotor;
    PDeviceControlInvoker< 2, void          (bool enable)>                                                                      EnableMotorsPower;
    PDeviceControlInvoker< 3, bool          () const>                                                                           IsMotorsPowerEnabled;
    PDeviceControlInvoker< 4, void          (handle_id motorID, float jerk)>                                                    SetJerk;
    PDeviceControlInvoker< 5, void          (handle_id motorID, bool reverse)>                                                  SetReverse;
    PDeviceControlInvoker< 6, bool          (handle_id motorID)>                                                                GetReverse;
    PDeviceControlInvoker< 7, void          (handle_id motorID, float steps)>                                                   SetStepsPerMillimeter;
    PDeviceControlInvoker< 8, float         (handle_id motorID)>                                                                GetStepsPerMillimeter;
    PDeviceControlInvoker< 9, void          (handle_id motorID, bool value)>                                                    SetWakeupOnFullStep;
    PDeviceControlInvoker<10, bool          (handle_id motorID)>                                                                GetWakeupOnFullStep;
    PDeviceControlInvoker<11, void          (handle_id motorID, float speedMMS, float accelerationMMS)>                         SetSpeed;
    PDeviceControlInvoker<12, void          (uint32_t motorIDMask, float speedMMS, float accelerationMMS)>                      SetSpeedMultiMask;
    PDeviceControlInvoker<13, void          (handle_id motorID, float position, float speed, float acceleration)>               StopAtOffset;
    PDeviceControlInvoker<14, void          (uint32_t motorIDMask, float position, float speed, float acceleration)>            StopAtOffsetMultiMask;
    PDeviceControlInvoker<15, void          (handle_id motorID, float position, float speed, float acceleration)>               StopAtPos;
    PDeviceControlInvoker<16, void          (handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS)>       SyncMove;
    PDeviceControlInvoker<17, void          (handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS)>       QueueMotion;
    PDeviceControlInvoker<18, void          (handle_id motorID)>                                                                StepForward;
    PDeviceControlInvoker<19, void          (handle_id motorID)>                                                                StepBackward;
    PDeviceControlInvoker<20, void          (handle_id motorID, bool enable)>                                                   EnableMotor;
    PDeviceControlInvoker<21, bool          (handle_id motorID)>                                                                IsMotorEnabled;
    PDeviceControlInvoker<22, void          (handle_id motorID)>                                                                Wait;
    PDeviceControlInvoker<23, float         (handle_id motorID, float acceleration)>                                            GetCurrentStopDistance;
    PDeviceControlInvoker<24, bool          (handle_id motorID)>                                                                IsRunning;
    PDeviceControlInvoker<25, void          (handle_id motorID, bool doRun)>                                                    StartStopTimer;
    PDeviceControlInvoker<26, void          (handle_id motorID)>                                                                ClearMotion;
    PDeviceControlInvoker<27, int32_t       (handle_id motorID)>                                                                GetStepPosition;
    PDeviceControlInvoker<28, float         (handle_id motorID)>                                                                GetPosition;
    PDeviceControlInvoker<29, void          (handle_id motorID, float position)>                                                ResetPosition;
    PDeviceControlInvoker<30, float         (handle_id motorID)>                                                                GetCurrentSpeed;
    PDeviceControlInvoker<31, bool          (handle_id motorID)>                                                                GetCurrentDirection;
    PDeviceControlInvoker<32, void          (handle_id motorID, float holdCurrent, float runCurrent, TimeValMillis fadeTime)>   SetCurrent;
    PDeviceControlInvoker<33, float         (handle_id motorID)>                                                                GetRunCurrent;
    PDeviceControlInvoker<34, void          (handle_id motorID, float current)>                                                 SetRunCurrent;
    PDeviceControlInvoker<35, void          (handle_id motorID, float current)>                                                 SetHoldCurrent;
    PDeviceControlInvoker<36, float         (handle_id motorID)>                                                                GetHoldCurrent;
    PDeviceControlInvoker<37, TimeValMillis (handle_id motorID)>                                                                GetCurrentFadeTime;
    PDeviceControlInvoker<38, void          (handle_id motorID, TimeValMillis fadeTime)>                                        SetCurrentFadeTime;
    PDeviceControlInvoker<39, void          (handle_id motorID, TimeValMillis time)>                                            SetPowerDownTime;
    PDeviceControlInvoker<40, void          (handle_id motorID, int32_t steps)>                                                 SetMicrosteps;
    PDeviceControlInvoker<41, uint32_t      (handle_id motorID)>                                                                GetStepTicks;
    PDeviceControlInvoker<42, float         (handle_id motorID)>                                                                GetStepTime;
    PDeviceControlInvoker<43, void          (handle_id motorID, float maxSpeed)>                                                SetMaxStealthChopSpeed;
    PDeviceControlInvoker<44, void          (handle_id motorID, float minSpeed)>                                                SetMinStallGuardSpeed;
    PDeviceControlInvoker<45, void          (handle_id motorID, float threshold)>                                               SetStallGuardThreshold;
    PDeviceControlInvoker<46, float         (handle_id motorID)>                                                                GetStallGuardResult;
    PDeviceControlInvoker<47, void          (handle_id motorID, bool doHalt)>                                                   SetHaltOnStall;
    PDeviceControlInvoker<48, void          (handle_id motorID)>                                                                ClearHaltedFlag;
    PDeviceControlInvoker<49, bool          (handle_id motorID)>                                                                HasHalted;
    PDeviceControlInvoker<50, void          (uint32_t motorIDMask)>                                                             StartMultipleMotors;
    PDeviceControlInvoker<51, void          (bool doWait, uint32_t motorIDMask)>                                                SyncStartMultipleMotors;
    PDeviceControlInvoker<52, void          (uint32_t motorIDMask)>                                                             WaitMultipleMotors;
    PDeviceControlInvoker<53, void          (handle_id motorID, handle_id waitGroupHandle)>                                     AddMotorToWaitGroup;

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
    void SetSpeedMulti(float speedMMS, float accelerationMMS, TFirstMotorID firstMotorID, TMotorIDs... motorIDs)
    {
        SetSpeedMultiMask(MakeMotorsMask(firstMotorID, motorIDs...), speedMMS, accelerationMMS);
    }

    template<typename TFirstMotorID, typename... TMotorIDs>
    void StopAtOffsetMulti(float offset, float speed, float acceleration, TFirstMotorID firstMotorID, TMotorIDs... motorIDs)
    {
        StopAtOffsetMultiMask(MakeMotorsMask(firstMotorID, motorIDs...), offset, speed, acceleration);
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

struct MultiMotorDriverParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "multimotor";

    MultiMotorDriverParameters() = default;
    MultiMotorDriverParameters(
        const PString&  devicePath,
        DigitalPinID    pinMotorEnable,
        uint32_t        baudrate,
        const PString&  controlPortPath
    )
        : KDriverParametersBase(devicePath)
        , PinMotorEnable(pinMotorEnable)
        , Baudrate(baudrate)
        , ControlPortPath(controlPortPath)
    {}

    DigitalPinID    PinMotorEnable;
    uint32_t        Baudrate;
    PString         ControlPortPath;

    friend void to_json(Pjson& data, const MultiMotorDriverParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
        data.update(Pjson{
            {"control_port_path",   value.ControlPortPath},
            {"baudrate",            value.Baudrate },
            {"pin_motorenable",     value.PinMotorEnable }
        });
    }
    friend void from_json(const Pjson& data, MultiMotorDriverParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));

        data.at("control_port_path").get_to(outValue.ControlPortPath);
        data.at("baudrate").get_to(outValue.Baudrate);
        data.at("pin_motorenable").get_to(outValue.PinMotorEnable);
    }
};
