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
// Created: 21.10.2025 20:00

#include <DeviceControl/MultiMotor.h>

#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/Drivers/MultiMotorController/MultiMotorInode.h>
#include <Kernel/Drivers/MultiMotorController/TMC2209IODriver.h>
#include <Kernel/Drivers/MultiMotorController/TMC2209Driver.h>

namespace kernel
{



PREGISTER_KERNEL_DRIVER(MultiMotorInode, MultiMotorDriverParameters);


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MultiMotorInode::MultiMotorInode(const MultiMotorDriverParameters& parameters)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
    , m_Mutex("multimotor", PEMutexRecursionMode_RaiseError)
{
    m_MotorEnablePin = parameters.PinMotorEnable;
    m_MotorEnablePin.Write(true);
    m_MotorEnablePin.SetDirection(DigitalPinDirection_e::Out);

    m_ControlPort = ptr_new<TMC2209IODriver>();
    m_ControlPort->Setup(parameters.ControlPortPath, parameters.Baudrate);

#define MMI_REGISTER_HANDLER(NAME) m_DeviceControlDispatcher.AddHandler(&PMultiMotor::NAME, this, &MultiMotorInode::NAME)
    MMI_REGISTER_HANDLER(CreateMotor);
    MMI_REGISTER_HANDLER(DeleteMotor);
    MMI_REGISTER_HANDLER(EnableMotorsPower);
    MMI_REGISTER_HANDLER(IsMotorsPowerEnabled);
    MMI_REGISTER_HANDLER(SetJerk);
    MMI_REGISTER_HANDLER(SetReverse);
    MMI_REGISTER_HANDLER(GetReverse);
    MMI_REGISTER_HANDLER(SetStepsPerMillimeter);
    MMI_REGISTER_HANDLER(GetStepsPerMillimeter);
    MMI_REGISTER_HANDLER(SetWakeupOnFullStep);
    MMI_REGISTER_HANDLER(GetWakeupOnFullStep);
    MMI_REGISTER_HANDLER(SetSpeed);
    MMI_REGISTER_HANDLER(SetSpeedMultiMask);
    MMI_REGISTER_HANDLER(StopAtOffset);
    MMI_REGISTER_HANDLER(StopAtOffsetMultiMask);
    MMI_REGISTER_HANDLER(StopAtPos);
    MMI_REGISTER_HANDLER(SyncMove);
    MMI_REGISTER_HANDLER(QueueMotion);
    MMI_REGISTER_HANDLER(StepForward);
    MMI_REGISTER_HANDLER(StepBackward);
    MMI_REGISTER_HANDLER(EnableMotor);
    MMI_REGISTER_HANDLER(IsMotorEnabled);
    MMI_REGISTER_HANDLER(Wait);
    MMI_REGISTER_HANDLER(GetCurrentStopDistance);
    MMI_REGISTER_HANDLER(IsRunning);
    MMI_REGISTER_HANDLER(StartStopTimer);
    MMI_REGISTER_HANDLER(ClearMotion);
    MMI_REGISTER_HANDLER(GetStepPosition);
    MMI_REGISTER_HANDLER(GetPosition);
    MMI_REGISTER_HANDLER(ResetPosition);
    MMI_REGISTER_HANDLER(GetCurrentSpeed);
    MMI_REGISTER_HANDLER(GetCurrentDirection);
    MMI_REGISTER_HANDLER(SetCurrent);
    MMI_REGISTER_HANDLER(GetRunCurrent);
    MMI_REGISTER_HANDLER(SetRunCurrent);
    MMI_REGISTER_HANDLER(SetHoldCurrent);
    MMI_REGISTER_HANDLER(GetHoldCurrent);
    MMI_REGISTER_HANDLER(GetCurrentFadeTime);
    MMI_REGISTER_HANDLER(SetCurrentFadeTime);
    MMI_REGISTER_HANDLER(SetPowerDownTime);
    MMI_REGISTER_HANDLER(SetMicrosteps);
    MMI_REGISTER_HANDLER(GetStepTicks);
    MMI_REGISTER_HANDLER(GetStepTime);
    MMI_REGISTER_HANDLER(SetMaxStealthChopSpeed);
    MMI_REGISTER_HANDLER(SetMinStallGuardSpeed);
    MMI_REGISTER_HANDLER(SetStallGuardThreshold);
    MMI_REGISTER_HANDLER(GetStallGuardResult);
    MMI_REGISTER_HANDLER(SetHaltOnStall);
    MMI_REGISTER_HANDLER(ClearHaltedFlag);
    MMI_REGISTER_HANDLER(HasHalted);
    MMI_REGISTER_HANDLER(StartMultipleMotors);
    MMI_REGISTER_HANDLER(SyncStartMultipleMotors);
    MMI_REGISTER_HANDLER(WaitMultipleMotors);
    MMI_REGISTER_HANDLER(AddMotorToWaitGroup);
#undef MMI_REGISTER_HANDLER

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);

