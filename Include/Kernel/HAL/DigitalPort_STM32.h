// This file is part of PadOS.
//
// Copyright (C) 2016-2020 Kurt Skauen <http://kavionic.com/>
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

#include <atomic>
#include <System/Platform.h>
#include <System/Sections.h>

enum DigitalPortID
{
    e_DigitalPortID_A,
#ifdef GPIOB
    e_DigitalPortID_B,
#endif
#ifdef GPIOC
    e_DigitalPortID_C,
#endif
#ifdef GPIOD
    e_DigitalPortID_D,
#endif
#ifdef GPIOE
    e_DigitalPortID_E,
#endif
#ifdef GPIOF
    e_DigitalPortID_F,
#endif
#ifdef GPIOG
    e_DigitalPortID_G,
#endif
#ifdef GPIOH
    e_DigitalPortID_H,
#endif
#ifdef GPIOI
    e_DigitalPortID_I,
#endif
#ifdef GPIOJ
    e_DigitalPortID_J,
#endif
#ifdef GPIOK
    e_DigitalPortID_K,
#endif
    e_DigitalPortID_Count,
    e_DigitalPortID_None
};


#define MAKE_DIGITAL_PIN_ID(port, pin) (uint32_t(port) << 4 | uint32_t(pin))
#define DIGITAL_PIN_ID_PORT(pinID) (((pinID) != DigitalPinID::None) ? DigitalPortID(uint32_t(pinID) >> 4) : e_DigitalPortID_None)
#define DIGITAL_PIN_ID_PIN(pinID) (uint32_t(pinID) & 0x0f)

enum class DigitalPinID : uint32_t
{
    None = uint32_t(-1),
    A0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 0),
    A1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 1),
    A2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 2),
    A3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 3),
    A4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 4),
    A5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 5),
    A6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 6),
    A7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 7),
    A8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 8),
    A9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 9),
    A10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 10),
    A11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 11),
    A12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 12),
    A13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 13),
    A14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 14),
    A15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_A, 15),

#ifdef GPIOB
    B0 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 0),
    B1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 1),
    B2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 2),
    B3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 3),
    B4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 4),
    B5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 5),
    B6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 6),
    B7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 7),
    B8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 8),
    B9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 9),
    B10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 10),
    B11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 11),
    B12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 12),
    B13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 13),
    B14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 14),
    B15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 15),
#endif
#ifdef GPIOC
    C0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 0),
    C1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 1),
    C2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 2),
    C3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 3),
    C4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 4),
    C5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 5),
    C6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 6),
    C7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 7),
    C8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 8),
    C9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 9),
    C10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 10),
    C11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 11),
    C12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 12),
    C13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 13),
    C14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 14),
    C15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_C, 15),
#endif
#ifdef GPIOD
    D0 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 0),
    D1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 1),
    D2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 2),
    D3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 3),
    D4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 4),
    D5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 5),
    D6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 6),
    D7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 7),
    D8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 8),
    D9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 9),
    D10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 10),
    D11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 11),
    D12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 12),
    D13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 13),
    D14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 14),
    D15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 15),
#endif
#ifdef GPIOE
    E0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 0),
    E1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 1),
    E2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 2),
    E3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 3),
    E4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 4),
    E5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 5),
    E6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 6),
    E7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 7),
    E8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 8),
    E9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 9),
    E10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 10),
    E11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 11),
    E12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 12),
    E13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 13),
    E14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 14),
    E15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_E, 15),
#endif
#ifdef GPIOF
    F0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 0),
    F1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 1),
    F2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 2),
    F3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 3),
    F4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 4),
    F5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 5),
    F6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 6),
    F7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 7),
    F8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 8),
    F9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 9),
    F10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 10),
    F11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 11),
    F12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 12),
    F13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 13),
    F14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 14),
    F15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_F, 15),
