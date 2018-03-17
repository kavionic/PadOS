// This file is part of PadOS.
//
// Copyright (C) 2016-2018 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include "sam.h"

#include <atomic>

#include "System/Utils/Utils.h"


#define e_DigitalPortID_A (PIOA)
#define e_DigitalPortID_B (PIOB)
#define e_DigitalPortID_C (PIOC)
#define e_DigitalPortID_D (PIOD)
#define e_DigitalPortID_E (PIOE)

enum class DigitalPinDirection_e
{
    In,
    Out,
    OpenCollector
};

enum class DigitalPinPeripheralID : int
{
    None = -1,
    A = 0,
    B = 1,
    C = 2,
    D = 3
};

enum class PinPullMode_e
{
    None,
    Up,
    Down
};

enum class DigitalPinDriveStrength_e
{
    Low,
    High,
};

enum class PinInterruptMode_e
{
    BothEdges,
    FallingEdge,
    RisingEdge,
    LowLevel,
    HighLevel
};

struct DigitalPort
{
    typedef uint32_t PortData_t;
    typedef std::atomic_uint_fast32_t IntMaskAcc;
    DigitalPort(Pio* port) : m_Port(port) {}


#if defined(__AVR_MEGA__)
    static inline void SetDirection(DPortAddr_t port, PortData_t pins) { DPORT_DDR(port) = pins; }
    static inline void SetAsInput(DPortAddr_t port, PortData_t pins)   { DPORT_DDR(port) &= ~pins; }
    static inline void SetAsOutput(DPortAddr_t port, PortData_t pins)  { DPORT_DDR(port) |= pins; }

    static inline void Set(DPortAddr_t port, PortData_t pins) { DPORT_DATA(port) = pins; }
    static inline void SetSome(DPortAddr_t port, PortData_t mask, PortData_t pins) { DPORT_DATA(port) = (DPORT_DATA(port) & ~mask) | pins; }
    static inline void SetHigh(DPortAddr_t port, PortData_t pins) { DPORT_DATA(port) |= pins; }
    static inline void SetLow(DPortAddr_t port, PortData_t pins) { DPORT_DATA(port) &= ~pins; }
    static inline PortData_t Get(DPortAddr_t port) { return DPORT_IN(port); }

    static void EnablePullup(DPortAddr_t port, PortData_t pins)
    {
        SetHigh(port, pins);
        //        DPORT_DATA(port) = (DPORT_DATA(port) & ~pins) | pins;
    }

#elif defined(__AVR_XMEGA__)

    static inline void SetDirection(PORT_t* port, PortData_t pins) { port->DIR = pins; }
    static inline void SetAsInput(PORT_t* port, PortData_t pins)   { port->DIRCLR = pins; }
    static inline void SetAsOutput(PORT_t* port, PortData_t pins)  { port->DIRSET = pins; }

    static inline void Set(PORT_t* port, PortData_t pins) { port->OUT = pins; }
    static inline void SetSome(PORT_t* port, PortData_t mask, PortData_t pins) { port->OUT = (port->OUT & ~mask) | pins; }
    static inline void SetHigh(PORT_t* port, PortData_t pins) { port->OUTSET = pins; }
    static inline void SetLow(PORT_t* port, PortData_t pins) { port->OUTCLR = pins; }
    static inline PortData_t Get(PORT_t* port) { return port->IN; }

    static void EnablePullup(PORT_t* port, PortData_t pins)
    {
        for ( uint8_t i = 0 ; i < 8 ; ++i ) {
            if ( pins & 0x01 ) ((register8_t*)&port->PIN0CTRL)[i] = (((register8_t*)&port->PIN0CTRL)[i] & U8(~PORT_OPC_gm)) | PORT_OPC_PULLUP_gc;
            pins >>= 1;
        }
    }
    static void SetPinsOutputAndPullConfig(PORT_t* port, PortData_t pins, uint8_t config)
    {
        for ( uint8_t i = 0 ; i < 8 ; ++i ) {
            if ( pins & 0x01 ) ((register8_t*)&port->PIN0CTRL)[i] = (((register8_t*)&port->PIN0CTRL)[i] & U8(~PORT_OPC_gm)) | config;
            pins >>= 1;
        }
    }
    static void InvertPins(PORT_t* port, PortData_t pins)
    {
        for ( uint8_t i = 0 ; i < 8 ; ++i )
        {
            if ( pins & 0x01 )
            {
                ((register8_t*)&port->PIN0CTRL)[i] = (((register8_t*)&port->PIN0CTRL)[i] & U8(~PORT_INVEN_bm)) | PORT_INVEN_bm;
                
            }
            pins >>= 1;
        }
    }
    inline void InvertPins(PortData_t pins) { InvertPins(m_Port, pins); }
    inline void SetPinsOutputAndPullConfig(PortData_t pins, uint8_t config) { SetPinsOutputAndPullConfig(m_Port, pins, config); }

#elif defined(__SAME70N21__) || defined(__SAME70Q21__)
//    static inline void SetDirection(Pio* port, PortData_t pins) { port->PIO_ODR = ~pins; port->PIO_OER = pins; }
    static inline void SetAsInput(Pio* port, PortData_t pins)   { port->PIO_ODR = pins; }
    static inline void SetAsOutput(Pio* port, PortData_t pins)  { port->PIO_OER = pins; }