    m_DeviceControlDispatcher.Dispatch(request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    CRITICAL_SCOPE(m_Mutex);

    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

handle_id MultiMotorInode::CreateMotor(handle_id motorID, const TMC2209IOSetup& setup)
{
    m_Motors[motorID].Setup_trw(m_ControlPort, setup);
    return motorID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::DeleteMotor(handle_id motorID)
{
    auto motor = m_Motors.find(motorID);
    if (motor == m_Motors.end()) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    motor->second.Shutdown();
    m_Motors.erase(motor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetJerk(handle_id motorID, float jerk)
{
    GetMotor(motorID).SetJerk(jerk);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetReverse(handle_id motorID, bool reverse)
{
    GetMotor(motorID).SetReverse(reverse);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MultiMotorInode::GetReverse(handle_id motorID)
{
    return GetMotor(motorID).GetReverse();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void  MultiMotorInode::SetStepsPerMillimeter(handle_id motorID, float steps)
{
    GetMotor(motorID).SetStepsPerMillimeter(steps);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetStepsPerMillimeter(handle_id motorID)
{
    return GetMotor(motorID).GetStepsPerMillimeter();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void  MultiMotorInode::SetWakeupOnFullStep(handle_id motorID, bool value)
{
    GetMotor(motorID).SetWakeupOnFullStep(value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool  MultiMotorInode::GetWakeupOnFullStep(handle_id motorID)
{
    return GetMotor(motorID).GetWakeupOnFullStep();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetSpeed(handle_id motorID, float speedMMS, float accelerationMMS)
{
    GetMotor(motorID).SetSpeed(speedMMS, accelerationMMS);
}

void MultiMotorInode::SetSpeedMultiMask(uint32_t motorIDMask, float speedMMS, float accelerationMMS)
{
    const std::vector<TMC2209Driver*> motors = GetMotorsFromMask(motorIDMask);
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        for (TMC2209Driver* motor : motors) {
            motor->SetSpeed(speedMMS, accelerationMMS);
        }
    } CRITICAL_END;
}

void MultiMotorInode::StopAtOffset(handle_id motorID, float offset, float speed, float acceleration)
{
    GetMotor(motorID).StopAtOffset(offset, speed, acceleration);
}

void MultiMotorInode::StopAtOffsetMultiMask(uint32_t motorIDMask, float offset, float speed, float acceleration)
{
    const std::vector<TMC2209Driver*> motors = GetMotorsFromMask(motorIDMask);
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        for (TMC2209Driver* motor : motors) {
            motor->StopAtOffset(offset, speed, acceleration);
        }
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::StopAtPos(handle_id motorID, float position, float speed, float acceleration)
{
    GetMotor(motorID).StopAtPos(position, speed, acceleration);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SyncMove(handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS)
{
    GetMotor(motorID).SyncMove(distanceMM, speedMMS, accelerationMMS);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::QueueMotion(handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS)
{
    GetMotor(motorID).QueueMotion(distanceMM, speedMMS, accelerationMMS);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::StepForward(handle_id motorID)
{
    GetMotor(motorID).StepForward();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::StepBackward(handle_id motorID)
{
    GetMotor(motorID).StepBackward();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::EnableMotor(handle_id motorID, bool enable)
{
    GetMotor(motorID).EnableMotor(enable);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MultiMotorInode::IsMotorEnabled(handle_id motorID)
{
    return GetMotor(motorID).IsMotorEnabled();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::Wait(handle_id motorID)
{
    GetMotor(motorID).Wait();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetCurrentStopDistance(handle_id motorID, float acceleration)
{
    return GetMotor(motorID).GetCurrentStopDistance(acceleration);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MultiMotorInode::IsRunning(handle_id motorID)
{
    return GetMotor(motorID).IsRunning();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::StartStopTimer(handle_id motorID, bool doRun)
{
    GetMotor(motorID).StartStopTimer(doRun);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::ClearMotion(handle_id motorID)
{
    GetMotor(motorID).ClearMotion();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t MultiMotorInode::GetStepPosition(handle_id motorID)
{
    return GetMotor(motorID).GetStepPosition();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetPosition(handle_id motorID)
{
    return GetMotor(motorID).GetPosition();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::ResetPosition(handle_id motorID, float position)
{
    GetMotor(motorID).ResetPosition(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetCurrentSpeed(handle_id motorID)
{
    return GetMotor(motorID).GetCurrentSpeed();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MultiMotorInode::GetCurrentDirection(handle_id motorID)
{
    return GetMotor(motorID).GetCurrentDirection();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetCurrent(handle_id motorID, float holdCurrent, float runCurrent, TimeValMillis fadeTime)
{
    GetMotor(motorID).SetCurrent_trw(holdCurrent, runCurrent, fadeTime);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetRunCurrent(handle_id motorID) const
{
    return GetMotor(motorID).GetRunCurrent();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetRunCurrent(handle_id motorID, float current)
{
    GetMotor(motorID).SetRunCurrent_trw(current);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetHoldCurrent(handle_id motorID, float current)
{
    GetMotor(motorID).SetHoldCurrent_trw(current);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetHoldCurrent(handle_id motorID) const
{
    return GetMotor(motorID).GetHoldCurrent();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValMillis MultiMotorInode::GetCurrentFadeTime(handle_id motorID) const
{
    return GetMotor(motorID).GetCurrentFadeTime();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetCurrentFadeTime(handle_id motorID, TimeValMillis fadeTime)
{
    GetMotor(motorID).SetCurrentFadeTime_trw(fadeTime);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetPowerDownTime(handle_id motorID, TimeValMillis time)
{
    GetMotor(motorID).SetPowerDownTime_trw(time);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetMicrosteps(handle_id motorID, int32_t steps)
{
    GetMotor(motorID).SetMicrosteps_trw(steps);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t MultiMotorInode::GetStepTicks(handle_id motorID) const
{
    return GetMotor(motorID).GetStepTicks_trw();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetStepTime(handle_id motorID) const
{
    return GetMotor(motorID).GetStepTime_trw();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetMaxStealthChopSpeed(handle_id motorID, float maxSpeed)
{
    GetMotor(motorID).SetMaxStealthChopSpeed_trw(maxSpeed);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetMinStallGuardSpeed(handle_id motorID, float minSpeed)
{
    GetMotor(motorID).SetMinStallGuardSpeed_trw(minSpeed);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetStallGuardThreshold(handle_id motorID, float threshold)
{
    GetMotor(motorID).SetStallGuardThreshold_trw(threshold);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MultiMotorInode::GetStallGuardResult(handle_id motorID) const
{
    return GetMotor(motorID).GetStallGuardResult_trw();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SetHaltOnStall(handle_id motorID, bool doHalt)
{
    GetMotor(motorID).SetHaltOnStall(doHalt);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::ClearHaltedFlag(handle_id motorID)
{
    GetMotor(motorID).ClearHaltedFlag();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MultiMotorInode::HasHalted(handle_id motorID) const
{
    return GetMotor(motorID).HasHalted();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::StartMultipleMotors(uint32_t motorIDMask)
{
    const std::vector<TMC2209Driver*> motors = GetMotorsFromMask(motorIDMask);
    for (TMC2209Driver* motor : motors) {
        motor->StartStopTimer(true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::SyncStartMultipleMotors(bool doWait, uint32_t motorIDMask)
{
    const std::vector<TMC2209Driver*> motors = GetMotorsFromMask(motorIDMask);
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        for (TMC2209Driver* motor : motors) {
            motor->StartStopTimer(true);
        }
    } CRITICAL_END;
    if (doWait) {
        for (TMC2209Driver* motor : motors) {
            motor->Wait();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::WaitMultipleMotors(uint32_t motorIDMask)
{
    const std::vector<TMC2209Driver*> motors = GetMotorsFromMask(motorIDMask);
    for (TMC2209Driver* motor : motors) {
        motor->Wait();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MultiMotorInode::AddMotorToWaitGroup(handle_id motorID, handle_id waitGroupHandle)
{
    Ptr<KObjectWaitGroup> waitGroup = KNamedObject::GetObject_trw<KObjectWaitGroup>(waitGroupHandle);
    waitGroup->AddObject_trw(&GetMotor(motorID).GetRunningCondition());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TMC2209Driver& MultiMotorInode::GetMotor(handle_id motorID)
{
    auto motor = m_Motors.find(motorID);
    if (motor == m_Motors.end()) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    return motor->second;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<TMC2209Driver*> MultiMotorInode::GetMotorsFromMask(uint32_t motorIDMask)
{
    std::vector<TMC2209Driver*> motors;
    for (uint32_t index = 0, mask = 1; mask != 0; ++index, mask >>= 1)
    {
        if (mask & motorIDMask) {
            motors.push_back(&GetMotor(index));
        }
    }
    return motors;
}

} // namespace kernel