#endif
#ifdef GPIOG
    G0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 0),
    G1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 1),
    G2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 2),
    G3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 3),
    G4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 4),
    G5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 5),
    G6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 6),
    G7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 7),
    G8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 8),
    G9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 9),
    G10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 10),
    G11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 11),
    G12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 12),
    G13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 13),
    G14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 14),
    G15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_G, 15),
#endif
#ifdef GPIOH
    H0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 0),
    H1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 1),
    H2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 2),
    H3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 3),
    H4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 4),
    H5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 5),
    H6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 6),
    H7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 7),
    H8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 8),
    H9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 9),
    H10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 10),
    H11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 11),
    H12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 12),
    H13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 13),
    H14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 14),
    H15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_H, 15),
#endif
#ifdef GPIOI
    I0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 0),
    I1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 1),
    I2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 2),
    I3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 3),
    I4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 4),
    I5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 5),
    I6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 6),
    I7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 7),
    I8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 8),
    I9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 9),
    I10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 10),
    I11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 11),
    I12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 12),
    I13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 13),
    I14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 14),
    I15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_I, 15),
#endif
#ifdef GPIOJ
    J0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 0),
    J1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 1),
    J2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 2),
    J3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 3),
    J4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 4),
    J5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 5),
    J6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 6),
    J7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 7),
    J8  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 8),
    J9  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 9),
    J10 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 10),
    J11 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 11),
    J12 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 12),
    J13 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 13),
    J14 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 14),
    J15 = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_J, 15),
#endif
#ifdef GPIOK
    K0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 0),
    K1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 1),
    K2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 2),
    K3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 3),
    K4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 4),
    K5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 5),
    K6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 6),
    K7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 7)
#endif
};

static IFLASHD constexpr GPIO_Port_t* DigitalPortsRegisters[] =
{
    GPIOA
#ifdef GPIOB
    ,GPIOB
#endif
#ifdef GPIOC
    ,GPIOC
#endif
#ifdef GPIOD
    ,GPIOD
#endif
#ifdef GPIOE
    ,GPIOE
#endif
#ifdef GPIOF
    ,GPIOF
#endif
#ifdef GPIOG
    ,GPIOG
#endif
#ifdef GPIOH
    ,GPIOH
#endif
#ifdef GPIOI
    ,GPIOI
#endif
#ifdef GPIOJ
    ,GPIOJ
#endif
#ifdef GPIOK
    ,GPIOK
#endif
};



enum class DigitalPinPeripheralID : int32_t
{
    None = -1,
    AF0  = 0,
    AF1  = 1,
    AF2  = 2,
    AF3  = 3,
    AF4  = 4,
    AF5  = 5,
    AF6  = 6,
    AF7  = 7,
    AF8  = 8,
    AF9  = 9,
    AF10 = 10,
    AF11 = 11,
    AF12 = 12,
    AF13 = 13,
    AF14 = 14,
    AF15 = 15
};


struct PinMuxTarget
{
    DigitalPinID           PINID;
    DigitalPinPeripheralID MUX;
};


struct DigitalPort
{
    typedef uint32_t PortData_t;
    typedef std::atomic_uint_fast32_t IntMaskAcc;
    DigitalPort(DigitalPortID port);

    static GPIO_Port_t* GetPortRegs(DigitalPortID portID);

//    static inline void SetAsInput(DigitalPortID portID, PortData_t pins)   { GetPortRegs(portID)->PIO_ODR = pins; }
//    static inline void SetAsOutput(DigitalPortID portID, PortData_t pins)  { GetPortRegs(portID)->PIO_OER = pins; }

    static void SetDirection(DigitalPortID portID, DigitalPinDirection_e direction, PortData_t pins);

    static void SetDriveStrength(DigitalPortID portID, DigitalPinDriveStrength_e strength, PortData_t pins);

