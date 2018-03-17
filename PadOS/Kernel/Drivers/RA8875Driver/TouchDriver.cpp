// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.01.2014 08:52:13

 #include "sam.h"

#include <string.h>
#include <fcntl.h>

#include "TouchDriver.h"

#include "Kernel/Kernel.h"
#include "DeviceControl/I2C.h"
#include "SystemSetup.h"
#include "Kernel/HAL/SAME70System.h"
#include "Kernel/SpinTimer.h"
#include "System/System.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

TouchDriver::TouchDriver(const DigitalPin& pinWAKE, const DigitalPin& pinRESET, const DigitalPin& pinINT) : m_PinWAKE(pinWAKE), m_PinRESET(pinRESET), m_PinINT(pinINT)
{
    m_CursorX = 0;
    m_CursorY = 0;        
    m_CursorMoved = false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TouchDriver::Initialize(const char* i2cPath)
{
    m_I2CDevice = open(i2cPath, O_RDWR);

    I2CIOCTL_SetSlaveAddress(m_I2CDevice, 0x38);
    
    m_PinWAKE.Write(true);
    m_PinRESET.Write(true);
    m_PinWAKE.SetDirection(DigitalPinDirection_e::Out);    
    m_PinRESET.SetDirection(DigitalPinDirection_e::Out);
   
    m_PinRESET.Write(false);
    SpinTimer::SleepMS(10);
    m_PinRESET.Write(true);
    
    SAME70System::EnablePeripheralClock(ID_TWIHS2);
    
    DigitalPort::SetPeripheralMux(LCD_TP_SDA_port, LCD_TP_SDA_bm, LCD_TP_SDA_perif);
    DigitalPort::SetPeripheralMux(LCD_TP_SCL_port, LCD_TP_SCL_bm, LCD_TP_SCL_perif);

    m_PinINT.SetInterruptMode(PinInterruptMode_e::FallingEdge);
    m_PinINT.GetInterruptStatus(); // Clear any pending interrupts.
    m_PinINT.EnableInterrupts();
    
    NVIC_ClearPendingIRQ(PIOA_IRQn);
    NVIC_SetPriority(PIOA_IRQn, 0);

    kernel::Kernel::RegisterIRQHandler(PIOA_IRQn, IRQHandler, this);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TouchDriver::HandleIRQ()
{
    if (m_PinINT.GetInterruptStatus())
    {
        if (m_State == State_e::Idle)
        {
            m_State = State_e::TouchRegistered;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

/*bool TouchDriver::GetCursorPos( int16_t* x, int16_t* y )
{
    if ( IsPressed() )
    {
        *x = m_CursorX;
        *y = m_CursorY;
        return true;
    }
    else
    {
        *x = m_CursorX;
        *y = m_CursorY;
        return false;
    }
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TouchDriver::Tick()
{
    if (m_I2CDevice != -1)
    {
        if (m_State == State_e::TouchRegistered)
        {
            int8_t pos = m_RegisterBufferHandler.PeekWritePos();
            if (pos != -1)
            {
                m_LastUpdateTime = get_system_time();
                m_State = State_e::ReceivingTouchData;
                kernel::Kernel::ReadAsync(m_I2CDevice, 0, &m_Registers[pos], sizeof(FT5x0xOMRegisters), this, TransactionCallback);
            }
        }
        int8_t pos = m_RegisterBufferHandler.PeekReadPos();
        if (pos != -1)
        {
            const FT5x0xOMRegisters& registers = m_Registers[pos];
            
            //printf("TP: parse data:\n");
            //int pointCount = FT5x0x_TD_STATUS_PT_COUNT(registers.TD_STATUS);
            for (int i = 0; i < FT5x0x_TP_REGISTER_COUNT; ++i)
            {
                const FT5x0xOMTouchData& touch = registers.TOUCH_DATA[i];
                int touchID = FT5x0x_TOUCH_YH_TOUCH_ID(touch.TOUCH_YH);
                int touchFlags = FT5x0x_TOUCH_XH_TOUCH_FLAGS(touch.TOUCH_XH);

                EventID_e eventID = EventID_e::None;
                switch(touchFlags)
                {
                    case FT5x0x_TOUCH_FLAGS_DOWN:    eventID = EventID_e::Down; break;
                    case FT5x0x_TOUCH_FLAGS_UP:      eventID = EventID_e::Up;   break;
                    case FT5x0x_TOUCH_FLAGS_CONTACT: eventID = EventID_e::Move; break;
                }
                if (eventID != EventID_e::None && touchID < MAX_POINTS)
                {
                    IPoint position(FT5x0x_TOUCH_XH_TOUCH_X(touch.TOUCH_XL, touch.TOUCH_XH), FT5x0x_TOUCH_YH_TOUCH_Y(touch.TOUCH_YL, touch.TOUCH_YH));
                    if (eventID != EventID_e::Move || position != m_TouchPositions[touchID])
                    {
                        m_TouchPositions[touchID] = position;
                    
                        //printf("  TP: %d : %d, %d/%d\n", int(eventID), touchID, position.x, position.y);
                        SignalTouchEvent(eventID, touchID, position);
                    }
                }
                
/*                if (touchID == 0)
                {
                    m_CursorMoved = true;
                    m_CursorX = FT5x0x_TOUCH_XH_TOUCH_X(touch.TOUCH_XL, touch.TOUCH_XH);
                    m_CursorY = FT5x0x_TOUCH_YH_TOUCH_Y(touch.TOUCH_YL, touch.TOUCH_YH);
                    if (touchFlags == FT5x0x_TOUCH_FLAGS_DOWN)
                        m_IsPressed = true;
                    else if (touchFlags == FT5x0x_TOUCH_FLAGS_UP)
                        m_IsPressed = false;
                }*/
            }
            m_RegisterBufferHandler.GetReadPos();
        }
        else
        {
            if (m_State != State_e::Idle)
            {
                bigtime_t time = get_system_time();
                if ((time - m_LastUpdateTime) > 100000)
                {
                    printf("TP: request timed out\n");
                    m_State = State_e::Idle;
                    m_PinRESET.Write(false);
                    SpinTimer::SleepMS(10);
                    m_PinRESET.Write(true);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

/*bool TouchDriver::GetAndClearCursorMoved()
{
    bool didMove = m_CursorMoved;
    m_CursorMoved = false;
    return didMove;
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TouchDriver::TransactionCallback(void* userObject, void* data, ssize_t length)
{
    static_cast<TouchDriver*>(userObject)->TransactionCallback(data, length);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void TouchDriver::TransactionCallback(void* data, ssize_t length)
{
    //printf("TP: recv\n");

    if (length == sizeof(FT5x0xOMRegisters))
    {
        m_RegisterBufferHandler.GetWritePos();
        m_State = State_e::Idle;        
    }
}

