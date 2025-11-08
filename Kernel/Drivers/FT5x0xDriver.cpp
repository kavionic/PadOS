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
#include <Kernel/KMessagePort.h>
#include <DeviceControl/I2C.h>
#include <DeviceControl/HID.h>
#include <System/ExceptionHandling.h>
#include <System/SystemMessageIDs.h>
#include <GUI/GUIEvent.h>

using namespace os;

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

    Start_trw(PThreadDetachState_Detached, 10);
        
    Ptr<KINode> inode = ptr_new<KINode>(nullptr, nullptr, this, false);
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
            printf(#NAME ": failed (%s)\n", strerror(std::to_underlying(result))); \
        } else { \
            printf(#NAME ": %d\n", reg); \
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
            printf("Log: '");
            for ( int i = reg; i > 0; --i) {
//                reg = FT5x0x_REG_LOG_CUR_CHAR;
//                kwrite(m_I2CDevice, 0, &reg, 1);
                if (kpread(m_I2CDevice, &reg, 1, FT5x0x_REG_LOG_CUR_CHAR) == PErrorCode::Success) {
                    printf("%c", reg);
                } else {
                    printf(".");
                }                
            }    
            printf("'\n");
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

                MessageID eventID = MessageID::NONE;

                switch(touchFlags)
                {
                    case FT5x0x_TOUCH_FLAGS_DOWN:    eventID = MessageID::MOUSE_DOWN; break;
                    case FT5x0x_TOUCH_FLAGS_UP:      eventID = MessageID::MOUSE_UP;   break;
                    case FT5x0x_TOUCH_FLAGS_CONTACT: eventID = MessageID::MOUSE_MOVE; break;
                }
                if (eventID != MessageID::NONE && touchID < MAX_POINTS)
                {
                    IPoint position(FT5x0x_TOUCH_XH_TOUCH_X(touch.TOUCH_XL, touch.TOUCH_XH), FT5x0x_TOUCH_YH_TOUCH_Y(touch.TOUCH_YL, touch.TOUCH_YH));
                    if (eventID != MessageID::MOUSE_MOVE || position != m_TouchPositions[touchID])
                    {
                        m_TouchPositions[touchID] = position;
                        
                        MotionEvent mouseEvent;
                        mouseEvent.Timestamp = kget_monotonic_time();
                        mouseEvent.EventID   = eventID;
                        mouseEvent.ToolType  = MotionToolType::Finger;
                        mouseEvent.ButtonID  = MouseButton_e(int(MouseButton_e::FirstTouchID) + touchID);
                        mouseEvent.Position  = Point(position);

//                        printf("Mouse event %d: %d/%d\n", eventID - MessageID::MOUSE_DOWN, position.x, position.y);
                        CRITICAL_BEGIN(m_Mutex)
                        {
                            for (auto file : m_OpenFiles)
                            {
                                if (file->m_TargetPort != -1)
                                {
                                    try {
                                        kmessage_port_send_trw(file->m_TargetPort, -1, int32_t(eventID), &mouseEvent, sizeof(mouseEvent));
                                    }
                                    catch (const std::exception& exc) {
                                        printf("FT5x0xDriver: failed to send event: %s\n", exc.what());
                                    }
                                }
                            }
                        } CRITICAL_END;                            
                        
                    }
                }
            }            
        }
        else
        {
            printf("ERROR: FT5x0xDriver::Run() read error. Resetting touch device.\n");
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

Ptr<KFileNode> FT5x0xDriver::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> inode, int flags)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<FT5x0xFile> file = ptr_new<FT5x0xFile>(flags);
    m_OpenFiles.push_back(ptr_raw_pointer_cast(file));
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FT5x0xDriver::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    CRITICAL_SCOPE(m_Mutex);
    auto i = std::find(m_OpenFiles.begin(), m_OpenFiles.end(), file);
    if (i != m_OpenFiles.end())
    {
        m_OpenFiles.erase(i);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FT5x0xDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<FT5x0xFile> ftFile = ptr_static_cast<FT5x0xFile>(file);
    
    port_id* inArg  = (int*)inData;
    port_id* outArg = (int*)outData;
    
    switch(request)
    {
        case HIDIOCTL_SET_TARGET_PORT:
            if (inArg == nullptr || inDataLength != sizeof(port_id)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            ftFile->m_TargetPort = *inArg;
            return;
        case HIDIOCTL_GET_TARGET_PORT:
            if (outArg == nullptr || outDataLength != sizeof(port_id)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            *outArg = ftFile->m_TargetPort;
            return;
        default:
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
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
