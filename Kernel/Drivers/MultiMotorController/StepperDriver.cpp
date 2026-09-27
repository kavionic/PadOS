// This file is part of PadOS.
//
// Copyright (c) 2019-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 09.10.2019 21:30


#include <bit>
#include <cmath>

#include <System/ExceptionHandling.h>
#include <Math/Acceleration.h>
#include <Kernel/SpinTimer.h>
#include <Kernel/Drivers/MultiMotorController/StepperDriver.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

StepperDriver::StepperDriver() : m_RunningCondition("STEPPERDRV_RUN"), m_QueueCondition("STEPPERDRV_QUEUE")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

StepperDriver::~StepperDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::Setup_trw(HWTimerID timerID, PinMuxTarget pinStep, DigitalPinID pinEnable, DigitalPinID pinDirection)
{
    if (pinStep.MUX == DigitalPinPeripheralID::None || pinDirection == DigitalPinID::None) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    TIM_TypeDef* timerChannel = get_timer_from_id(timerID);
    IRQn_Type irq = get_timer_irq(timerID, HWTimerIRQType::Update);

    if (timerChannel == nullptr || irq == IRQ_COUNT) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    m_IsInitialized = true;

    m_TimerChannel = timerChannel;
    m_PinEnable = pinEnable;
    m_PinDirection = pinDirection;
    m_PinStepMux = pinStep;
    m_PinStep = m_PinStepMux.PINID;

    if (m_PinEnable.IsValid())
    {
        m_PinEnable = true;
        m_PinEnable.SetDirection(DigitalPinDirection_e::Out);
    }
    m_PinDirection.SetDirection(DigitalPinDirection_e::Out);
    m_PinStep.SetDirection(DigitalPinDirection_e::Out);

    m_TimerPerifFrequency = get_timer_int_clock_freq(timerID);
    m_PulseWidthTicks = (m_TimerPerifFrequency * STEP_PULSE_WIDTH_US + 999999) / 1000000;
    m_TimerChannel->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_URS_Msk;
    m_TimerChannel->CCMR1 = (4U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk; // Force inactive
    m_TimerChannel->CCER = TIM_CCER_CC1E_Msk; // Enable compare 1 output
    m_TimerChannel->BDTR = TIM_BDTR_MOE_Msk;
    m_TimerChannel->CCR1 = 0;
    m_TimerChannel->ARR = 1;
    m_TimerChannel->PSC = 0;
    m_TimerChannel->SR = ~TIM_SR_UIF_Msk;
    m_PinStep.SetPeripheralMux(m_PinStepMux.MUX);
    m_TimerChannel->DIER |= TIM_DIER_UIE_Msk;
    uint32_t dbgFlagMask = 0;
    volatile uint32_t* dbgReg = get_timer_dbg_clk_flag(timerID, dbgFlagMask);
    if (dbgReg != nullptr) {
        *dbgReg |= dbgFlagMask;
    }

    NVIC_ClearPendingIRQ(irq);
    register_irq_handler(irq, IRQCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::Shutdown()
{
    ClearMotion();
    m_IsInitialized = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::SetStepsPerMillimeter(float steps)
{
    m_StepsPerMillimeter = steps;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperMotionNode::Update(bool direction, int32_t distance, float startSpeed, float cruiseSpeed, float endSpeed, float acceleration)
{
    m_StartSpeed = startSpeed;
    m_TargetSpeed = cruiseSpeed;
    m_Direction = direction;
    m_Acceleration = acceleration;

    float accDist;
    float decDist;

    PAcceleration::CalculateMaxCruiseSpeed(float(distance), startSpeed, cruiseSpeed, endSpeed, acceleration, accDist, decDist, m_CruiseSpeed, endSpeed);

    m_AccelStepsLeft = int32_t(accDist + 0.5f);
    m_DecelStepsLeft = int32_t(decDist + 0.5f);

    if (distance != StepperDriver::INFINIT_DISTANCE_I)
    {
        const int32_t totalAccSteps = m_AccelStepsLeft + m_DecelStepsLeft;
        const int32_t totalSteps = distance;

        if (totalAccSteps < totalSteps) {
            m_CruiceStepsLeft = totalSteps - totalAccSteps;
        } else {
            m_CruiceStepsLeft = 0;
        }
    }
    else
    {
        if (m_CruiseSpeed > 0.001f) {
            m_CruiceStepsLeft = std::numeric_limits<int32_t>::max();
        } else {
            m_CruiceStepsLeft = 0;
        }
    }

    m_EndSpeed = endSpeed;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::SetSpeed(float speed, float acceleration)
{
    if (!m_IsInitialized) {
        return;
    }
    MotionSnapshot snapshot = CaptureMotionSnapshot();
    const uint32_t stopGeneration = snapshot.StopGeneration;
    const bool direction = CalcDirection(speed);
    const float stepSpeed = std::abs(speed * m_StepsPerMillimeter);
    const float stepAcceleration = acceleration * m_StepsPerMillimeter;
    for (;;)
    {
        if (snapshot.StopGeneration != stopGeneration || (speed == 0.0f && !snapshot.Running)) {
            return;
        }
        StepperMotionNode nodes[2] = {};
        size_t count = 1;

        if (stepSpeed != 0.0f && (!snapshot.Running || snapshot.QueueCount == 0 || snapshot.Speed < m_Jerk))
        {
            const float startSpeed = std::max(std::min(m_Jerk, stepSpeed), snapshot.Speed);
            nodes[0].Update(direction, INFINIT_DISTANCE_I, startSpeed, stepSpeed, stepSpeed, stepAcceleration);
        }
        else if (stepSpeed != 0.0f && direction == snapshot.Direction)
        {
            nodes[0].Update(direction, INFINIT_DISTANCE_I, snapshot.Speed, stepSpeed, stepSpeed, stepAcceleration);
        }
        else
        {
            const float startSpeed = std::min(m_Jerk, stepSpeed);
            const float stopSpeed = (stepSpeed == 0.0f)
                ? std::min(snapshot.Speed, m_Jerk * 0.5f) : std::min(snapshot.Speed, m_Jerk - startSpeed);
            const float stopDistance = PAcceleration::CalcAccelerationDistance(snapshot.Speed, stopSpeed, stepAcceleration);
            nodes[0].m_Direction = snapshot.Direction;
            nodes[0].m_StartSpeed = snapshot.Speed;
            nodes[0].m_CruiseSpeed = snapshot.Speed;
            nodes[0].m_EndSpeed = stopSpeed;
            nodes[0].m_Acceleration = stepAcceleration;
            nodes[0].m_DecelStepsLeft = int32_t(stopDistance + 0.5f);
            if (stepSpeed != 0.0f)
            {
                nodes[1].Update(direction, INFINIT_DISTANCE_I, startSpeed, stepSpeed, stepSpeed, stepAcceleration);
                count = 2;
            }
        }
        if (ReplaceMotion(snapshot, nodes, count, true)) {
            return;
        }
        snapshot = CaptureMotionSnapshot();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StopAtOffset(float offset, float speed, float acceleration)
{
    StopAtPos(GetPosition() + offset, speed, acceleration);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StopAtPos(float position, float speed, float acceleration)
{
    if (!m_IsInitialized) {
        return;
    }
    MotionSnapshot snapshot = CaptureMotionSnapshot();
    const uint32_t stopGeneration = snapshot.StopGeneration;
    const int32_t stepPosition = int32_t(std::round(position * m_StepsPerMillimeter));
    const float stepSpeed = std::abs(speed * m_StepsPerMillimeter);
    const float stepAcceleration = acceleration * m_StepsPerMillimeter;
    for (;;)
    {
        if (snapshot.StopGeneration != stopGeneration) {
            return;
        }
        const int32_t distance = stepPosition - snapshot.Position;
        const bool direction = CalcDirection(distance);
        StepperMotionNode nodes[2] = {};
        size_t count = 1;
        int32_t correction = 0;

        if (snapshot.QueueCount == 0)
        {
            if (distance != 0)
            {
                nodes[0].Update(direction, std::abs(distance), std::min(m_Jerk, stepSpeed), stepSpeed, m_Jerk, stepAcceleration);
            }
        }
        else if (direction == snapshot.Direction)
        {
            nodes[0].Update(direction, std::abs(distance), snapshot.Speed, stepSpeed, m_Jerk, stepAcceleration);
            const int32_t nodeDistance = nodes[0].m_AccelStepsLeft + nodes[0].m_CruiceStepsLeft + nodes[0].m_DecelStepsLeft;
            correction = distance - (direction ? nodeDistance : -nodeDistance);
        }
        else if (snapshot.Speed < m_Jerk)
        {
            nodes[0].Update(
                direction,
                std::abs(distance),
                std::min(m_Jerk - snapshot.Speed, stepSpeed),
                stepSpeed,
                std::min(stepSpeed, m_Jerk),
                stepAcceleration);
        }
        else
        {
            const float startSpeed = std::min(m_Jerk, stepSpeed);
            const float stopSpeed = std::min(snapshot.Speed, m_Jerk - startSpeed);
            const float stopDistanceFloat = PAcceleration::CalcAccelerationDistance(snapshot.Speed, stopSpeed, stepAcceleration);
            const int32_t stopDistance = int32_t(std::ceil(stopDistanceFloat));
            nodes[0].Update(snapshot.Direction, stopDistance, snapshot.Speed, snapshot.Speed, stopSpeed, stepAcceleration);
            correction = distance - (snapshot.Direction ? stopDistance : -stopDistance);
        }
        if (correction != 0)
        {
            nodes[1].Update(
                CalcDirection(correction),
                std::abs(correction),
                std::min(m_Jerk, stepSpeed),
                stepSpeed,
                m_Jerk,
                stepAcceleration);
            count = 2;
        }
        if (ReplaceMotion(snapshot, nodes, count, snapshot.Running || snapshot.QueueCount == 0)) {
            return;
        }
        snapshot = CaptureMotionSnapshot();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::SyncMove(float distanceMM, float speedMMS, float accelerationMMS)
{
    QueueMotion(distanceMM, speedMMS, accelerationMMS);
    StartStopTimer(true);
    Wait();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::QueueMotion(float distanceMM, float speedMMS, float accelerationMMS)
{
    QueueMotionInternal(
        int32_t(std::round(distanceMM * m_StepsPerMillimeter)),
        speedMMS * m_StepsPerMillimeter,
        accelerationMMS * m_StepsPerMillimeter);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StepForward()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (!m_IsInitialized) {
        return;
    }

    while (m_IsRunning) {
        Wait();
    }

    m_TimerChannel->CCMR1 = (4U << TIM_CCMR1_OC1M_Pos);
    SpinTimer::SleepuS(1);
    m_PinDirection = !m_Reverse;
    for (int i = 0; i < 10; ++i) {
        m_TimerChannel->CCMR1 = (5 << TIM_CCMR1_OC1M_Pos); // Force active
    }
    m_TimerChannel->CCMR1 = (4U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk; // Force inactive
    if (m_PinDirection.Read()) {
        m_Position++;
    } else {
        m_Position--;
    }
    m_HasTimerPhase = false;
    ++m_MotionRevision;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StepBackward()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (!m_IsInitialized) {
        return;
    }

    while (m_IsRunning) {
        Wait();
    }

    m_TimerChannel->CCMR1 = (4U << TIM_CCMR1_OC1M_Pos);
    SpinTimer::SleepuS(1);
    m_PinDirection = m_Reverse;
    for (int i = 0; i < 10; ++i) {
        m_TimerChannel->CCMR1 = (5 << TIM_CCMR1_OC1M_Pos); // Force active
    }
    m_TimerChannel->CCMR1 = (4U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE_Msk; // Force inactive
    if (m_PinDirection.Read()) {
        m_Position++;
    } else {
        m_Position--;
    }
    m_HasTimerPhase = false;
    ++m_MotionRevision;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::Wait()
{
    if (!m_IsInitialized) {
        return;
    }

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (m_IsRunning && m_WakeupOnFullStep)
        {
            m_RunningCondition.IRQWait();
        }
        else
        {
            while (m_IsRunning)
            {
                m_RunningCondition.IRQWait();
            }
        }
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float StepperDriver::GetCurrentStopDistance(float acceleration) const
{
    if (!m_IsInitialized) {
        return 0.0f;
    }
    const MotionSnapshot snapshot = CaptureMotionSnapshot();
    if (snapshot.QueueCount == 0 || snapshot.Speed <= m_Jerk) {
        return 0.0f;
    }
    const float stopDistance = PAcceleration::CalcAccelerationDistance(
        snapshot.Speed,
        m_Jerk,
        acceleration * m_StepsPerMillimeter) / m_StepsPerMillimeter;
    return snapshot.Direction ? stopDistance : -stopDistance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StartStopTimer(bool doRun)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (!m_IsInitialized) {
        return;
    }
    if (!doRun)
    {
        ++m_StopGeneration;
        m_TimerChannel->CR1 &= ~TIM_CR1_CEN_Msk;
        if (IsInterruptFlagged() && m_IsRunning) {
            CompleteStep(false);
        }
        m_QueueCondition.Wakeup(0);
    }
    if (doRun != m_IsRunning)
    {
        if (doRun)
        {
            if (m_MotionQueueCurrentCount == 0) {
                return;
            }
            PrimeTimer(CaptureTimerPhase());
            m_IsRunning = true;
            m_TimerChannel->CR1 |= TIM_CR1_CEN_Msk;
        }
        else
        {
            m_IsRunning = false;
        }
        ++m_MotionRevision;
        m_RunningCondition.Wakeup(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::ClearMotion()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    StartStopTimer(false);
    m_MotionQueueCurrentCount = 0;
    m_MotionQueueCurrentNode = 0;
    m_MotionQueueInPos = 0;
    m_HasTimerPhase = false;
    ++m_MotionRevision;
    m_QueueCondition.Wakeup(0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::QueueMotionInternal(int32_t distance, float speed, float acceleration)
{
    if (!m_IsInitialized || distance == 0) {
        return;
    }
    const bool direction = CalcDirection(float(distance) * speed);
    distance = std::abs(distance);
    speed = std::abs(speed);
    MotionSnapshot snapshot = CaptureMotionSnapshot();
    const uint32_t stopGeneration = snapshot.StopGeneration;

    for (;;)
    {
        if (snapshot.StopGeneration != stopGeneration) {
            return;
        }
        if (snapshot.QueueCount == MOTION_BUFFER_COUNT)
        {
            CRITICAL_SCOPE(CRITICAL_IRQ);
            while (m_MotionQueueCurrentCount == MOTION_BUFFER_COUNT && m_StopGeneration == stopGeneration) {
                m_QueueCondition.IRQWait();
            }
            snapshot = CaptureMotionSnapshot();
            continue;
        }
        StepperMotionNode previous = snapshot.Tail;
        StepperMotionNode node;
        float startSpeed = std::min(m_Jerk, speed);
        const bool updatePrevious = snapshot.QueueCount != 0 && previous.m_Direction == direction;
        if (updatePrevious)
        {
            const int32_t remainingDistance = (previous.m_CruiceStepsLeft == std::numeric_limits<int32_t>::max())
                ? INFINIT_DISTANCE_I : previous.m_AccelStepsLeft + previous.m_CruiceStepsLeft + previous.m_DecelStepsLeft;
            const float previousSpeed = (snapshot.TailIndex == snapshot.CurrentNode) ? snapshot.Speed : previous.m_StartSpeed;
            previous.Update(direction, remainingDistance, previousSpeed, previous.m_TargetSpeed, speed, acceleration);
            startSpeed = std::max(startSpeed, previous.m_EndSpeed);
        }
        node.Update(direction, distance, startSpeed, speed, m_Jerk, acceleration);

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (SuspendForMotionChange(snapshot))
            {
                if (updatePrevious) {
                    m_MotionQueue[snapshot.TailIndex] = previous;
                }
                const bool wasEmpty = m_MotionQueueCurrentCount == 0;
                m_MotionQueue[m_MotionQueueInPos] = node;
                m_MotionQueueInPos = GetNextBlockIndex(m_MotionQueueInPos);
                m_MotionQueueCurrentCount = m_MotionQueueCurrentCount + 1;
                if (wasEmpty) {
                    ActivateCurrentNode();
                }
                ++m_MotionRevision;
                if (m_IsRunning)
                {
                    const StepperMotionNode& currentNode = m_MotionQueue[m_MotionQueueCurrentNode];
                    UpdateLookahead(currentNode, GetMotionPhase(currentNode));
                    m_TimerChannel->CR1 |= TIM_CR1_CEN_Msk;
                }
                else if (m_MotionQueueCurrentCount > MOTION_BUFFER_COUNT / 2)
                {
                    StartStopTimer(true);
                }
                return;
            }
        } CRITICAL_END;
        snapshot = CaptureMotionSnapshot();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

StepperDriver::MotionSnapshot StepperDriver::CaptureMotionSnapshot() const
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    MotionSnapshot snapshot = {};
    snapshot.Revision = m_MotionRevision;
    snapshot.StopGeneration = m_StopGeneration;
    snapshot.QueueCount = m_MotionQueueCurrentCount;
    snapshot.CurrentNode = m_MotionQueueCurrentNode;
    snapshot.TailIndex = GetPrevBlockIndex(m_MotionQueueInPos);
    snapshot.Position = GetStepPosition();
    snapshot.Speed = m_CurrentSpeed;
    snapshot.Running = m_IsRunning;
    if (snapshot.QueueCount != 0)
    {
        snapshot.Direction = m_MotionQueue[m_MotionQueueCurrentNode].m_Direction;
        snapshot.Tail = m_MotionQueue[snapshot.TailIndex];
    }
    else
    {
        snapshot.Direction = m_StepDirection != m_Reverse;
    }
    return snapshot;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool StepperDriver::SuspendForMotionChange(const MotionSnapshot& snapshot)
{
    if (snapshot.Revision != m_MotionRevision || snapshot.StopGeneration != m_StopGeneration ||
        snapshot.Position != GetStepPosition())
    {
        return false;
    }
    // Masking the IRQ does not stop the timer. Freeze it before checking for an unaccounted pulse.
    m_TimerChannel->CR1 &= ~TIM_CR1_CEN_Msk;
    if (IsInterruptFlagged() && m_IsRunning)
    {
        CompleteStep(true);
        if (m_IsRunning) {
            m_TimerChannel->CR1 |= TIM_CR1_CEN_Msk;
        }
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool StepperDriver::ReplaceMotion(const MotionSnapshot& snapshot, const StepperMotionNode* nodes, size_t count, bool run)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (!SuspendForMotionChange(snapshot)) {
        return false;
    }
    const TimerPhase phase = CaptureTimerPhase();
    uint32_t nodeCount = 0;
    for (size_t index = 0; index < count; ++index)
    {
        if (!IsMotionEmpty(nodes[index])) {
            m_MotionQueue[nodeCount++] = nodes[index];
        }
    }
    m_MotionQueueCurrentNode = 0;
    m_MotionQueueInPos = nodeCount;
    m_MotionQueueCurrentCount = nodeCount;
    ActivateCurrentNode();

    const bool wasRunning = m_IsRunning;
    m_IsRunning = run && m_MotionQueueCurrentCount != 0;
    if (m_IsRunning)
    {
        PrimeTimer(phase);
        m_TimerChannel->CR1 |= TIM_CR1_CEN_Msk;
    }
    else
    {
        m_HasTimerPhase = false;
    }
    ++m_MotionRevision;
    m_QueueCondition.Wakeup(0);
    if (wasRunning != m_IsRunning) {
        m_RunningCondition.Wakeup(0);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

StepperDriver::TimerPhase StepperDriver::CaptureTimerPhase() const
{
    TimerPhase phase;
    phase.High = m_PinStep.Read();
    if (m_HasTimerPhase)
    {
        const uint32_t counter = m_TimerChannel->CNT;
        phase.ElapsedTicks = m_ElapsedTimerTicks + (uint64_t(counter) << m_ActiveInterval.PrescalerShift);
        if (!phase.High && counter >= m_ActiveInterval.Pulse)
        {
            const uint32_t lowTicks = (counter - m_ActiveInterval.Pulse) << m_ActiveInterval.PrescalerShift;
            phase.LowTicks = m_LowTimerTicks + std::min(lowTicks, m_PulseWidthTicks - m_LowTimerTicks);
        }
    }
    return phase;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

StepperDriver::TimerInterval StepperDriver::CalculateTimerInterval(float speed)
{
    if (speed != m_CachedTimerSpeed)
    {
        const float period = (speed > 0.0f)
            ? std::max(float(m_TimerPerifFrequency) / speed, float(m_PulseWidthTicks * 2)) : TIMER_MAX_REFERENCE_PERIOD;
        if (period >= TIMER_MAX_REFERENCE_PERIOD)
        {
            m_CachedInterval.PrescalerShift = TIMER_REGISTER_BITS;
            m_CachedInterval.Reload = TIMER_MAX_RELOAD;
        }
        else
        {
            const uint32_t referenceReload = uint32_t(std::ceil(period)) - 1;
            const uint32_t usedBits = std::bit_width(referenceReload);
            m_CachedInterval.PrescalerShift = (usedBits > TIMER_REGISTER_BITS) ? usedBits - TIMER_REGISTER_BITS : 0;
            m_CachedInterval.Reload = referenceReload >> m_CachedInterval.PrescalerShift;
        }
        const uint32_t roundMask = (1U << m_CachedInterval.PrescalerShift) - 1;
        m_CachedInterval.Pulse = (m_PulseWidthTicks + roundMask) >> m_CachedInterval.PrescalerShift;
        m_CachedTimerSpeed = speed;
    }
    return m_CachedInterval;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::PrimeTimer(const TimerPhase& phase)
{
    // Used only at startup, resume, or explicit motion replacement. The step ISR never generates UG.
    // Keep STEP at its existing level while loading the active registers, then time the next real rising edge.
    m_TimerChannel->CCMR1 = (phase.High ? (5U << TIM_CCMR1_OC1M_Pos) : (4U << TIM_CCMR1_OC1M_Pos)) |
        TIM_CCMR1_OC1PE_Msk;
    m_TimerChannel->CR1 = TIM_CR1_ARPE_Msk | TIM_CR1_URS_Msk;
    m_StopAfterCurrentStep = false;

    m_ActiveInterval = CalculateTimerInterval(m_CurrentSpeed);
    const uint32_t shift = m_ActiveInterval.PrescalerShift;
    const uint32_t roundMask = (1U << shift) - 1;
    const uint64_t periodTicks = uint64_t(m_ActiveInterval.Reload + 1) << shift;
    const uint64_t remainingTicks = (periodTicks > phase.ElapsedTicks) ? periodTicks - phase.ElapsedTicks : 0;
    const uint32_t remainingHigh = (phase.High && phase.ElapsedTicks < m_PulseWidthTicks)
        ? m_PulseWidthTicks - uint32_t(phase.ElapsedTicks) : 0;
    const uint32_t remainingLow = phase.High ? m_PulseWidthTicks : m_PulseWidthTicks - phase.LowTicks;
    const uint32_t highCount = phase.High ? std::max<uint32_t>(1, (remainingHigh + roundMask) >> shift) : 0;
    const uint32_t lowCount = std::max<uint32_t>(1, (remainingLow + roundMask) >> shift);
    const uint32_t remainingCount = uint32_t((remainingTicks + roundMask) >> shift);
    const uint32_t periodCount = std::max<uint32_t>(2, std::max(remainingCount, highCount + lowCount));

    // Carry elapsed time across repeated replacements, including when the new deadline is already in the past.
    // Only preserve the minimum high/low durations; never manufacture catch-up pulses with UG.
    m_ActiveInterval.Reload = periodCount - 1;
    m_ActiveInterval.Pulse = highCount;
    m_ElapsedTimerTicks = phase.ElapsedTicks;
    m_LowTimerTicks = phase.High ? 0 : phase.LowTicks;
    m_HasTimerPhase = true;

    m_PinDirection = m_StepDirection;
    m_TimerChannel->PSC = (1U << shift) - 1;
    m_TimerChannel->ARR = m_ActiveInterval.Reload;
    m_TimerChannel->CCR1 = m_ActiveInterval.Pulse;
    m_TimerChannel->EGR = TIM_EGR_UG_Msk;
    m_TimerChannel->SR = ~TIM_SR_UIF_Msk;
    m_PreloadedInterval = m_ActiveInterval;
    const StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
    UpdateLookahead(node, GetMotionPhase(node));
    m_TimerChannel->CCMR1 = TIMER_PWM_CONFIG;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::UpdateLookahead(const StepperMotionNode& node, MotionPhase phase)
{
    // Commands can replace a cruise with a ramp or change the interval already preloaded.
    m_CruiseIntervalReady = false;
    m_NextSpeed = m_CurrentSpeed;
    bool nodeComplete;
    if (phase == MotionPhase::Acceleration)
    {
        m_NextSpeed = AdvanceSpeed(m_CurrentSpeed, node.m_CruiseSpeed, node.m_Acceleration, m_Jerk);
        nodeComplete = node.m_AccelStepsLeft == 1 && node.m_CruiceStepsLeft == 0 && node.m_DecelStepsLeft == 0;
    }
    else if (phase == MotionPhase::Cruise)
    {
        nodeComplete = node.m_CruiceStepsLeft == 1 && node.m_DecelStepsLeft == 0;
    }
    else
    {
        m_NextSpeed = AdvanceSpeed(m_CurrentSpeed, node.m_EndSpeed, node.m_Acceleration, m_Jerk);
        nodeComplete = node.m_DecelStepsLeft == 1;
    }
    const bool hasNext = !nodeComplete || m_MotionQueueCurrentCount > 1;
    if (nodeComplete && hasNext)
    {
        const StepperMotionNode& nextNode = m_MotionQueue[GetNextBlockIndex(m_MotionQueueCurrentNode)];
        m_NextSpeed = GetInitialSpeed(nextNode);
    }
    // A final update must still produce the last rising edge. OPM stops the counter at that edge,
    // without relying on IRQ latency to prevent another pulse.
    const TimerInterval nextInterval = CalculateTimerInterval(hasNext ? m_NextSpeed : m_CurrentSpeed);
    if (nextInterval.PrescalerShift != m_PreloadedInterval.PrescalerShift)
    {
        m_PreloadedInterval.PrescalerShift = nextInterval.PrescalerShift;
        m_TimerChannel->PSC = (1U << nextInterval.PrescalerShift) - 1;
    }
    if (nextInterval.Reload != m_PreloadedInterval.Reload)
    {
        m_PreloadedInterval.Reload = nextInterval.Reload;
        m_TimerChannel->ARR = nextInterval.Reload;
    }
    if (nextInterval.Pulse != m_PreloadedInterval.Pulse)
    {
        m_PreloadedInterval.Pulse = nextInterval.Pulse;
        m_TimerChannel->CCR1 = nextInterval.Pulse;
    }
    if (hasNext == m_StopAfterCurrentStep)
    {
        m_StopAfterCurrentStep = !hasNext;
        if (hasNext) {
            m_TimerChannel->CR1 &= ~TIM_CR1_OPM_Msk;
        } else {
            m_TimerChannel->CR1 |= TIM_CR1_OPM_Msk;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::CompleteStep(bool prepareNext)
{
    m_Position += m_StepDirection ? 1 : -1;
    ++m_MotionRevision;

    StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
    // Leave two cruise steps for lookahead to prepare a phase/node transition or the final hardware stop.
    if (m_CruiseIntervalReady && node.m_CruiceStepsLeft > 2 && m_StepDirection == (node.m_Direction != m_Reverse))
    {
        if (node.m_CruiceStepsLeft != std::numeric_limits<int32_t>::max()) {
            --node.m_CruiceStepsLeft;
        }
    }
    else
    {
        UpdateMotionAfterStep(node, prepareNext);
    }
    if (m_IsRunning && m_WakeupOnFullStep && (m_Position & 0x0f) == 0) {
        m_RunningCondition.Wakeup(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::UpdateMotionAfterStep(StepperMotionNode& node, bool prepareNext)
{
    m_CruiseIntervalReady = false;
    m_ActiveInterval = m_PreloadedInterval;
    m_ElapsedTimerTicks = 0;
    m_LowTimerTicks = 0;
    m_CurrentSpeed = m_NextSpeed;

    MotionPhase phase;
    bool phaseComplete;
    if (node.m_AccelStepsLeft != 0)
    {
        phase = MotionPhase::Acceleration;
        phaseComplete = --node.m_AccelStepsLeft == 0;
    }
    else if (node.m_CruiceStepsLeft != 0)
    {
        phase = MotionPhase::Cruise;
        phaseComplete = node.m_CruiceStepsLeft != std::numeric_limits<int32_t>::max() && --node.m_CruiceStepsLeft == 0;
    }
    else
    {
        phase = MotionPhase::Deceleration;
        phaseComplete = --node.m_DecelStepsLeft == 0;
    }

    bool nodeComplete = false;
    if (phaseComplete)
    {
        if (phase == MotionPhase::Acceleration && node.m_CruiceStepsLeft != 0) {
            phase = MotionPhase::Cruise;
        } else if (phase != MotionPhase::Deceleration && node.m_DecelStepsLeft != 0) {
            phase = MotionPhase::Deceleration;
        } else {
            nodeComplete = true;
        }
    }
    StepperMotionNode* currentNode = &node;
    if (nodeComplete)
    {
        const uint32_t nextIndex = GetNextBlockIndex(m_MotionQueueCurrentNode);
        const uint32_t remainingNodes = m_MotionQueueCurrentCount - 1;
        m_MotionQueueCurrentNode = nextIndex;
        m_MotionQueueCurrentCount = remainingNodes;
        m_QueueCondition.Wakeup(0);
        if (remainingNodes == 0)
        {
            m_TimerChannel->CR1 &= ~TIM_CR1_CEN_Msk;
            m_IsRunning = false;
            m_HasTimerPhase = false;
            m_RunningCondition.Wakeup(0);
            return;
        }
        currentNode = &m_MotionQueue[nextIndex];
        phase = GetMotionPhase(*currentNode);
    }
    const bool direction = currentNode->m_Direction != m_Reverse;
    if (prepareNext && direction != m_StepDirection) {
        m_PinDirection = direction;
    }
    m_StepDirection = direction;
    if (prepareNext)
    {
        UpdateLookahead(*currentNode, phase);
        // A real update has promoted the preload and cleared any partial interval from PrimeTimer().
        m_CruiseIntervalReady = phase == MotionPhase::Cruise && currentNode->m_CruiceStepsLeft > 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float StepperDriver::AdvanceSpeed(float speed, float targetSpeed, float acceleration, float jerk)
{
    const float delta = acceleration / std::max(speed, jerk);
    return (speed < targetSpeed) ? std::min(speed + delta, targetSpeed) : std::max(speed - delta, targetSpeed);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float StepperDriver::GetInitialSpeed(const StepperMotionNode& node)
{
    return (node.m_AccelStepsLeft > 0) ? node.m_StartSpeed : node.m_CruiseSpeed;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

StepperDriver::MotionPhase StepperDriver::GetMotionPhase(const StepperMotionNode& node)
{
    if (node.m_AccelStepsLeft != 0) {
        return MotionPhase::Acceleration;
    } else if (node.m_CruiceStepsLeft != 0) {
        return MotionPhase::Cruise;
    } else {
        return MotionPhase::Deceleration;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool StepperDriver::IsMotionEmpty(const StepperMotionNode& node)
{
    return node.m_AccelStepsLeft == 0 && node.m_CruiceStepsLeft == 0 && node.m_DecelStepsLeft == 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool StepperDriver::IsInterruptFlagged() const
{
    const bool flagged = (m_TimerChannel->SR & TIM_SR_UIF_Msk) != 0;
    if (flagged) {
        m_TimerChannel->SR = ~TIM_SR_UIF_Msk;
    }
    return flagged;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult StepperDriver::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<StepperDriver*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult StepperDriver::HandleIRQ()
{
    if (!IsInterruptFlagged()) {
        return IRQResult::UNHANDLED;
    }
    if (m_IsRunning) {
        CompleteStep(true);
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::ActivateCurrentNode()
{
    if (m_MotionQueueCurrentCount != 0)
    {
        const StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
        m_CurrentSpeed = GetInitialSpeed(node);
        m_StepDirection = node.m_Direction != m_Reverse;
    }
}


} // namespace kernel