    static inline void SetDirection(Pio* port, DigitalPinDirection_e direction, PortData_t pins)
    {
        switch(direction)
        {
            case DigitalPinDirection_e::In:
                port->PIO_ODR  = pins;
                port->PIO_MDDR = pins;
                break;
            case DigitalPinDirection_e::Out:
                port->PIO_MDDR = pins;
                port->PIO_OER  = pins;
                break;
            case DigitalPinDirection_e::OpenCollector:
                port->PIO_MDER = pins;
                port->PIO_OER  = pins;
                break;
        }
    }

    static inline void SetDriveStrength(Pio* port, DigitalPinDriveStrength_e strength, PortData_t pins)
    {
        switch(strength)
        {
            case DigitalPinDriveStrength_e::Low:
                port->PIO_DRIVER  &= ~pins;
                break;
            case DigitalPinDriveStrength_e::High:
                port->PIO_DRIVER  |= pins;
                break;
        }
    }

    static inline void Set(Pio* port, PortData_t pins) { port->PIO_ODSR = pins; }
    static inline void SetSome(Pio* port, PortData_t mask, PortData_t pins) { port->PIO_OWER = mask; port->PIO_ODSR = pins; }
    static inline void SetHigh(Pio* port, PortData_t pins)                { port->PIO_SODR = pins; }
    static inline void SetLow(Pio* port, PortData_t pins)                 { port->PIO_CODR = pins; }
    static inline PortData_t Get(Pio* port)                               { return port->PIO_PDSR; }

    static void EnablePullup(Pio* port, PortData_t pins);

    static void SetPullMode(Pio* port, PinPullMode_e mode, PortData_t pins)
    {
        switch(mode)
        {
            case PinPullMode_e::None:
                port->PIO_PUDR = pins;
                port->PIO_PPDDR = pins;
                break;
            case PinPullMode_e::Down:
                port->PIO_PPDER = pins;
                port->PIO_PUDR = pins;
                break;
            case PinPullMode_e::Up:
                port->PIO_PUER = pins;
                port->PIO_PPDDR = pins;
                break;
        }
    }

    static void EnableInterrupts(Pio* port, PortData_t pins) { port->PIO_IER = pins; }
    static void DisableInterrupts(Pio* port, PortData_t pins) { port->PIO_IDR = pins; }
    static void SetInterruptMode(Pio* port, PinInterruptMode_e mode, PortData_t pins)
    {
        switch(mode)
        {
            case PinInterruptMode_e::BothEdges:
                port->PIO_AIMDR = pins; // Set default mode (both edges).
                break;
            case PinInterruptMode_e::FallingEdge:
                port->PIO_AIMER = pins; // Disable default mode (depends on PIO_ELSR and PIO_FRLHSR).
                port->PIO_ESR = pins; // Edge detect.
                port->PIO_FELLSR = pins; // Falling / low
                break;
            case PinInterruptMode_e::RisingEdge:
                port->PIO_AIMER = pins; // Disable default mode (depends on PIO_ELSR and PIO_FRLHSR).
                port->PIO_ESR = pins; // Edge detect.
                port->PIO_REHLSR = pins; // Rising / high
                break;
            case PinInterruptMode_e::LowLevel:
                port->PIO_AIMER = pins; // Disable default mode (depends on PIO_ELSR and PIO_FRLHSR).
                port->PIO_LSR = pins; // Level detect.
                port->PIO_FELLSR = pins; // Falling / low
                break;
            case PinInterruptMode_e::HighLevel:
                port->PIO_AIMER = pins; // Disable default mode (depends on PIO_ELSR and PIO_FRLHSR).
                port->PIO_LSR = pins; // Level detect.
                port->PIO_REHLSR = pins; // Rising / high
                break;
                
        }
    }
            
    static PortData_t GetInterruptStatus(Pio* port, IntMaskAcc* flagAccumulator, PortData_t pins)
    {
        *flagAccumulator |= port->PIO_ISR;
        PortData_t result = *flagAccumulator;
        *flagAccumulator &= ~pins;
        return result;
    }
    static PortData_t GetInterruptStatus(Pio* port)
    {
        return port->PIO_ISR;
    }
                
