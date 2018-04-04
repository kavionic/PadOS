// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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

#include "sam.h"

#include <string.h>
#include <fcntl.h>

#include "FT5x0xDriver.h"
#include "DeviceControl/I2C.h"
#include "DeviceControl/FT5x0x.h"
#include "System/Threads.h"
#include "System/SystemMessageIDs.h"
#include "System/GUI/GUIEvent.h"


using namespace kernel;
using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FT5x0xDriver::FT5x0xDriver(const DigitalPin& pinWAKE, const DigitalPin& pinRESET, const DigitalPin& pinINT, const char* i2cPath) : Thread("ft5x0x_driver"), m_PinWAKE(pinWAKE), m_PinRESET(pinRESET), m_PinINT(pinINT), m_Mutex("ft5x0x_mutex", 1, true), m_EventSemaphore("ft5x0x_events", 0, false)
{
    SetDeleteOnExit(false);
    
    m_I2CDevice = Kernel::OpenFile(i2cPath, O_RDWR);

    if (m_I2CDevice >= 0)
    {
        I2CIOCTL_SetSlaveAddress(m_I2CDevice, 0x38);
        I2CIOCTL_SetInternalAddrLen(m_I2CDevice, 1);

        m_PinWAKE.Write(true);
        m_PinRESET.Write(false);
        m_PinRESET.SetDirection(DigitalPinDirection_e::Out);
        m_PinWAKE.SetDirection(DigitalPinDirection_e::Out);
    
//        m_PinRESET.Write(false);
        snooze(bigtime_from_ms(200));
        m_PinRESET.Write(true);
        snooze(bigtime_from_ms(300));
        m_PinWAKE.Write(false);
        snooze(bigtime_from_ms(200));
        m_PinWAKE.Write(true);
        snooze(bigtime_from_ms(200));
//        m_PinRESET.SetDirection(DigitalPinDirection_e::In);

        m_PinINT.SetInterruptMode(PinInterruptMode_e::FallingEdge);
        m_PinINT.GetInterruptStatus(); // Clear any pending interrupts.
        m_PinINT.EnableInterrupts();

        
        Kernel::RegisterIRQHandler(PIOA_IRQn, IRQHandler, this);

        uint8_t reg = 0;
        Kernel::Write(m_I2CDevice, 0, &reg, 1);
//        reg = 3;
//        Kernel::Write(m_I2CDevice, FT5x0x_REG_G_PERIODE_ACTIVE, &reg, 1);
/*        for (;;)
        {
            if (Kernel::Read(m_I2CDevice, FT5x0x_REG_G_PERIODE_ACTIVE, &reg, 1) != 1) {
                snooze(bigtime_from_s(5));
            }
            snooze(bigtime_from_ms(100));
        }*/
        PrintChipStatus();
//        Kernel::Write(m_I2CDevice, )

        Start(true, 10);
    }
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

void FT5x0xDriver::PrintChipStatus()
{
        uint8_t reg;

#define PRINT_REG(NAME) \
        /*reg = FT5x0x_REG_##NAME;*/ \
        /*Kernel::Write(m_I2CDevice, 0, &reg, 1);*/ \
        if (Kernel::Read(m_I2CDevice, FT5x0x_REG_##NAME, &reg, 1) == 1) { \
            printf(#NAME ": %d\n", reg); \
        } else { \
            printf(#NAME ": failed (%s)\n", strerror(get_last_error())); \
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
//                Kernel::Write(m_I2CDevice, 0, &reg, 1);
                if (Kernel::Read(m_I2CDevice, FT5x0x_REG_LOG_CUR_CHAR, &reg, 1) == 1) {
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

int FT5x0xDriver::Run()
{
    for(;;)
    {
        m_EventSemaphore.Acquire();
        
        FT5x0xOMRegisters registers;

        ssize_t length = kernel::Kernel::Read(m_I2CDevice, 0, &registers, sizeof(FT5x0xOMRegisters) - 2);
        
        if (length == (sizeof(FT5x0xOMRegisters) - 2))
        {
            for (int i = 0; i < FT5x0x_TP_REGISTER_COUNT; ++i)
            {
                const FT5x0xOMTouchData& touch = registers.TOUCH_DATA[i];
                int touchID = FT5x0x_TOUCH_YH_TOUCH_ID(touch.TOUCH_YH);
                int touchFlags = FT5x0x_TOUCH_XH_TOUCH_FLAGS(touch.TOUCH_XH);

                int eventID = 0;

                switch(touchFlags)
                {
                    case FT5x0x_TOUCH_FLAGS_DOWN:    eventID = MessageID::MOUSE_DOWN; break;
                    case FT5x0x_TOUCH_FLAGS_UP:      eventID = MessageID::MOUSE_UP;   break;
                    case FT5x0x_TOUCH_FLAGS_CONTACT: eventID = MessageID::MOUSE_MOVE; break;
                }
                if (eventID != 0 && touchID < MAX_POINTS)
                {
                    IPoint position(FT5x0x_TOUCH_XH_TOUCH_X(touch.TOUCH_XL, touch.TOUCH_XH), FT5x0x_TOUCH_YH_TOUCH_Y(touch.TOUCH_YL, touch.TOUCH_YH));
                    if (eventID != MessageID::MOUSE_MOVE || position != m_TouchPositions[touchID])
                    {
                        m_TouchPositions[touchID] = position;
                        
                        MsgMouseEvent mouseEvent;
                        mouseEvent.ButtonID = MouseButton_e(int(MouseButton_e::FirstTouchID) + touchID);
                        mouseEvent.Position = Point(position);

//                        printf("Mouse event %d: %d/%d\n", eventID - MessageID::MOUSE_DOWN, position.x, position.y);
                        CRITICAL_BEGIN(m_Mutex)
                        {
                            for (auto file : m_OpenFiles)
                            {
                                if (file->m_TargetPort != -1) {
                                    send_message(file->m_TargetPort, -1, eventID, &mouseEvent, sizeof(mouseEvent));
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
            snooze(bigtime_from_ms(5));
            m_PinRESET.Write(true);
            snooze(bigtime_from_ms(300));
            m_EventSemaphore.SetCount(1);
        }        
    }
    return 0; 
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileHandle> FT5x0xDriver::Open( int flags)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<FT5x0xFile> file = ptr_new<FT5x0xFile>();
    m_OpenFiles.push_back(file);
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FT5x0xDriver::Close(Ptr<KFileHandle> file)
{
    CRITICAL_SCOPE(m_Mutex);
    auto i = std::find(m_OpenFiles.begin(), m_OpenFiles.end(), file);
    if (i != m_OpenFiles.end())
    {
        m_OpenFiles.erase(i);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FT5x0xDriver::DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<FT5x0xFile> ftFile = ptr_static_cast<FT5x0xFile>(file);
    
    port_id* inArg  = (int*)inData;
    port_id* outArg = (int*)outData;
    
    switch(request)
    {
        case FT5x0xIOCTL_SET_TARGET_PORT:
            if (inArg == nullptr || inDataLength != sizeof(port_id)) { set_last_error(EINVAL); return -1; }
            ftFile->m_TargetPort = *inArg;
            return 0;
        case FT5x0xIOCTL_GET_TARGET_PORT:
            if (outArg == nullptr || outDataLength != sizeof(port_id)) { set_last_error(EINVAL); return -1; }
            *outArg = ftFile->m_TargetPort;
            return 0;
        default:
            set_last_error(EINVAL);
            return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FT5x0xDriver::HandleIRQ()
{
    if (m_PinINT.GetInterruptStatus())
    {
        m_EventSemaphore.Release();
    }
}
