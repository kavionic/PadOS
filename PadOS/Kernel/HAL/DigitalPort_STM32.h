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

#define MAKE_DIGITAL_PIN_ID(port, pin) (uint32_t(port) << 4 | uint32_t(pin))
#define DIGITAL_PIN_ID_PORT(pinID) DigitalPortID(uint32_t(pinID) >> 4)
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

	B0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_B, 0),
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

	D0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_D, 0),
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

	K0  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 0),
	K1  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 1),
	K2  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 2),
	K3  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 3),
	K4  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 4),
	K5  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 5),
	K6  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 6),
	K7  = MAKE_DIGITAL_PIN_ID(e_DigitalPortID_K, 7)
};

constexpr GPIO_Port_t* DigitalPortsRegisters[] =
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
	const DigitalPinID           PINID;
	const DigitalPinPeripheralID MUX;
};

constexpr PinMuxTarget	PINMUX_TIM2_CH1_A0  = { DigitalPinID::A0, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_CH1_A5  = { DigitalPinID::A5, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_CH1_A15 = { DigitalPinID::A15, DigitalPinPeripheralID::AF1 };

constexpr PinMuxTarget	PINMUX_TIM2_ETR_A0  = { DigitalPinID::A0, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_ETR_A5  = { DigitalPinID::A5, DigitalPinPeripheralID::AF1 };
constexpr PinMuxTarget	PINMUX_TIM2_ETR_A15 = { DigitalPinID::A15, DigitalPinPeripheralID::AF1 };

constexpr PinMuxTarget	PINMUX_TIM3_CH1_A6  = { DigitalPinID::A6, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM3_CH1_B4  = { DigitalPinID::B4, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM3_CH1_C6  = { DigitalPinID::C6, DigitalPinPeripheralID::AF2 };

constexpr PinMuxTarget	PINMUX_TIM4_CH1_B6  = { DigitalPinID::B6, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM4_CH1_D12 = { DigitalPinID::D12, DigitalPinPeripheralID::AF2 };

constexpr PinMuxTarget	PINMUX_TIM5_CH1_A0  = { DigitalPinID::A0, DigitalPinPeripheralID::AF2 };
constexpr PinMuxTarget	PINMUX_TIM5_CH1_H10 = { DigitalPinID::H10, DigitalPinPeripheralID::AF2 };

constexpr PinMuxTarget	PINMUX_TIM8_CH1_C6  = { DigitalPinID::C6, DigitalPinPeripheralID::AF3 };
constexpr PinMuxTarget	PINMUX_TIM8_CH1_I5  = { DigitalPinID::I5, DigitalPinPeripheralID::AF3 };
constexpr PinMuxTarget	PINMUX_TIM8_CH1_J8  = { DigitalPinID::J8, DigitalPinPeripheralID::AF3 };

constexpr PinMuxTarget	PINMUX_TIM8_CH1N_A7  = { DigitalPinID::A7, DigitalPinPeripheralID::AF3 };

constexpr PinMuxTarget	PINMUX_USART1_RX_PA10 = { DigitalPinID::A10, DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_USART1_RX_PB7  = { DigitalPinID::B7, DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_USART1_RX_PB15 = { DigitalPinID::B15, DigitalPinPeripheralID::AF4 };

constexpr PinMuxTarget	PINMUX_USART1_TX_PA9 = { DigitalPinID::A9, DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_USART1_TX_PB6 = { DigitalPinID::B6, DigitalPinPeripheralID::AF7 };

constexpr PinMuxTarget	PINMUX_USART2_RX_PD6 = { DigitalPinID::D6, DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_USART2_RX_PA3 = { DigitalPinID::A3, DigitalPinPeripheralID::AF7 };

constexpr PinMuxTarget	PINMUX_USART2_TX_PD5 = { DigitalPinID::D5, DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_USART2_TX_PA2 = { DigitalPinID::A2, DigitalPinPeripheralID::AF7 };

constexpr PinMuxTarget	PINMUX_I2C2_SCL_PB10 = { DigitalPinID::B10, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SCL_PF1  = { DigitalPinID::F1,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SCL_PH4  = { DigitalPinID::H4,  DigitalPinPeripheralID::AF4 };

constexpr PinMuxTarget	PINMUX_I2C2_SDA_PB11 = { DigitalPinID::B11, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SDA_PF0  = { DigitalPinID::F0,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SDA_PH5  = { DigitalPinID::H5,  DigitalPinPeripheralID::AF4 };

constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PB12 = { DigitalPinID::B12, DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PF2  = { DigitalPinID::F2,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PH6  = { DigitalPinID::H6,  DigitalPinPeripheralID::AF4 };


constexpr PinMuxTarget	PINMUX_I2C4_SCL_PB6 = { DigitalPinID::B6, DigitalPinPeripheralID::AF6 };
constexpr PinMuxTarget	PINMUX_I2C4_SCL_PB8 = { DigitalPinID::B8,  DigitalPinPeripheralID::AF6 };
constexpr PinMuxTarget	PINMUX_I2C4_SCL_PD12 = { DigitalPinID::D12,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C4_SCL_PF14 = { DigitalPinID::F14,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C4_SCL_PH11 = { DigitalPinID::H11,  DigitalPinPeripheralID::AF4 };

constexpr PinMuxTarget	PINMUX_I2C4_SDA_PB7 = { DigitalPinID::B7, DigitalPinPeripheralID::AF6 };
constexpr PinMuxTarget	PINMUX_I2C4_SDA_PB9 = { DigitalPinID::B9,  DigitalPinPeripheralID::AF6 };
constexpr PinMuxTarget	PINMUX_I2C4_SDA_PD13 = { DigitalPinID::D13,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C4_SDA_PF15 = { DigitalPinID::F15,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C4_SDA_PH12 = { DigitalPinID::H12,  DigitalPinPeripheralID::AF4 };

constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PB5 = { DigitalPinID::B5, DigitalPinPeripheralID::AF6 };
constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PD11 = { DigitalPinID::D11,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PF13 = { DigitalPinID::F13,  DigitalPinPeripheralID::AF4 };
constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PH10 = { DigitalPinID::H10,  DigitalPinPeripheralID::AF4 };


constexpr PinMuxTarget	PINMUX_FMC_SDNWE_PA7	= { DigitalPinID::A7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDCKE1_PB5	= { DigitalPinID::B5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNE1_PB6	= { DigitalPinID::B6,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NL_PB7		= { DigitalPinID::B7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNWE_PC0	= { DigitalPinID::C0,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNE_PC2		= { DigitalPinID::C2,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDCK_PC3		= { DigitalPinID::C3,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNE_PC4		= { DigitalPinID::C4,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDCK_PC5		= { DigitalPinID::C5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NWAIT_PC6	= { DigitalPinID::C6,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_FMC_NE1_PC7		= { DigitalPinID::C7,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_FMC_NE2_PC8		= { DigitalPinID::C8,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_FMC_D2_PD0		= { DigitalPinID::D0,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D3_PD1		= { DigitalPinID::D1,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_CLK_PD3		= { DigitalPinID::D3,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NOE_PD4		= { DigitalPinID::D4,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NWE_PD5		= { DigitalPinID::D5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NWAIT_PD6	= { DigitalPinID::D6,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NE1_PD7		= { DigitalPinID::D7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D13_PD8		= { DigitalPinID::D8,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D14_PD9		= { DigitalPinID::D9,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D15_PD10		= { DigitalPinID::D10,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A16_PD11		= { DigitalPinID::D11,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A17_PD12		= { DigitalPinID::D12,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A18_PD13		= { DigitalPinID::D13,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D0_PD14		= { DigitalPinID::D14,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D1_PD15		= { DigitalPinID::D15,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NBL0_PE0		= { DigitalPinID::E0,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NBL1_PE1		= { DigitalPinID::E1,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A23_PE2		= { DigitalPinID::E2,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A19_PE3		= { DigitalPinID::E3,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A20_PE4		= { DigitalPinID::E4,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A21_PE5		= { DigitalPinID::E5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A22_PE6		= { DigitalPinID::E6,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D4_PE7		= { DigitalPinID::E7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D5_PE8		= { DigitalPinID::E8,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D6_PE9		= { DigitalPinID::E9,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D7_PE10		= { DigitalPinID::E10,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D8_PE11		= { DigitalPinID::E11,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D9_PE12		= { DigitalPinID::E12,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D10_PE13		= { DigitalPinID::E13,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D11_PE14		= { DigitalPinID::E14,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D12_PE15		= { DigitalPinID::E15,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A0_PF0		= { DigitalPinID::F0,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A1_PF1		= { DigitalPinID::F1,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A2_PF2		= { DigitalPinID::F2,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A3_PF3		= { DigitalPinID::F3,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A4_PF4		= { DigitalPinID::F4,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A5_PF5		= { DigitalPinID::F5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNRAS_PF11	= { DigitalPinID::F11,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A6_PF12		= { DigitalPinID::F12,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A7_PF13		= { DigitalPinID::F13,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A8_PF14		= { DigitalPinID::F14,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A9_PF15		= { DigitalPinID::F15,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A10_PG0		= { DigitalPinID::G0,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A11_PG1		= { DigitalPinID::G1,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A12_PG2		= { DigitalPinID::G2,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A13_PG3		= { DigitalPinID::G3,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A14_BA0_PG4	= { DigitalPinID::G4,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A15_BA1_PG5	= { DigitalPinID::G5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NE3_PG6		= { DigitalPinID::G6,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_INT_PG7		= { DigitalPinID::G7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDCLK_PG8	= { DigitalPinID::G8,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NE2_PG9		= { DigitalPinID::G9,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NE3_PG10		= { DigitalPinID::G10,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NE4_PG12		= { DigitalPinID::G12,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A24_PG13		= { DigitalPinID::G13,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_A25_PG14		= { DigitalPinID::G14,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNCAS_PG15	= { DigitalPinID::G15,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDCKE0_PH2	= { DigitalPinID::H2,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNE0_PH3	= { DigitalPinID::H3,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNWE_PH5	= { DigitalPinID::H5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDNE1_PH6	= { DigitalPinID::H6,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_SDCKE1_PH7	= { DigitalPinID::H7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D16_PH8		= { DigitalPinID::H8,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D17_PH9		= { DigitalPinID::H9,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D18_PH10		= { DigitalPinID::H10,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D19_PH11		= { DigitalPinID::H11,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D20_PH12		= { DigitalPinID::H12,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D21_PH13		= { DigitalPinID::H13,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D22_PH14		= { DigitalPinID::H14,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D23_PH15		= { DigitalPinID::H15,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D24_PI0		= { DigitalPinID::I0,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D25_PI1		= { DigitalPinID::I1,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D26_PI2		= { DigitalPinID::I2,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D27_PI3		= { DigitalPinID::I3,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NBL2_PI4		= { DigitalPinID::I4,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_NBL3_PI5		= { DigitalPinID::I5,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D28_PI6		= { DigitalPinID::I6,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D29_PI7		= { DigitalPinID::I7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D30_PI9		= { DigitalPinID::I9,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_FMC_D31_PI10		= { DigitalPinID::I10,	DigitalPinPeripheralID::AF12 };

constexpr PinMuxTarget	PINMUX_SDMMC1_D0_PC8 = { DigitalPinID::C8,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D1_PC9 = { DigitalPinID::C9,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D2_PC10 = { DigitalPinID::C10,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D3_PC11 = { DigitalPinID::C11,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D4_PB8 = { DigitalPinID::B8,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D5_PB9 = { DigitalPinID::B9,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D6_PC6 = { DigitalPinID::C6,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D7_PC7 = { DigitalPinID::C7,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_CMD_PD2 = { DigitalPinID::D2,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_CK_PC12 = { DigitalPinID::C12,	DigitalPinPeripheralID::AF12 };
constexpr PinMuxTarget	PINMUX_SDMMC1_CKIN_PB8 = { DigitalPinID::B8,	DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_SDMMC1_CDIR_PB9 = { DigitalPinID::B9,	DigitalPinPeripheralID::AF7 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D0DIR_PC6 = { DigitalPinID::C6,	DigitalPinPeripheralID::AF8 };
constexpr PinMuxTarget	PINMUX_SDMMC1_D123DIR_PC7 = { DigitalPinID::C7,	DigitalPinPeripheralID::AF8 };

constexpr PinMuxTarget	PINMUX_SDMMC2_D0_PB14 = { DigitalPinID::B14,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D1_PB15 = { DigitalPinID::B15,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D2_PB3		= { DigitalPinID::B3,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D2_PG11 = { DigitalPinID::G11,	DigitalPinPeripheralID::AF10 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D3_PB4		= { DigitalPinID::B4,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D4_PB8		= { DigitalPinID::B8,	DigitalPinPeripheralID::AF10 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D5_PB9		= { DigitalPinID::B9,	DigitalPinPeripheralID::AF10 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D6_PC6		= { DigitalPinID::C6,	DigitalPinPeripheralID::AF10 };
constexpr PinMuxTarget	PINMUX_SDMMC2_D7_PC7		= { DigitalPinID::C7,	DigitalPinPeripheralID::AF10 };
constexpr PinMuxTarget	PINMUX_SDMMC2_CMD_PA0 = { DigitalPinID::A0,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_SDMMC2_CMD_PD7 = { DigitalPinID::D7,	DigitalPinPeripheralID::AF11 };
constexpr PinMuxTarget	PINMUX_SDMMC2_CK_PC1 = { DigitalPinID::C1,	DigitalPinPeripheralID::AF9 };
constexpr PinMuxTarget	PINMUX_SDMMC2_CK_PD6		= { DigitalPinID::D6,	DigitalPinPeripheralID::AF11 };


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
        	modeBitPos = 2*8;
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
    DigitalPin() : m_PinID(DigitalPinID::None), m_Port(e_DigitalPortID_None), m_PinMask(0) { }
	DigitalPin(DigitalPinID pinID) : m_PinID(pinID), m_Port(DIGITAL_PIN_ID_PORT(pinID)), m_PinMask(BIT32(DIGITAL_PIN_ID_PIN(pinID), 1)) { }
	DigitalPin(DigitalPortID port, int pin) : m_PinID(DigitalPinID(MAKE_DIGITAL_PIN_ID(port, pin))), m_Port(port), m_PinMask(BIT32(pin, 1)) { }
    
    void Set(DigitalPortID port, int pin)  { m_Port = port; m_PinMask = BIT32(pin, 1); }
        
    void SetDirection(DigitalPinDirection_e dir) { m_Port.SetDirection(dir, m_PinMask); }
    void SetDriveStrength(DigitalPinDriveStrength_e strength) { m_Port.SetDriveStrength(strength, m_PinMask); }

    void SetPullMode(PinPullMode_e mode) { m_Port.SetPullMode(mode, m_PinMask); }
    void SetPeripheralMux(DigitalPinPeripheralID peripheral) { m_Port.SetPeripheralMux(m_PinMask, peripheral); }
	static void ActivatePeripheralMux(const PinMuxTarget& PinMux) { DigitalPin(PinMux.PINID).SetPeripheralMux(PinMux.MUX); }

	void EnableInterrupts() { EXTI->IMR1 |= m_PinMask; }
	void DisableInterrupts() { EXTI->IMR1 &= ~m_PinMask; }
	void SetInterruptMode(PinInterruptMode_e mode)
	{
		int pinIndex = DIGITAL_PIN_ID_PIN(m_PinID);

		if (mode == PinInterruptMode_e::FallingEdge || mode == PinInterruptMode_e::BothEdges) {
			EXTI->FTSR1 |= m_PinMask; // EXTI1 falling edge enabled.
		} else {
			EXTI->FTSR1 &= ~m_PinMask; // EXTI1 falling edge enabled.
		}
		if (mode == PinInterruptMode_e::RisingEdge || mode == PinInterruptMode_e::BothEdges) {
			EXTI->RTSR1 |= m_PinMask; // EXTI1 rising edge enabled.
		} else {
			EXTI->RTSR1 &= ~m_PinMask; // EXTI1 rising edge enabled.
		}
		if (mode != PinInterruptMode_e::None)
		{
			int portIndex = DIGITAL_PIN_ID_PORT(m_PinID);
			uint32_t regIndex = pinIndex >> 2;
			uint32_t groupPos = (pinIndex & 0x03) * 4;
			uint32_t mask = 0x000f << groupPos;
			SYSCFG->EXTICR[regIndex] = (SYSCFG->EXTICR[regIndex] & ~mask) | (portIndex << groupPos); // Route signals from this port to EXTI.
		}
	}
	bool GetAndClearInterruptStatus()
	{
		bool flag = (EXTI->PR1 & m_PinMask) != 0;
		EXTI->PR1 = m_PinMask;
		return flag;
	}

    void Write(bool value) {if (value) m_Port.SetHigh(m_PinMask); else m_Port.SetLow(m_PinMask); }
    bool Read() const { return (m_Port.Get() & m_PinMask) != 0; }
    

    operator bool () { return Read(); }
    DigitalPin& operator=(bool value) { Write(value); return *this; }    
        
private:
	DigitalPinID	m_PinID;
	DigitalPort		m_Port;
    uint32_t		m_PinMask;
};