    static void SetPeripheralMux(Pio* port, PortData_t pins, DigitalPinPeripheralID peripheral)
    {
        if (peripheral == DigitalPinPeripheralID::None) {
            port->PIO_PER = pins;
        } else {
            port->PIO_PDR = pins;
            port->PIO_ABCDSR[0] = (port->PIO_ABCDSR[0] & ~pins) | ((int(peripheral) & 0x01) ? pins : 0);
            port->PIO_ABCDSR[1] = (port->PIO_ABCDSR[1] & ~pins) | ((int(peripheral) & 0x02) ? pins : 0);
        }
    }

    void SetPeripheralMux(PortData_t pins, DigitalPinPeripheralID peripheral) { SetPeripheralMux(m_Port, pins, peripheral); }
#endif

//    inline void SetDirection(PortData_t pins) { SetDirection(m_Port, pins); }
    inline void SetDirection(DigitalPinDirection_e direction, PortData_t pins) { SetDirection(m_Port, direction, pins); }
    inline void SetDriveStrength(DigitalPinDriveStrength_e strength, PortData_t pins) { SetDriveStrength(m_Port, strength, pins); }
    inline void SetAsInput(PortData_t pins)   { SetAsInput(m_Port, pins); }
    inline void SetAsOutput(PortData_t pins)  { SetAsOutput(m_Port, pins); }

    
    inline void EnablePullup(PortData_t pins)          { EnablePullup(m_Port, pins); }
    inline void SetPullMode(PinPullMode_e mode, PortData_t pins) { SetPullMode(m_Port, mode, pins); }

    inline void EnableInterrupts(PortData_t pins)                          { EnableInterrupts(m_Port, pins); }
    inline void DisableInterrupts(PortData_t pins)                         { DisableInterrupts(m_Port, pins); }
    inline void SetInterruptMode(PinInterruptMode_e mode, PortData_t pins) { SetInterruptMode(m_Port, mode, pins); }
    inline PortData_t GetInterruptStatus(IntMaskAcc* flagAccumulator, PortData_t pins) { return GetInterruptStatus(m_Port, flagAccumulator, pins); }
    inline PortData_t GetInterruptStatus()                                                           { return GetInterruptStatus(m_Port); }

    inline void Set(PortData_t pins)                   { Set(m_Port, pins); }
    inline void SetSome(PortData_t mask, PortData_t pins) { SetSome(m_Port, mask, pins); }
    inline void SetHigh(PortData_t pins)               { SetHigh(m_Port, pins); }
    inline void SetLow(PortData_t pins)                { SetLow(m_Port, pins); }
    inline PortData_t Get()  const                     { return Get(m_Port); }

    Pio*    m_Port;
};

class DigitalPin
{
public:
    DigitalPin(Pio* port, int pin) : m_Port(port), m_PinMask(BIT32(pin, 1)) { }
    
        
//    void SetDirection(DigitalPinDirection_e dir) { if (dir == DigitalPinDirection_e::IN) m_Port.SetAsInput(m_PinMask); else m_Port.SetAsOutput(m_PinMask); }
    void SetDirection(DigitalPinDirection_e dir) { m_Port.SetDirection(dir, m_PinMask); }
    void SetDriveStrength(DigitalPinDriveStrength_e strength) { m_Port.SetDriveStrength(strength, m_PinMask); }

    void SetPullMode(PinPullMode_e mode) { m_Port.SetPullMode(mode, m_PinMask); }
    void SetPeripheralMux(DigitalPinPeripheralID peripheral) { m_Port.SetPeripheralMux(m_PinMask, peripheral); }
        
    void EnableInterrupts()                                        { m_Port.EnableInterrupts(m_PinMask); }
    void DisableInterrupts()                                       { m_Port.DisableInterrupts(m_PinMask); }
    void SetInterruptMode(PinInterruptMode_e mode)                 { m_Port.SetInterruptMode(mode, m_PinMask); }
    bool GetInterruptStatus(DigitalPort::IntMaskAcc* flagAccumulator) { return (m_Port.GetInterruptStatus(flagAccumulator, m_PinMask) & m_PinMask) != 0; }
    bool GetInterruptStatus()                                                       { return (m_Port.GetInterruptStatus() & m_PinMask) != 0; }
        
    void Write(bool value) {if (value) m_Port.SetHigh(m_PinMask); else m_Port.SetLow(m_PinMask); }
    bool Read() const { return (m_Port.Get() & m_PinMask) != 0; }
        
private:
    DigitalPort m_Port;
    uint32_t    m_PinMask;
};
