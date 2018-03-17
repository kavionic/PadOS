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
// Created: 23.01.2014 08:51:55

 #pragma once

#include <stdint.h>

#include "Kernel/HAL/DigitalPort.h"
#include "System/Math/Point.h"
#include "System/Signals/Signal.h"
#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"

namespace kernel
{


template<typename T, int QUEUE_SIZE> class RingBufferHandler
{
public:
    static const T QUEUE_SIZE_MASK = QUEUE_SIZE - 1;

    RingBufferHandler() {
        m_QueueInPos = -1;
        m_QueueOutPos = 0;    
    }

    T GetWritePos()
    {
        T newPos = -1;
        {
            CRITICAL_BEGIN(CRITICAL_IRQ)
            {
                if ( m_QueueInPos != -1 )
                {
                    if ( m_QueueInPos != m_QueueOutPos )
                    {
                        newPos = m_QueueInPos;
                        m_QueueInPos = (m_QueueInPos + 1) & QUEUE_SIZE_MASK;
                    }
                }
                else
                {
                    newPos = m_QueueOutPos;
                    m_QueueInPos = (m_QueueOutPos + 1) & QUEUE_SIZE_MASK;
                }
            } CRITICAL_END;
        }
        return newPos;
    }
            
    T GetReadPos()
    {
        T newPos = -1;
        {
            CRITICAL_BEGIN(CRITICAL_IRQ)
            {
                if ( m_QueueInPos != -1 )
                {
                    newPos = m_QueueOutPos;
                    m_QueueOutPos = (m_QueueOutPos + 1) & QUEUE_SIZE_MASK;
                    if ( m_QueueInPos == m_QueueOutPos )
                    {
                        m_QueueInPos = -1; // Queue empty
                    }
                }
            } CRITICAL_END;
        }
        return newPos;
    }

    T PeekReadPos()
    {
        T pos;

        CRITICAL_BEGIN(CRITICAL_IRQ) {
                pos = (m_QueueInPos != -1) ? T(m_QueueOutPos) : -1;
        } CRITICAL_END;

        return pos;
    }
    T PeekWritePos()
    {
        T pos;
        CRITICAL_BEGIN(CRITICAL_IRQ) {
            pos = (m_QueueInPos != -1) ? m_QueueInPos : m_QueueOutPos;
        } CRITICAL_END;
        return pos;
    }

    std::atomic<T> m_QueueInPos;
    std::atomic<T> m_QueueOutPos;
        
};

struct FT5x0xOMTouchData
{
    uint8_t TOUCH_XH;
    uint8_t TOUCH_XL;
    uint8_t TOUCH_YH;
    uint8_t TOUCH_YL;
    uint8_t reserved[2];
};

#define FT5x0x_TP_REGISTER_COUNT   5

struct FT5x0xOMRegisters
{
    uint8_t DEVICE_MODE;
    uint8_t GEST_ID;
    uint8_t TD_STATUS;
    FT5x0xOMTouchData TOUCH_DATA[FT5x0x_TP_REGISTER_COUNT];
};

#define FT5x0x_DEVICE_MODE_MODE_bp   4
#define FT5x0x_DEVICE_MODE_MODE_bm   BIT8(FT5x0x_DEVICE_MODE_MODE_bp, 0x7)
#define FT5x0x_DEVICE_MODE_MODE(reg) (((reg) >> FT5x0x_DEVICE_MODE_MODE_bp) & FT5x0x_DEVICE_MODE_MODE_bm)

#define FT5x0x_DEVICE_MODE_MODE_NORMAL  0x00
#define FT5x0x_DEVICE_MODE_MODE_SYSINFO 0x01
#define FT5x0x_DEVICE_MODE_MODE_TEST    0x04

#define FT5x0x_GEST_ID_NO_GESTURE 0x00
#define FT5x0x_GEST_ID_MOVE_UP    0x10
#define FT5x0x_GEST_ID_MOVE_LEFT  0x14
#define FT5x0x_GEST_ID_MOVE_DOWN  0x18
#define FT5x0x_GEST_ID_MOVE_RIGHT 0x1c
#define FT5x0x_GEST_ID_ZOOM_IN    0x48
#define FT5x0x_GEST_ID_ZOOM_OUT   0x49


#define FT5x0x_TD_STATUS_PT_COUNT_bp   0
#define FT5x0x_TD_STATUS_PT_COUNT_bm   BIT8(FT5x0x_TD_STATUS_PT_COUNT_bp, 0xf)
#define FT5x0x_TD_STATUS_PT_COUNT(reg) (((reg) & FT5x0x_TD_STATUS_PT_COUNT_bm) >> FT5x0x_TD_STATUS_PT_COUNT_bp)

#define FT5x0x_TOUCH_XH_TOUCH_XH_bp   0
#define FT5x0x_TOUCH_XH_TOUCH_XH_bm   BIT8(FT5x0x_TOUCH_XH_TOUCH_XH_bp, 0xf)
#define FT5x0x_TOUCH_XH_TOUCH_XH(reg) (((reg) & FT5x0x_TOUCH_XH_TOUCH_XH_bm) >> FT5x0x_TOUCH_XH_TOUCH_XH_bp)

#define FT5x0x_TOUCH_XH_TOUCH_X(regL, regH) (regL | (FT5x0x_TOUCH_XH_TOUCH_XH(regH) << 8))

#define FT5x0x_TOUCH_XH_TOUCH_FLAGS_bp   6
#define FT5x0x_TOUCH_XH_TOUCH_FLAGS_bm   BIT8(FT5x0x_TOUCH_XH_TOUCH_FLAGS_bp, 0x3)
#define FT5x0x_TOUCH_XH_TOUCH_FLAGS(reg) (((reg) & FT5x0x_TOUCH_XH_TOUCH_FLAGS_bm) >> FT5x0x_TOUCH_XH_TOUCH_FLAGS_bp)

#define FT5x0x_TOUCH_FLAGS_DOWN     0
#define FT5x0x_TOUCH_FLAGS_UP       1
#define FT5x0x_TOUCH_FLAGS_CONTACT  2
#define FT5x0x_TOUCH_FLAGS_RESERVED 3

#define FT5x0x_TOUCH_YH_TOUCH_YH_bp   0
#define FT5x0x_TOUCH_YH_TOUCH_YH_bm   BIT8(FT5x0x_TOUCH_YH_TOUCH_YH_bp, 0xf)
#define FT5x0x_TOUCH_YH_TOUCH_YH(reg) (((reg) & FT5x0x_TOUCH_YH_TOUCH_YH_bm) >> FT5x0x_TOUCH_YH_TOUCH_YH_bp)

#define FT5x0x_TOUCH_YH_TOUCH_Y(regL, regH) (regL | (FT5x0x_TOUCH_YH_TOUCH_YH(regH) << 8))

#define FT5x0x_TOUCH_YH_TOUCH_ID_bp   4
#define FT5x0x_TOUCH_YH_TOUCH_ID_bm   BIT8(FT5x0x_TOUCH_YH_TOUCH_ID_bp, 0xf)
#define FT5x0x_TOUCH_YH_TOUCH_ID(reg) (((reg) & FT5x0x_TOUCH_YH_TOUCH_ID_bm) >> FT5x0x_TOUCH_YH_TOUCH_ID_bp)



class TouchDriver
{
public:
    static TouchDriver Instance;
    
