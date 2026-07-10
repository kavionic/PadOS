// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 19.03.2018 21:30:24

#include "System/Platform.h"

#include <string.h>
#include <fcntl.h>

#include <Kernel/KTime.h>
#include <Kernel/Drivers/FT5x0xDriver.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/UserInput/UserInputManager.h>
#include <DeviceControl/I2C.h>
#include <System/ExceptionHandling.h>
#include <GUI/GUIEvent.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FT5x0xDriver::FT5x0xDriver() : KThread("ft5x0x_driver"), m_Mutex("ft5x0x_mutex", PEMutexRecursionMode_RaiseError), m_EventSemaphore("ft5x0x_events", CLOCK_MONOTONIC_COARSE, 0)
{
    SetDeleteOnExit(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FT5x0xDriver::~FT5x0xDriver()
{
    if (m_SourceID != -1) {
        KUserInputManager::Get().RemoveSource(m_SourceID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FT5x0xDriver::Setup(const char* devicePath, const DigitalPin& pinWAKE, const DigitalPin& pinRESET, const DigitalPin& pinINT, IRQn_Type irqNum, const char* i2cPath)
{
    m_PinWAKE  = pinWAKE;
    m_PinRESET = pinRESET;
    m_PinINT   = pinINT;
    
	m_PinINT.SetDirection(DigitalPinDirection_e::In);

    m_I2CDevice = kopen_trw(i2cPath, O_RDWR);

    I2CIOCTL_SetTimeout(m_I2CDevice, TimeValNanos::FromMilliseconds(100));
	I2CIOCTL_SetSlaveAddress(m_I2CDevice, 0x38);
    I2CIOCTL_SetInternalAddrLen(m_I2CDevice, 1);

    m_PinWAKE.Write(true);
    m_PinRESET.SetDirection(DigitalPinDirection_e::Out);
    m_PinRESET.Write(false);
    m_PinWAKE.SetDirection(DigitalPinDirection_e::Out);
    
    snooze_ms(200);
    m_PinRESET.Write(true);
	snooze_ms(300);
    m_PinWAKE.Write(false);
	snooze_ms(200);
    m_PinWAKE.Write(true);
	snooze_ms(200);

	m_PinINT.SetInterruptMode(PinInterruptMode_e::FallingEdge);
    m_PinINT.GetAndClearInterruptStatus(); // Clear any pending interrupts.
    m_PinINT.EnableInterrupts();
        
    register_irq_handler(irqNum, IRQHandler, this);

    uint8_t reg = 0;
    kpwrite(m_I2CDevice, &reg, 1, 0);
//        reg = 3;
//        kwrite(m_I2CDevice, FT5x0x_REG_G_PERIODE_ACTIVE, &reg, 1);
/*        for (;;)
    {
        if (kpread(m_I2CDevice, FT5x0x_REG_G_PERIODE_ACTIVE, &reg, 1) != 1) {
            snooze(bigtime_from_s(5));
        }
        snooze_ms(100);
    }*/
    PrintChipStatus();
//        kwrite(m_I2CDevice, )

    if (m_SourceID == -1) {
        m_SourceID = KUserInputManager::Get().AddSource(PInputClass::TouchScreen);
    }

    Start_trw(KSpawnThreadFlag::None, PThreadDetachState_Detached, 10);

    Ptr<KInode> inode = ptr_new<KInode>(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    kregister_device_root_trw(devicePath, inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FT5x0xDriver::PrintChipStatus()
{
        uint8_t reg;

#define PRINT_REG(NAME) \
        /*reg = FT5x0x_REG_##NAME;*/ \
        /*kwrite(m_I2CDevice, 0, &reg, 1);*/ \
        if (const PErrorCode result = kpread(m_I2CDevice, &reg, 1, FT5x0x_REG_##NAME); result != PErrorCode::Success) { \
            p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, #NAME ": failed ({})", strerror(std::to_underlying(result))); \
        } else { \
            p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, #NAME ": {}", reg); \
        }
        
        PRINT_REG(G_ERR);
        PRINT_REG(G_STATE);
        PRINT_REG(G_CTRL);
        PRINT_REG(G_TIME_ENTER_MONITOR);
        PRINT_REG(G_PERIODE_ACTIVE);
        PRINT_REG(G_PERIODE_MONITOR);
        PRINT_REG(G_MODE);
        PRINT_REG(G_PMODE);
        PRINT_REG(G_FIRMWARE_ID);
        PRINT_REG(G_CLB);
        PRINT_REG(LOG_MSG_CNT);

        
        if (reg > 0)
        {
            PString log;
            for ( int i = reg; i > 0; --i) {
//                reg = FT5x0x_REG_LOG_CUR_CHAR;
//                kwrite(m_I2CDevice, 0, &reg, 1);
                if (kpread(m_I2CDevice, &reg, 1, FT5x0x_REG_LOG_CUR_CHAR) == PErrorCode::Success) {
                    log += reg;
                } else {
                    log += '.';
                }                
            }    
            p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "Log: '{}'", log);
       }            
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* FT5x0xDriver::Run()
{
    for(;;)
    {
        m_EventSemaphore.Acquire();
        
        FT5x0xOMRegisters registers;

        const PErrorCode result = kpread(m_I2CDevice, &registers, sizeof(FT5x0xOMRegisters) - 2, 0);
        
        if (result == PErrorCode::Success)
        {
            for (int i = 0; i < FT5x0x_TP_REGISTER_COUNT; ++i)
            {
                const FT5x0xOMTouchData& touch = registers.TOUCH_DATA[i];
                int touchID = FT5x0x_TOUCH_YH_TOUCH_ID(touch.TOUCH_YH);
                int touchFlags = FT5x0x_TOUCH_XH_TOUCH_FLAGS(touch.TOUCH_XH);

                PInputEventID eventID = PInputEventID::TouchMove;
                bool hasEvent = true;

                switch(touchFlags)
                {
                    case FT5x0x_TOUCH_FLAGS_DOWN:
                        eventID = PInputEventID::TouchDown;
                        break;
                    case FT5x0x_TOUCH_FLAGS_UP:
                        eventID = PInputEventID::TouchUp;
                        break;
                    case FT5x0x_TOUCH_FLAGS_CONTACT:
                        eventID = PInputEventID::TouchMove;
                        break;
                    default:
                        hasEvent = false;
                        break;
                }
                if (hasEvent && touchID < MAX_POINTS)
                {
                    PIPoint position(FT5x0x_TOUCH_XH_TOUCH_X(touch.TOUCH_XL, touch.TOUCH_XH), FT5x0x_TOUCH_YH_TOUCH_Y(touch.TOUCH_YL, touch.TOUCH_YH));
                    if (eventID != PInputEventID::TouchMove || position != m_TouchPositions[touchID])
                    {
                        m_TouchPositions[touchID] = position;
                        
                        PTouchEvent touchEvent;
                        touchEvent.EventSize = sizeof(touchEvent);
                        touchEvent.EventType = PInputEventType::TouchEvent;
                        touchEvent.ClassID   = PInputClass::TouchScreen;
                        touchEvent.Timestamp = kget_monotonic_time();
                        touchEvent.EventID   = eventID;
                        touchEvent.SourceID  = m_SourceID;
                        touchEvent.TouchID   = uint32_t(touchID);
                        touchEvent.ToolType  = PMotionToolType::Finger;
                        touchEvent.Pressure  = (eventID != PInputEventID::TouchUp) ? 1.0f : 0.0f;
                        touchEvent.Position  = PPoint(position);

//                        p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "Mouse event {}: {}/{}", eventID, position.x, position.y);
                        try {
                            KUserInputManager::Get().AddEvent(touchEvent);
                        }
                        catch (const std::exception& exc) {
                            p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "FT5x0xDriver: failed to queue event: {}", exc.what());
                        }
                        
                    }
                }
            }            
        }
        else
        {
            p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "FT5x0xDriver::Run() read error. Resetting touch device.");
            m_PinRESET.Write(false);
			snooze_ms(5);
            m_PinRESET.Write(true);
			snooze_ms(300);
            m_EventSemaphore.SetCount(1);
        }        
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FT5x0xDriver::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult FT5x0xDriver::HandleIRQ()
{
	if (m_PinINT.GetAndClearInterruptStatus())
	{
		m_EventSemaphore.Release();
		return IRQResult::HANDLED;
	}
	return IRQResult::UNHANDLED;
}

} // namespace kernel
