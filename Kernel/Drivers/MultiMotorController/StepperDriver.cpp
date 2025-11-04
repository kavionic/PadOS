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


#include <cmath>

#include <System/ExceptionHandling.h>
#include <Math/Acceleration.h>
#include <Kernel/SpinTimer.h>
#include <Kernel/Drivers/MultiMotorController/StepperDriver.h>

#include "SystemSetup.h"

using namespace os;


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

StepperDriver::StepperDriver() : m_RunningCondition("STEPPERDRV_RUN")
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

void StepperDriver::Setup_trw(HWTimerID timerID, uint32_t timerClkFrequency, PinMuxTarget pinStep, DigitalPinID pinEnable, DigitalPinID pinDirection)
{
    if (pinStep.MUX == DigitalPinPeripheralID::None || pinDirection == DigitalPinID::None) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    TIM_TypeDef* timerChannel = get_timer_from_id(timerID);
    IRQn_Type irq = get_timer_irq(timerID, HWTimerIRQType::Update);

    if (timerChannel == nullptr || irq == IRQ_COUNT) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
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

    m_PinStep.SetPeripheralMux(m_PinStepMux.MUX);

    m_TimerPerifFrequency = timerClkFrequency;
    m_TimerFrequency = 3000000;
    m_TimerChannel->CR1 = TIM_CR1_ARPE_Msk;
    m_TimerChannel->CCMR1 = (6 << TIM_CCMR1_OC1M_Pos); // PWM-mode1
    m_TimerChannel->CCER = TIM_CCER_CC1E_Msk; // Enable compare 1 output
    m_TimerChannel->BDTR = TIM_BDTR_MOE_Msk;
    m_TimerChannel->CCR1 = 10;
    m_TimerChannel->PSC = m_TimerPerifFrequency / m_TimerFrequency - 1;
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

    Acceleration::CalculateMaxCruiseSpeed(float(distance), startSpeed, cruiseSpeed, endSpeed, acceleration, accDist, decDist, m_CruiseSpeed, endSpeed);

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
    const bool direction = CalcDirection(speed);
    speed = std::abs(speed * m_StepsPerMillimeter);
    acceleration *= m_StepsPerMillimeter;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (speed != 0.0f)
        {
            if (!m_IsRunning || m_MotionQueueCurrentCount == 0 || m_CurrentSpeed < m_Jerk)
            {
                //            printf("%p: Start from %.3f to %.3f\n", m_TimerChannel, m_CurrentSpeed, speed);
                m_MotionQueue[0].Update(direction, INFINIT_DISTANCE_I, std::max(m_Jerk, m_CurrentSpeed), speed, speed, acceleration);

                m_MotionQueueCurrentNode = 0;
                m_MotionQueueInPos = 1;
                m_MotionQueueCurrentCount = 1;
                ActivateCurrentNode();
                if (!m_IsRunning) {
                    StartStopTimer(true);
                }
            }
            else
            {
                StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
                if (direction == node.m_Direction)
                {
                    //                printf("%p: update from %.3f to %.3f\n", m_TimerChannel, m_CurrentSpeed, speed);
                    node.Update(node.m_Direction, INFINIT_DISTANCE_I, m_CurrentSpeed, speed, speed, acceleration);
                    m_MotionQueueInPos = GetNextBlockIndex(m_MotionQueueCurrentNode);
                    m_MotionQueueCurrentCount = 1;
                }
                else
                {
                    //                printf("%p: reverse from %.3f to %.3f\n", m_TimerChannel, m_CurrentSpeed, speed);
                    const float startSpeed = std::min(m_Jerk, speed);
                    const float stopSpeed = std::min(m_CurrentSpeed, m_Jerk - startSpeed);
                    const float stopDist = Acceleration::CalcAccelerationDistance(m_CurrentSpeed, stopSpeed, acceleration);

                    node.m_Acceleration = acceleration;
                    node.m_AccelStepsLeft = 0;
                    node.m_CruiceStepsLeft = 0;
                    node.m_DecelStepsLeft = int32_t(stopDist + 0.5f);
                    node.m_EndSpeed = stopSpeed;
                    node.m_StartSpeed = m_CurrentSpeed;
                    node.m_TargetSpeed = 0;

                    StepperMotionNode& nextNode = m_MotionQueue[GetNextBlockIndex(m_MotionQueueCurrentNode)];
                    nextNode.Update(direction, INFINIT_DISTANCE_I, startSpeed, speed, speed, acceleration);
                    m_MotionQueueInPos = GetNextBlockIndex(m_MotionQueueCurrentNode + 1);
                    m_MotionQueueCurrentCount = 2;
                }
            }
        }
        else if (m_IsRunning)
        {
            StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
            m_MotionQueueCurrentCount = 1;
            m_MotionQueueInPos = GetNextBlockIndex(m_MotionQueueCurrentNode);

            const float stopSpeed = std::min(m_CurrentSpeed, m_Jerk * 0.5f);
            const float stopDist = Acceleration::CalcAccelerationDistance(m_CurrentSpeed, stopSpeed, acceleration);

            node.m_Acceleration = acceleration;
            node.m_AccelStepsLeft = 0;
            node.m_CruiceStepsLeft = 0;
            node.m_DecelStepsLeft = int32_t(stopDist + 0.5f);
            node.m_EndSpeed = stopSpeed;
            node.m_StartSpeed = m_CurrentSpeed;
            node.m_TargetSpeed = 0;
        }
    } CRITICAL_END;
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
    const int32_t stepPosition = int32_t(position * m_StepsPerMillimeter + 0.5f);

    speed *= m_StepsPerMillimeter;
    acceleration *= m_StepsPerMillimeter;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        const int32_t currentPosition = GetStepPosition();
        const int32_t distance = stepPosition - currentPosition;
        if (m_MotionQueueCurrentCount > 0)
        {
            StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
            m_MotionQueueInPos = GetNextBlockIndex(m_MotionQueueCurrentNode);
            m_MotionQueueCurrentCount = 1;
            const bool newDirection = CalcDirection(distance);
            if (newDirection == node.m_Direction)
            {
                node.Update(newDirection, std::abs(distance), m_CurrentSpeed, speed, m_Jerk, acceleration);
                const int32_t nodeDistance = (node.m_AccelStepsLeft + node.m_CruiceStepsLeft + node.m_DecelStepsLeft) * (newDirection ? 1 : -1);
                const int32_t stopPos = currentPosition + nodeDistance;
                if (stopPos != stepPosition)
                {
                    // Correct for overshoot if necessary.
                    QueueMotionInternal(stepPosition - stopPos, speed, acceleration);
                }
            }
            else
            {
                if (m_CurrentSpeed < m_Jerk)
                {
                    node.Update(newDirection, std::abs(distance), m_Jerk - m_CurrentSpeed, speed, std::min(speed, m_Jerk), acceleration);
                    m_CurrentSpeed = 0.0f;
                    ActivateCurrentNode();
                }
                else
                {
                    const float startSpeed = std::min(m_Jerk, speed);
                    const float stopSpeed = std::min(m_CurrentSpeed, m_Jerk - startSpeed);
                    const int32_t stopDist = int32_t(ceil(Acceleration::CalcAccelerationDistance(m_CurrentSpeed, stopSpeed, acceleration)));
                    node.Update(node.m_Direction, stopDist, m_CurrentSpeed, m_CurrentSpeed, stopSpeed, acceleration);
                    if (node.m_Direction) {
                        QueueMotionInternal(distance - stopDist, speed, acceleration);
                    } else {
                        QueueMotionInternal(distance + stopDist, speed, acceleration);
                    }
                }
            }
        }
        else
        {
            QueueMotionInternal(distance, speed, acceleration);
            StartStopTimer(true);
        }
    } CRITICAL_END;
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
    QueueMotionInternal(int32_t(distanceMM * m_StepsPerMillimeter + 0.5f), speedMMS * m_StepsPerMillimeter, accelerationMMS * m_StepsPerMillimeter);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StepForward()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (!m_IsInitialized) return;

    while (m_IsRunning) Wait();

    m_PinDirection = !m_Reverse;
    for (int i = 0; i < 10; ++i) {
        m_TimerChannel->CCMR1 = (5 << TIM_CCMR1_OC1M_Pos); // Force active
    }
    m_TimerChannel->CCMR1 = (6 << TIM_CCMR1_OC1M_Pos); // PWM-mode1
    if (m_PinDirection) {
        m_Position++;
    } else {
        m_Position--;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StepBackward()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (!m_IsInitialized) return;

    while (m_IsRunning) Wait();

    m_PinDirection = m_Reverse;
    for (int i = 0; i < 10; ++i) {
        m_TimerChannel->CCMR1 = (5 << TIM_CCMR1_OC1M_Pos); // Force active
    }
    m_TimerChannel->CCMR1 = (6 << TIM_CCMR1_OC1M_Pos); // PWM-mode1
    if (m_PinDirection) {
        m_Position++;
    } else {
        m_Position--;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::Wait()
{
    if (!m_IsInitialized) return;

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
    float stopDist = 0.0f;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (!m_IsInitialized) return 0.0f;
        if (m_MotionQueueCurrentCount > 0 && m_CurrentSpeed > m_Jerk)
        {
            const StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
            stopDist = Acceleration::CalcAccelerationDistance(m_CurrentSpeed, m_Jerk, acceleration * m_StepsPerMillimeter);
            if (node.m_Direction == m_Reverse) {
                stopDist = -stopDist;
            }
        }
        else
        {
            return 0.0f;
        }
    } CRITICAL_END;
    return stopDist / m_StepsPerMillimeter;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::StartStopTimer(bool doRun)
{
    if (!m_IsInitialized) return;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (doRun && m_MotionQueueCurrentCount == 0) {
            return;
        }
        if (doRun != m_IsRunning)
        {
            m_IsRunning = doRun;
            if (m_IsRunning) {
                m_TimerChannel->EGR = TIM_EGR_UG_Msk;
                m_TimerChannel->CR1 |= TIM_CR1_CEN_Msk;
            }
            else {
                m_TimerChannel->CR1 &= ~TIM_CR1_CEN_Msk;
            }
            m_RunningCondition.Wakeup(0);
        }
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::ClearMotion()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    m_MotionQueueCurrentCount = 0;
    m_MotionQueueCurrentNode = 0;
    m_MotionQueueInPos = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::QueueMotionInternal(int32_t distance, float speed, float acceleration)
{
    if (distance == 0) {
        return;
    }
    const bool direction = CalcDirection(float(distance) * speed);

    distance = std::abs(distance);
    speed = std::abs(speed);

    while (m_MotionQueueCurrentCount == MOTION_BUFFER_COUNT);

    CRITICAL_SCOPE(CRITICAL_IRQ);

    if (m_MotionQueueCurrentCount < MOTION_BUFFER_COUNT)
    {
        StepperMotionNode& node = m_MotionQueue[m_MotionQueueInPos];

        float startSpeed = std::min(m_Jerk, speed);
        if (m_MotionQueueCurrentCount > 0)
        {
            StepperMotionNode& prevNode = m_MotionQueue[GetPrevBlockIndex(m_MotionQueueInPos)];
            if (direction == prevNode.m_Direction)
            {
                const int32_t remainingDistance = prevNode.m_AccelStepsLeft + prevNode.m_CruiceStepsLeft + prevNode.m_DecelStepsLeft;
                prevNode.Update(direction, remainingDistance, prevNode.m_StartSpeed, prevNode.m_TargetSpeed, speed, acceleration);
                startSpeed = std::max(startSpeed, prevNode.m_EndSpeed);
                //                if (startSpeed < m_Jerk) startSpeed = m_Jerk;
            }
        }

        node.Update(direction, distance, startSpeed, speed, m_Jerk, acceleration);

        if (m_MotionQueueCurrentCount == 0)
        {
            ActivateCurrentNode();
        }
        m_MotionQueueInPos = GetNextBlockIndex(m_MotionQueueInPos);
        m_MotionQueueCurrentCount++;
        if (m_MotionQueueCurrentCount > MOTION_BUFFER_COUNT / 2) {
            StartStopTimer(true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::SetTimerSpeed(float speed)
{
    if (speed > 0.0f)
    {
        uint32_t timerFrequency = uint32_t(std::min(3e6f/*m_TimerPerifFrequency*/, speed * 65535.0f));
        const uint32_t divider = std::min(65536ul, (m_TimerPerifFrequency + timerFrequency - 1) / timerFrequency);

        if ((divider - 1) != m_TimerChannel->PSC)
        {
            m_TimerChannel->PSC = divider - 1;
            if (m_IsRunning) {
                m_TimerChannel->EGR = TIM_EGR_UG_Msk;
            }
            m_TimerFrequency = m_TimerPerifFrequency / divider;
        }

        const uint32_t period = uint32_t(std::min(65535.0f, float(m_TimerFrequency) / speed));

        m_TimerChannel->ARR = period;
        m_TimerChannel->CCR1 = period / 2;
    }
    else
    {
        m_TimerChannel->ARR = 0; // Halt timer.
    }
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
    if (IsInterruptFlagged())
    {
        if (m_PinDirection) {
            m_Position++;
        } else {
            m_Position--;
        }
        if (m_WakeupOnFullStep && (m_Position & 0x0f) == 0) {
            m_RunningCondition.Wakeup(0);
        }
        if (m_MotionQueueCurrentCount != 0)
        {
            StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];
            if (node.m_AccelStepsLeft != 0)
            {
                --node.m_AccelStepsLeft;
                float acceleration = node.m_Acceleration;

                // Divide acceleration with the current speed to account for the current update frequency.
                if (m_CurrentSpeed < m_Jerk) {
                    acceleration /= m_Jerk;
                } else {
                    acceleration /= m_CurrentSpeed;
                }
                if (m_CurrentSpeed < node.m_CruiseSpeed)
                {
                    m_CurrentSpeed += acceleration;
                    if (m_CurrentSpeed > node.m_CruiseSpeed) {
                        m_CurrentSpeed = node.m_CruiseSpeed;
                    }
                }
                else
                {
                    m_CurrentSpeed -= acceleration;
                    if (m_CurrentSpeed < node.m_CruiseSpeed) {
                        m_CurrentSpeed = node.m_CruiseSpeed;
                    }
                }
                SetTimerSpeed(m_CurrentSpeed);
            }
            else if (node.m_CruiceStepsLeft != 0)
            {
                if (node.m_CruiceStepsLeft != std::numeric_limits<int32_t>::max()) {
                    --node.m_CruiceStepsLeft;
                }
            }
            else if (node.m_DecelStepsLeft != 0)
            {
                --node.m_DecelStepsLeft;
                float acceleration = node.m_Acceleration;
                if (m_CurrentSpeed < m_Jerk) {
                    acceleration /= m_Jerk;
                } else {
                    acceleration /= m_CurrentSpeed;
                }
                if (m_CurrentSpeed < node.m_EndSpeed)
                {
                    m_CurrentSpeed += acceleration;
                    if (m_CurrentSpeed > node.m_EndSpeed) {
                        m_CurrentSpeed = node.m_EndSpeed;
                    }
                }
                else
                {
                    m_CurrentSpeed -= acceleration;
                    if (m_CurrentSpeed < node.m_EndSpeed) {
                        m_CurrentSpeed = node.m_EndSpeed;
                    }
                }
                SetTimerSpeed(m_CurrentSpeed);
            }
            if (node.m_AccelStepsLeft == 0 && node.m_CruiceStepsLeft == 0 && node.m_DecelStepsLeft == 0)
            {
                m_MotionQueueCurrentNode = GetNextBlockIndex(m_MotionQueueCurrentNode);
                m_MotionQueueCurrentCount--;
                if (m_MotionQueueCurrentCount != 0) {
                    ActivateCurrentNode();
                } else {
                    StartStopTimer(false);
                }
            }
        }
        return IRQResult::HANDLED;
    }
    return IRQResult::UNHANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void StepperDriver::ActivateCurrentNode()
{
    if (!m_IsInitialized) return;

    StepperMotionNode& node = m_MotionQueue[m_MotionQueueCurrentNode];

    m_PinDirection = m_Reverse ? (!node.m_Direction) : node.m_Direction;
    m_CurrentSpeed = (node.m_AccelStepsLeft > 0) ? node.m_StartSpeed : node.m_CruiseSpeed;

    SetTimerSpeed(m_CurrentSpeed);
}


} // namespace kernel