    TouchDriver(const DigitalPin& pinWAKE, const DigitalPin& pinRESET, const DigitalPin& pinINT);
    void Initialize(const char* i2cPath);
    void HandleIRQ();
    
    void Tick();
    
    enum class EventID_e
    {
        None,
        Down,
        Up,
        Move
    };

    Signal<void, EventID_e, int, const IPoint&> SignalTouchEvent;
private:
    enum class State_e
    {
        Idle,
        TouchRegistered,
        ReceivingTouchData
    };
    static void IRQHandler(IRQn_Type irq, void* userData) { static_cast<TouchDriver*>(userData)->HandleIRQ(); }
    static void TransactionCallback(void* userObject, void* data, ssize_t length);    
    void TransactionCallback(void* data, ssize_t length);    
    
    DigitalPin   m_PinWAKE;
    DigitalPin   m_PinRESET;
    DigitalPin   m_PinINT;
    
    int m_I2CDevice = -1;
//    I2C* m_I2C = nullptr;
    
    FT5x0xOMRegisters m_Registers[2];
    RingBufferHandler<int8_t, 2> m_RegisterBufferHandler;

    static const int MAX_POINTS = 10;
    IPoint m_TouchPositions[MAX_POINTS];

    State_e   m_State = State_e::Idle;
    bigtime_t m_LastUpdateTime;
    int16_t   m_CursorX;
    int16_t   m_CursorY;
    bool      m_IsPressed = false;
    bool      m_CursorMoved;
};


} // namespace