    static void Set(DigitalPortID portID, PortData_t pins);
//    static inline void SetSome(DigitalPortID portID, PortData_t mask, PortData_t pins) { DigitalPortsRegisters[portID]->PIO_OWER = mask; DigitalPortsRegisters[portID]->PIO_ODSR = pins; }
    static void SetHigh(DigitalPortID portID, PortData_t pins);
    static void SetLow(DigitalPortID portID, PortData_t pins);
    static PortData_t Get(DigitalPortID portID);

//    static void EnablePullup(DigitalPortID portID, PortData_t pins);

    static void SetPullMode(DigitalPortID portID, PinPullMode_e mode, PortData_t pins);

    static void SetPeripheralMux(DigitalPortID portID, PortData_t pins, DigitalPinPeripheralID peripheral);
    void SetPeripheralMux(PortData_t pins, DigitalPinPeripheralID peripheral);

    void SetDirection(DigitalPinDirection_e direction, PortData_t pins);
    void SetDriveStrength(DigitalPinDriveStrength_e strength, PortData_t pins);
//    inline void SetAsInput(PortData_t pins)   { SetAsInput(m_Port, pins); }
//    inline void SetAsOutput(PortData_t pins)  { SetAsOutput(m_Port, pins); }

    
//    inline void EnablePullup(PortData_t pins)          { EnablePullup(m_Port, pins); }
    void SetPullMode(PinPullMode_e mode, PortData_t pins);

    void Set(PortData_t pins);
//    inline void SetSome(PortData_t mask, PortData_t pins) { SetSome(m_Port, mask, pins); }
    void SetHigh(PortData_t pins);
    void SetLow(PortData_t pins);
    PortData_t Get()  const;

private:
    static IntMaskAcc s_IntMaskAccumulators[e_DigitalPortID_Count];
    DigitalPortID    m_Port;
};

class DigitalPin
{
public:
    DigitalPin();
    DigitalPin(DigitalPinID pinID);
    DigitalPin(DigitalPortID port, int pin);
    
    void Set(DigitalPortID port, int pin);
    void Set(DigitalPinID pinID);

    DigitalPinID GetID() const;

    bool IsValid() const;

    void SetDirection(DigitalPinDirection_e dir);
    void SetDriveStrength(DigitalPinDriveStrength_e strength);

    void SetPullMode(PinPullMode_e mode);
    void SetPeripheralMux(DigitalPinPeripheralID peripheral);
    static void ActivatePeripheralMux(const PinMuxTarget& PinMux);

    void EnableInterrupts();
    void DisableInterrupts();
    void SetInterruptMode(PinInterruptMode_e mode);
#if defined(STM32H7)
    inline bool GetAndClearInterruptStatus()
    {
        bool flag = (EXTI->PR1 & m_PinMask) != 0;
        EXTI->PR1 = m_PinMask;
        return flag;
    }
#elif defined(STM32G0)
    inline bool GetAndClearInterruptStatus(PinInterruptMode_e mode)
    {
        switch (mode)
        {
            case PinInterruptMode_e::None:
                return false;
                break;
            case PinInterruptMode_e::BothEdges:
            {
                const bool flag = ((EXTI->RPR1 | EXTI->FPR1) & m_PinMask) != 0;
                EXTI->RPR1 = m_PinMask;
                EXTI->FPR1 = m_PinMask;
                return flag;
            }
            case PinInterruptMode_e::FallingEdge:
            {
                const bool flag = (EXTI->FPR1 & m_PinMask) != 0;
                EXTI->FPR1 = m_PinMask;
                return flag;
            }
            case PinInterruptMode_e::RisingEdge:
            {
                const bool flag = (EXTI->RPR1 & m_PinMask) != 0;
                EXTI->RPR1 = m_PinMask;
                return flag;
            }
        }
        return false;
    }
#else
#error Unknown platform.
#endif

    void Write(bool value);
    bool Read() const;
    

    operator bool ();
    DigitalPin& operator=(DigitalPinID pinID);
    DigitalPin& operator=(bool value);

private:
    DigitalPinID    m_PinID;
    DigitalPort     m_Port;
    uint32_t        m_PinMask;
};
