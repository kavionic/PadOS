// This file is part of PadOS.
//
// Copyright (C) 2019-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 09.10.2019 21:30


#pragma once

#include <System/Platform.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/IRQDispatcher.h>

namespace kernel
{

struct StepperMotionNode
{
    void Update(bool direction, int32_t distance, float startSpeed, float cruiseSpeed, float endSpeed, float acceleration);
    
    int32_t m_AccelStepsLeft;
    int32_t m_CruiceStepsLeft;
    int32_t m_DecelStepsLeft;

    float   m_TargetSpeed;  // The requested speed.

    float   m_StartSpeed;   // End speed of previous move.
    float   m_CruiseSpeed;  // The maximum speed we will actually be able to reach.
    float   m_EndSpeed;     // Start speed of next move.
    float   m_Acceleration;
    bool    m_Direction;   
};

class StepperDriver : public PtrTarget
{
public:
    static constexpr int32_t INFINIT_DISTANCE_I = std::numeric_limits<int32_t>::max() - 1000;
    static constexpr float   INFINIT_DISTANCE_F = float(INFINIT_DISTANCE_I);

    StepperDriver();
    virtual ~StepperDriver();

    void Setup_trw(HWTimerID timerID, uint32_t timerClkFrequency, PinMuxTarget pinStep, DigitalPinID pinEnable, DigitalPinID pinDirection);

    void            SetJerk(float jerk) { m_Jerk = jerk * m_StepsPerMillimeter; }
    void            SetReverse(bool reverse) { m_Reverse = reverse; }
    bool            GetReverse() const { return m_Reverse; }
    void            Shutdown();
    
    inline bool     IsInitialized() const { return m_IsInitialized; }

    void            SetStepsPerMillimeter(float steps);
    inline float    GetStepsPerMillimeter() const { return m_StepsPerMillimeter; }
    
    inline void     SetWakeupOnFullStep(bool value) { m_WakeupOnFullStep = value; }
    inline bool     GetWakeupOnFullStep() const     { return m_WakeupOnFullStep; }

    void            SetSpeed(float speedMMS, float accelerationMMS);
    void            StopAtOffset(float offset, float speed, float acceleration);
    void            StopAtPos(float position, float speed, float acceleration);
    void            SyncMove(float distanceMM, float speedMMS, float accelerationMMS);
    void            QueueMotion(float distanceMM, float speedMMS, float accelerationMMS);
    void            StepForward();
    void            StepBackward();
    inline void     EnableMotor(bool enable) { if (m_IsInitialized) { if (m_PinEnable.IsValid()) m_PinEnable = !enable; } }
    inline bool     IsMotorEnabled() const { return m_IsInitialized && (!m_PinEnable.IsValid() ||!m_PinEnable.Read()); }
    void            Wait();
    float           GetCurrentStopDistance(float acceleration) const;

    inline bool     IsRunning() const { return m_IsRunning; }
    void            StartStopTimer(bool doRun);
    void            ClearMotion();

    inline KConditionVariable& GetRunningCondition() { return m_RunningCondition; }

	inline int32_t  GetStepPosition() const { return m_Reverse ? (-m_Position) : m_Position; }
	inline float    GetPosition() const { return float(GetStepPosition()) / m_StepsPerMillimeter; }

    inline void	    ResetPosition(float position = 0.0f) { m_Position = (m_Reverse) ? -int32_t(position * m_StepsPerMillimeter) : int32_t(position * m_StepsPerMillimeter); }
    
    inline float    GetCurrentSpeed() const { return m_CurrentSpeed / m_StepsPerMillimeter; }
    inline bool     GetCurrentDirection() const { return m_Reverse ? (!m_PinDirection.Read()) : m_PinDirection.Read(); }

    inline static void StartSteppers(StepperDriver& driver) { driver.StartStopTimer(true); }
    template<typename ...DRIVERS>
    static void StartSteppers(StepperDriver& driver, DRIVERS&... drivers) { driver.StartStopTimer(true); StartSteppers(drivers...); }

    static inline void WaitMulti(StepperDriver& driver) { driver.Wait(); }
    template<typename... DRIVERS>
    static void WaitMulti(StepperDriver& driver, DRIVERS&... drivers) { driver.Wait(); WaitMulti(drivers...); }
        
    template<typename... DRIVERS>
    static void SyncStartSteppers(bool doWait, DRIVERS&... drivers)
    {
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
        	StartSteppers(drivers...);
        } CRITICAL_END;
        if (doWait) {
            WaitMulti(drivers...);
        }            
    }
    static inline bool CalcDirection(float speed) { return speed >= 0.0f; }
    static inline bool CalcDirection(int32_t speed) { return speed >= 0; }
protected:
private:
    static const int MOTION_BUFFER_COUNT = 16;

    void                QueueMotionInternal(int32_t distance, float speed, float acceleration);

    inline bool         IsInterruptFlagged() const { bool wasTrigged = (m_TimerChannel->SR & TIM_SR_UIF_Msk) != 0; m_TimerChannel->SR &= ~TIM_SR_UIF_Msk; return wasTrigged; }
    void                SetTimerSpeed(float speed);

    static IRQResult    IRQCallback(IRQn_Type irq, void* userData);
	IRQResult           HandleIRQ();
    void                ActivateCurrentNode();

    static inline uint32_t GetNextBlockIndex(uint32_t index) { return (index + 1) & (MOTION_BUFFER_COUNT - 1); }
    static inline uint32_t GetPrevBlockIndex(uint32_t index) { return (index - 1) & (MOTION_BUFFER_COUNT - 1); }

    MCU_Timer16_t*      m_TimerChannel;
    DigitalPin          m_PinEnable;
    DigitalPin          m_PinDirection;
    DigitalPin          m_PinStep;
    PinMuxTarget        m_PinStepMux;

    uint32_t            m_TimerPerifFrequency = 0;
    uint32_t            m_TimerFrequency = 0;
    float               m_StepsPerMillimeter = 1.0f;
    float               m_Jerk = 350.0f;

	KConditionVariable  m_RunningCondition;

    StepperMotionNode   m_MotionQueue[MOTION_BUFFER_COUNT];
    volatile uint32_t   m_MotionQueueCurrentNode = 0;
    uint32_t            m_MotionQueueInPos = 0;
    volatile uint32_t   m_MotionQueueCurrentCount = 0;
    float               m_CurrentSpeed = 0.0f;
    int32_t             m_Position = 0;
    volatile bool       m_IsRunning = false;
    volatile bool       m_WakeupOnFullStep = false;
    bool                m_IsInitialized = false;
    bool                m_Reverse = false;


    StepperDriver(const StepperDriver &) = delete;
    StepperDriver& operator=(const StepperDriver &) = delete;
};

} // namespace kernel
