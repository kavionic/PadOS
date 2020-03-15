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

#include "System/Utils/Utils.h"

enum DigitalPortID
{
    e_DigitalPortID_A,
    e_DigitalPortID_B,
    e_DigitalPortID_C,
    e_DigitalPortID_D,
    e_DigitalPortID_E,
    e_DigitalPortID_F,
    e_DigitalPortID_G,
    e_DigitalPortID_H,
    e_DigitalPortID_I,
    e_DigitalPortID_J,
    e_DigitalPortID_K,
    e_DigitalPortID_Count,
    e_DigitalPortID_None
};

static GPIO_Port_t* DigitalPortsRegisters[] =
{
	GPIOA,
	GPIOB,
	GPIOC,
	GPIOD,
	GPIOE,
	GPIOF,
	GPIOG,
	GPIOH,
	GPIOI,
	GPIOJ,
	GPIOK
};



enum class DigitalPinPeripheralID : int
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
	const DigitalPortID          PORT;
	const int                    PIN;
	const DigitalPinPeripheralID MUX;
};

constexpr PinMuxTarget	PINMUX_TIM2_CH1_A0  = { e_DigitalPortID_A, 0, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_CH1_A5  = { e_DigitalPortID_A, 5, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_CH1_A15 = { e_DigitalPortID_A, 15, DigitalPinPeripheralID::AF1 };

constexpr PinMuxTarget	PINMUX_TIM2_ETR_A0  = { e_DigitalPortID_A, 0, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_ETR_A5  = { e_DigitalPortID_A, 5, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_ETR_A15 = { e_DigitalPortID_A, 15, DigitalPinPeripheralID::AF1 };

constexpr PinMuxTarget	PINMUX_TIM3_CH1_A6  = { e_DigitalPortID_A, 6, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM3_CH1_B4  = { e_DigitalPortID_B, 4, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM3_CH1_C6  = { e_DigitalPortID_C, 6, DigitalPinPeripheralID::AF2 };

constexpr PinMuxTarget	PINMUX_TIM4_CH1_B6  = { e_DigitalPortID_B, 6, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM4_CH1_D12 = { e_DigitalPortID_D, 12, DigitalPinPeripheralID::AF2 };

constexpr PinMuxTarget	PINMUX_TIM5_CH1_A0  = { e_DigitalPortID_A, 0, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM5_CH1_H10 = { e_DigitalPortID_H, 10, DigitalPinPeripheralID::AF2 };

constexpr PinMuxTarget	PINMUX_TIM8_CH1_C6  = { e_DigitalPortID_C, 6, DigitalPinPeripheralID::AF3 };
constexpr PinMuxTarget	PINMUX_TIM8_CH1_I5  = { e_DigitalPortID_I, 5, DigitalPinPeripheralID::AF3 };
constexpr PinMuxTarget	PINMUX_TIM8_CH1_J8  = { e_DigitalPortID_J, 8, DigitalPinPeripheralID::AF3 };

constexpr PinMuxTarget	PINMUX_TIM8_CH1N_A7  = { e_DigitalPortID_A, 7, DigitalPinPeripheralID::AF3 };

constexpr PinMuxTarget	PINMUX_USART0_RX_PA10 = { e_DigitalPortID_A, 10, DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_USART0_RX_PB7  = { e_DigitalPortID_B, 7, DigitalPinPeripheralID::AF7 };

constexpr PinMuxTarget	PINMUX_USART0_TX_PA9 = { e_DigitalPortID_A, 9, DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_USART0_TX_PB6 = { e_DigitalPortID_B, 6, DigitalPinPeripheralID::AF7 };

constexpr PinMuxTarget	PINMUX_I2C2_SCL_PB10 = { e_DigitalPortID_B, 10, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SCL_PF1  = { e_DigitalPortID_F, 1, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SCL_PH4  = { e_DigitalPortID_H, 4, DigitalPinPeripheralID::AF4 };

constexpr PinMuxTarget	PINMUX_I2C2_SDA_PB11 = { e_DigitalPortID_B, 11, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SDA_PF0  = { e_DigitalPortID_F, 0, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SDA_PH5  = { e_DigitalPortID_H, 5, DigitalPinPeripheralID::AF4 };

constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PB12 = { e_DigitalPortID_B, 12, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PF2  = { e_DigitalPortID_F, 2, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PH6  = { e_DigitalPortID_H, 6, DigitalPinPeripheralID::AF4 };

struct DigitalPort
{
    typedef uint32_t PortData_t;
    typedef std::atomic_uint_fast32_t IntMaskAcc;
    DigitalPort(DigitalPortID port) : m_Port(port) {}

    static GPIO_Port_t* GetPortRegs(DigitalPortID portID) { return DigitalPortsRegisters[portID]; }

//    static inline void SetAsInput(DigitalPortID portID, PortData_t pins)   { GetPortRegs(portID)->PIO_ODR = pins; }
//    static inline void SetAsOutput(DigitalPortID portID, PortData_t pins)  { GetPortRegs(portID)->PIO_OER = pins; }

    static inline void SetDirection(DigitalPortID portID, DigitalPinDirection_e direction, PortData_t pins)
    {
    	GPIO_Port_t* port = GetPortRegs(portID);
        switch(direction)
        {
            case DigitalPinDirection_e::In:
            	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2)
            	{
            		if (j & pins) {
            			port->MODER = (port->MODER & ~(3<<i)); // Mode 0: input
            		}
            	}
                break;
            case DigitalPinDirection_e::Out:
            	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2)
            	{
            		if (j & pins) {
            			port->MODER = (port->MODER & ~(3<<i)) | (1<<i); // Mode 1: output
            		}
            	}
                port->OTYPER &= ~pins; // 0: push-pull
                break;
            case DigitalPinDirection_e::OpenCollector:
            	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2)
            	{
            		if (j & pins) {
            			port->MODER = (port->MODER & ~(3<<i)) | (1<<i); // Mode 1: output
            		}
            	}
                port->OTYPER |= pins; // 1: open collector
                break;
        }
    }

    static inline void SetDriveStrength(DigitalPortID portID, DigitalPinDriveStrength_e strength, PortData_t pins)
    {
    	GPIO_Port_t* port = GetPortRegs(portID);
    	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2) {
    		if (j & pins) {
    			port->OSPEEDR = (port->OSPEEDR & ~(3<<i)) | (uint32_t(strength)<<i);
    		}
    	}
    }

    static inline void Set(DigitalPortID portID, PortData_t pins) { DigitalPortsRegisters[portID]->ODR = pins; }
//    static inline void SetSome(DigitalPortID portID, PortData_t mask, PortData_t pins) { DigitalPortsRegisters[portID]->PIO_OWER = mask; DigitalPortsRegisters[portID]->PIO_ODSR = pins; }
    static inline void SetHigh(DigitalPortID portID, PortData_t pins)                { DigitalPortsRegisters[portID]->BSRR = pins; }
    static inline void SetLow(DigitalPortID portID, PortData_t pins)                 { DigitalPortsRegisters[portID]->BSRR = pins << 16; }
    static inline PortData_t Get(DigitalPortID portID)                               { return DigitalPortsRegisters[portID]->IDR; }

    static void EnablePullup(DigitalPortID portID, PortData_t pins);

    static void SetPullMode(DigitalPortID portID, PinPullMode_e mode, PortData_t pins)
    {
    	GPIO_Port_t* port = GetPortRegs(portID);
        switch(mode)
        {
            case PinPullMode_e::None:
            	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2) {
            		if (j & pins) {
            			port->PUPDR = (port->PUPDR & ~(3<<i)) | (0<<i); // 0: no pull
            		}
            	}
                break;
            case PinPullMode_e::Down:
            	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2) {
            		if (j & pins) {
            			port->PUPDR = (port->PUPDR & ~(3<<i)) | (2<<i); // 2: pull down
            		}
            	}
                break;
            case PinPullMode_e::Up:
            	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2) {
            		if (j & pins) {
            			port->PUPDR = (port->PUPDR & ~(3<<i)) | (1<<i); // 1: pull up
            		}
            	}
                break;
        }
    }

    static void SetPeripheralMux(DigitalPortID portID, PortData_t pins, DigitalPinPeripheralID peripheral)
    {
    	GPIO_Port_t* port = GetPortRegs(portID);
        if (peripheral == DigitalPinPeripheralID::None)
        {
        	for (uint16_t i = 0, j = 0x0001; j != 0; j<<=1, i+=2)
			{
        		if (j & pins) {
        			port->MODER = (port->MODER & ~(3<<i)) | (3<<i); // Mode 3: analog (reset state)
        		}
        	}
        }
        else
        {
        	uint32_t value = uint32_t(peripheral) & 0xf;
        	uint32_t valueMask = 0xf;

        	int modeBitPos = 0;
        	for (uint8_t pinsL = uint8_t(pins); pinsL != 0; pinsL >>= 1, value <<= 4, valueMask <<= 4)
        	{
        		if (pinsL & 0x01)
        		{
        			set_bit_group(port->AFR[0], valueMask, value);
        			set_bit_group(port->MODER, 3<<modeBitPos, 2<<modeBitPos); // Mode 2: alternate function
        		}
        		modeBitPos += 2;
        	}
        	value = uint32_t(peripheral) & 0xf;
        	valueMask = 0xf;
        	for (uint8_t pinsH = uint8_t(pins >> 8); pinsH != 0; pinsH >>= 1, value <<= 4, valueMask <<= 4)
        	{
        		if (pinsH & 0x01)
        		{
        			set_bit_group(port->AFR[1], valueMask, value);
        			set_bit_group(port->MODER, 3<<modeBitPos, 2<<modeBitPos); // Mode 2: alternate function
        		}
        		modeBitPos += 2;
        	}
        }
    }
    void SetPeripheralMux(PortData_t pins, DigitalPinPeripheralID peripheral) { SetPeripheralMux(m_Port, pins, peripheral); }

    inline void SetDirection(DigitalPinDirection_e direction, PortData_t pins) { SetDirection(m_Port, direction, pins); }
    inline void SetDriveStrength(DigitalPinDriveStrength_e strength, PortData_t pins) { SetDriveStrength(m_Port, strength, pins); }
//    inline void SetAsInput(PortData_t pins)   { SetAsInput(m_Port, pins); }
//    inline void SetAsOutput(PortData_t pins)  { SetAsOutput(m_Port, pins); }

    
    inline void EnablePullup(PortData_t pins)          { EnablePullup(m_Port, pins); }
    inline void SetPullMode(PinPullMode_e mode, PortData_t pins) { SetPullMode(m_Port, mode, pins); }

    inline void Set(PortData_t pins)                   { Set(m_Port, pins); }
//    inline void SetSome(PortData_t mask, PortData_t pins) { SetSome(m_Port, mask, pins); }
    inline void SetHigh(PortData_t pins)               { SetHigh(m_Port, pins); }
    inline void SetLow(PortData_t pins)                { SetLow(m_Port, pins); }
    inline PortData_t Get()  const                     { return Get(m_Port); }

private:
    static IntMaskAcc s_IntMaskAccumulators[e_DigitalPortID_Count];
    DigitalPortID    m_Port;
};

class DigitalPin
{
public:
    DigitalPin() : m_Port(e_DigitalPortID_None), m_PinMask(0) { }
    DigitalPin(DigitalPortID port, int pin) : m_Port(port), m_PinMask(BIT32(pin, 1)) { }
    
    void Set(DigitalPortID port, int pin)  { m_Port = port; m_PinMask = BIT32(pin, 1); }
        
    void SetDirection(DigitalPinDirection_e dir) { m_Port.SetDirection(dir, m_PinMask); }
    void SetDriveStrength(DigitalPinDriveStrength_e strength) { m_Port.SetDriveStrength(strength, m_PinMask); }

    void SetPullMode(PinPullMode_e mode) { m_Port.SetPullMode(mode, m_PinMask); }
    void SetPeripheralMux(DigitalPinPeripheralID peripheral) { m_Port.SetPeripheralMux(m_PinMask, peripheral); }
        
    void Write(bool value) {if (value) m_Port.SetHigh(m_PinMask); else m_Port.SetLow(m_PinMask); }
    bool Read() const { return (m_Port.Get() & m_PinMask) != 0; }
    
    operator bool () { return Read(); }
    DigitalPin& operator=(bool value) { Write(value); return *this; }    
        
private:
    DigitalPort m_Port;
    uint32_t    m_PinMask;
};
