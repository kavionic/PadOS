// This file is part of PadOS.
//
// Copyright (c) 2019-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    // Thread-side calls, including configuration and shutdown, must be serialized by the caller.
    // IRQ guards protect motion state shared with the timer and stall handlers.
    static constexpr int32_t INFINIT_DISTANCE_I = std::numeric_limits<int32_t>::max() - 1000;
    static constexpr float   INFINIT_DISTANCE_F = float(INFINIT_DISTANCE_I);

    StepperDriver();
    virtual ~StepperDriver();

    void Setup_trw(HWTimerID timerID, PinMuxTarget pinStep, DigitalPinID pinEnable, DigitalPinID pinDirection);

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
    static constexpr uint32_t TIMER_REGISTER_BITS = 16;
    static constexpr uint32_t STEP_PULSE_WIDTH_US = 1;
    static constexpr uint32_t TIMER_MAX_RELOAD = (1U << TIMER_REGISTER_BITS) - 1;
    static constexpr float TIMER_MAX_REFERENCE_PERIOD = 4294967296.0f;
    static constexpr uint32_t TIMER_PWM_CONFIG = (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk;

    enum class MotionPhase
    {
        Acceleration,
        Cruise,
        Deceleration
    };

    struct TimerInterval
    {
        uint32_t PrescalerShift = 0;
        uint32_t Reload = 1;
        uint32_t Pulse = 1;
    };

    struct TimerPhase
    {
        uint64_t ElapsedTicks = 0;
        uint32_t LowTicks = 0; // Saturated at the nominal pulse width.
        bool High = false;
    };

    struct MotionSnapshot
    {
        uint32_t Revision;
        uint32_t StopGeneration;
        uint32_t QueueCount;
        uint32_t CurrentNode;
        uint32_t TailIndex;
        int32_t Position;
        float Speed;
        bool Direction;
        bool Running;
        StepperMotionNode Tail;
    };

    void                QueueMotionInternal(int32_t distance, float speed, float acceleration);
    MotionSnapshot      CaptureMotionSnapshot() const;
    bool                SuspendForMotionChange(const MotionSnapshot& snapshot);
    bool                ReplaceMotion(const MotionSnapshot& snapshot, const StepperMotionNode* nodes, size_t count, bool run);
    TimerPhase          CaptureTimerPhase() const;
    TimerInterval       CalculateTimerInterval(float speed);
    void                PrimeTimer(const TimerPhase& phase);
    void                UpdateLookahead(const StepperMotionNode& node, MotionPhase phase);
    void                CompleteStep(bool prepareNext);
    void                UpdateMotionAfterStep(StepperMotionNode& node, bool prepareNext);
    static float        AdvanceSpeed(float speed, float targetSpeed, float acceleration, float jerk);
    static float        GetInitialSpeed(const StepperMotionNode& node);
    static MotionPhase  GetMotionPhase(const StepperMotionNode& node);
    static bool         IsMotionEmpty(const StepperMotionNode& node);
    bool                IsInterruptFlagged() const;

    static IRQResult    IRQCallback(IRQn_Type irq, void* userData);
	IRQResult           HandleIRQ();
    void                ActivateCurrentNode();

    static inline uint32_t GetNextBlockIndex(uint32_t index) { return (index + 1) & (MOTION_BUFFER_COUNT - 1); }
    static inline uint32_t GetPrevBlockIndex(uint32_t index) { return (index - 1) & (MOTION_BUFFER_COUNT - 1); }

    MCU_Timer16_t*       m_TimerChannel = nullptr;
    DigitalPin          m_PinEnable;
    DigitalPin          m_PinDirection;
    DigitalPin          m_PinStep;
    PinMuxTarget        m_PinStepMux;

    uint32_t            m_TimerPerifFrequency = 0;
    float               m_StepsPerMillimeter = 1.0f;
    float               m_Jerk = 350.0f;

	KConditionVariable  m_RunningCondition;
    KConditionVariable  m_QueueCondition;

    TimerInterval       m_ActiveInterval;
    TimerInterval       m_PreloadedInterval;
    TimerInterval       m_CachedInterval;
    float               m_CachedTimerSpeed = -1.0f;
    float               m_NextSpeed = 0.0f;
    uint64_t            m_ElapsedTimerTicks = 0;
    uint32_t            m_LowTimerTicks = 0;
    uint32_t            m_PulseWidthTicks = 1;
    uint32_t            m_MotionRevision = 0;
    uint32_t            m_StopGeneration = 0;
    bool                m_HasTimerPhase = false;
    bool                m_StopAfterCurrentStep = false;
    bool                m_CruiseIntervalReady = false;
    bool                m_StepDirection = false;

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
