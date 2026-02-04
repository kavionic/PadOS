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

#pragma once
#include <Kernel/VFS/KInode.h>
#include <RPC/RPCDispatcher.h>

struct TMC2209IOSetup;
struct MultiMotorDriverParameters;

namespace kernel
{

class MultiMotorDriver;
class TMC2209Driver;
class TMC2209IODriver;

class MultiMotorInode : public KInode, public KFilesystemFileOps
{
public:
    MultiMotorInode(const MultiMotorDriverParameters& parameters);

    virtual void DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual void ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

    void        SetupControllerIO(const char* controlPortPath, uint32_t baudrate);
    handle_id   CreateMotor(handle_id motorID, const TMC2209IOSetup& setup);
    void        DeleteMotor(handle_id motorID);
    void        EnableMotorsPower(bool enable) { m_MotorEnablePin = !enable; }
    bool        IsMotorsPowerEnabled() const { return !m_MotorEnablePin.Read(); }
    void SetJerk(handle_id motorID, float jerk);
    void SetReverse(handle_id motorID, bool reverse);
    bool GetReverse(handle_id motorID);

    void  SetStepsPerMillimeter(handle_id motorID, float steps);
    float GetStepsPerMillimeter(handle_id motorID);
    void  SetWakeupOnFullStep(handle_id motorID, bool value);
    bool  GetWakeupOnFullStep(handle_id motorID);

    void SetSpeed(handle_id motorID, float speedMMS, float accelerationMMS);
    void SetSpeedMultiMask(uint32_t motorIDMask, float speedMMS, float accelerationMMS);
    void StopAtOffset(handle_id motorID, float offset, float speed, float acceleration);
    void StopAtOffsetMultiMask(uint32_t motorIDMask, float offset, float speed, float acceleration);
    void StopAtPos(handle_id motorID, float position, float speed, float acceleration);
    void SyncMove(handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS);
    void QueueMotion(handle_id motorID, float distanceMM, float speedMMS, float accelerationMMS);
    void StepForward(handle_id motorID);
    void StepBackward(handle_id motorID);
    void EnableMotor(handle_id motorID, bool enable);
    bool IsMotorEnabled(handle_id motorID);
    void Wait(handle_id motorID);
    float GetCurrentStopDistance(handle_id motorID, float acceleration);

    bool IsRunning(handle_id motorID);
    void StartStopTimer(handle_id motorID, bool doRun);
    void ClearMotion(handle_id motorID);

    int32_t GetStepPosition(handle_id motorID);
    float   GetPosition(handle_id motorID);
    void    ResetPosition(handle_id motorID, float position);
    float   GetCurrentSpeed(handle_id motorID);
    bool    GetCurrentDirection(handle_id motorID);

    void            SetCurrent(handle_id motorID, float holdCurrent, float runCurrent, TimeValMillis fadeTime);
    float           GetRunCurrent(handle_id motorID) const;
    void            SetRunCurrent(handle_id motorID, float current);
    void            SetHoldCurrent(handle_id motorID, float current);
    float           GetHoldCurrent(handle_id motorID) const;
    TimeValMillis   GetCurrentFadeTime(handle_id motorID) const;
    void            SetCurrentFadeTime(handle_id motorID, TimeValMillis fadeTime);
    void            SetPowerDownTime(handle_id motorID, TimeValMillis time);
    void            SetMicrosteps(handle_id motorID, int32_t steps);
    uint32_t        GetStepTicks(handle_id motorID) const;
    float           GetStepTime(handle_id motorID) const;
    void            SetMaxStealthChopSpeed(handle_id motorID, float maxSpeed);
    void            SetMinStallGuardSpeed(handle_id motorID, float minSpeed);
    void            SetStallGuardThreshold(handle_id motorID, float threshold);
    float           GetStallGuardResult(handle_id motorID) const;
    void            SetHaltOnStall(handle_id motorID, bool doHalt);
    void            ClearHaltedFlag(handle_id motorID);
    bool            HasHalted(handle_id motorID) const;
    void            StartMultipleMotors(uint32_t motorIDMask);
    void            SyncStartMultipleMotors(bool doWait, uint32_t motorIDMask);
    void            WaitMultipleMotors(uint32_t motorIDMask);
    void            AddMotorToWaitGroup(handle_id motorID, handle_id waitGroupHandle);

private:
    TMC2209Driver& GetMotor(handle_id motorID);
    const TMC2209Driver& GetMotor(handle_id motorID) const { return const_cast<MultiMotorInode*>(this)->GetMotor(motorID); }
    std::vector<TMC2209Driver*> GetMotorsFromMask(uint32_t motorIDMask);

    KMutex               m_Mutex;

    DigitalPin           m_MotorEnablePin;
    Ptr<TMC2209IODriver> m_ControlPort;

    PRPCDispatcher m_DeviceControlDispatcher;
    std::map<int, TMC2209Driver> m_Motors;
};

} // namespace kernel
