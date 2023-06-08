// This file is part of PadOS.
//
// Copyright (C) 2023 Kurt Skauen <http://kavionic.com/>
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



static IFLASHD constexpr PinMuxTarget	PINMUX_NONE = { DigitalPinID::None, DigitalPinPeripheralID::None };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM1_CH1_A8 = { DigitalPinID::A8, DigitalPinPeripheralID::AF1 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM2_CH1_A0 = { DigitalPinID::A0, DigitalPinPeripheralID::AF1 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM2_CH1_A5 = { DigitalPinID::A5, DigitalPinPeripheralID::AF1 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM2_CH1_A15 = { DigitalPinID::A15, DigitalPinPeripheralID::AF1 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM2_ETR_A0 = { DigitalPinID::A0, DigitalPinPeripheralID::AF1 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM2_ETR_A5 = { DigitalPinID::A5, DigitalPinPeripheralID::AF1 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM2_ETR_A15 = { DigitalPinID::A15, DigitalPinPeripheralID::AF1 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM3_CH1_A6 = { DigitalPinID::A6, DigitalPinPeripheralID::AF2 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM3_CH1_B4 = { DigitalPinID::B4, DigitalPinPeripheralID::AF2 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM3_CH1_C6 = { DigitalPinID::C6, DigitalPinPeripheralID::AF2 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM4_CH1_B6 = { DigitalPinID::B6, DigitalPinPeripheralID::AF2 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM4_CH1_D12 = { DigitalPinID::D12, DigitalPinPeripheralID::AF2 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM5_CH1_A0 = { DigitalPinID::A0, DigitalPinPeripheralID::AF2 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM5_CH1_H10 = { DigitalPinID::H10, DigitalPinPeripheralID::AF2 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM8_CH1_C6 = { DigitalPinID::C6, DigitalPinPeripheralID::AF3 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM8_CH1_I5 = { DigitalPinID::I5, DigitalPinPeripheralID::AF3 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM8_CH1_J8 = { DigitalPinID::J8, DigitalPinPeripheralID::AF3 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM8_CH1N_A7 = { DigitalPinID::A7, DigitalPinPeripheralID::AF3 };

static IFLASHD constexpr PinMuxTarget	PINMUX_TIM15_CH1_E5 = { DigitalPinID::E5, DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_TIM17_CH1_B9 = { DigitalPinID::B9, DigitalPinPeripheralID::AF1 };

static IFLASHD constexpr PinMuxTarget	PINMUX_USART1_RX_PA10 = { DigitalPinID::A10, DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_USART1_RX_PB7 = { DigitalPinID::B7, DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_USART1_RX_PB15 = { DigitalPinID::B15, DigitalPinPeripheralID::AF4 };

static IFLASHD constexpr PinMuxTarget	PINMUX_USART1_TX_PA9 = { DigitalPinID::A9, DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_USART1_TX_PB6 = { DigitalPinID::B6, DigitalPinPeripheralID::AF7 };

static IFLASHD constexpr PinMuxTarget	PINMUX_USART2_RX_PD6 = { DigitalPinID::D6, DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_USART2_RX_PA3 = { DigitalPinID::A3, DigitalPinPeripheralID::AF7 };

static IFLASHD constexpr PinMuxTarget	PINMUX_USART2_TX_PD5 = { DigitalPinID::D5, DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_USART2_TX_PA2 = { DigitalPinID::A2, DigitalPinPeripheralID::AF7 };

static IFLASHD constexpr PinMuxTarget	PINMUX_I2C1_SCL_PB6 = { DigitalPinID::B6, DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C1_SCL_PB8 = { DigitalPinID::B8, DigitalPinPeripheralID::AF4 };

static IFLASHD constexpr PinMuxTarget	PINMUX_I2C1_SDA_PB7 = { DigitalPinID::B7, DigitalPinPeripheralID::AF4 };

static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SCL_PB10 = { DigitalPinID::B10, DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SCL_PF1 = { DigitalPinID::F1,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SCL_PH4 = { DigitalPinID::H4,  DigitalPinPeripheralID::AF4 };

static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SDA_PB11 = { DigitalPinID::B11, DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SDA_PF0 = { DigitalPinID::F0,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SDA_PH5 = { DigitalPinID::H5,  DigitalPinPeripheralID::AF4 };

static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PB12 = { DigitalPinID::B12, DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PF2 = { DigitalPinID::F2,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C2_SMBA_PH6 = { DigitalPinID::H6,  DigitalPinPeripheralID::AF4 };


static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SCL_PB6 = { DigitalPinID::B6, DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SCL_PB8 = { DigitalPinID::B8,  DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SCL_PD12 = { DigitalPinID::D12,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SCL_PF14 = { DigitalPinID::F14,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SCL_PH11 = { DigitalPinID::H11,  DigitalPinPeripheralID::AF4 };

static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SDA_PB7 = { DigitalPinID::B7, DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SDA_PB9 = { DigitalPinID::B9,  DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SDA_PD13 = { DigitalPinID::D13,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SDA_PF15 = { DigitalPinID::F15,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SDA_PH12 = { DigitalPinID::H12,  DigitalPinPeripheralID::AF4 };

static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PB5 = { DigitalPinID::B5, DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PD11 = { DigitalPinID::D11,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PF13 = { DigitalPinID::F13,  DigitalPinPeripheralID::AF4 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2C4_SMBA_PH10 = { DigitalPinID::H10,  DigitalPinPeripheralID::AF4 };


static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNWE_PA7 = { DigitalPinID::A7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDCKE1_PB5 = { DigitalPinID::B5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNE1_PB6 = { DigitalPinID::B6,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NL_PB7 = { DigitalPinID::B7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNWE_PC0 = { DigitalPinID::C0,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNE0_PC2 = { DigitalPinID::C2,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDCKE0_PC3 = { DigitalPinID::C3,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNE0_PC4 = { DigitalPinID::C4,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDCKE0_PC5 = { DigitalPinID::C5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NWAIT_PC6 = { DigitalPinID::C6,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NE1_PC7 = { DigitalPinID::C7,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NE2_PC8 = { DigitalPinID::C8,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D2_PD0 = { DigitalPinID::D0,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D3_PD1 = { DigitalPinID::D1,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_CLK_PD3 = { DigitalPinID::D3,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NOE_PD4 = { DigitalPinID::D4,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NWE_PD5 = { DigitalPinID::D5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NWAIT_PD6 = { DigitalPinID::D6,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NE1_PD7 = { DigitalPinID::D7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D13_PD8 = { DigitalPinID::D8,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D14_PD9 = { DigitalPinID::D9,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D15_PD10 = { DigitalPinID::D10,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A16_PD11 = { DigitalPinID::D11,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A17_PD12 = { DigitalPinID::D12,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A18_PD13 = { DigitalPinID::D13,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D0_PD14 = { DigitalPinID::D14,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D1_PD15 = { DigitalPinID::D15,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NBL0_PE0 = { DigitalPinID::E0,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NBL1_PE1 = { DigitalPinID::E1,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A23_PE2 = { DigitalPinID::E2,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A19_PE3 = { DigitalPinID::E3,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A20_PE4 = { DigitalPinID::E4,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A21_PE5 = { DigitalPinID::E5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A22_PE6 = { DigitalPinID::E6,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D4_PE7 = { DigitalPinID::E7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D5_PE8 = { DigitalPinID::E8,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D6_PE9 = { DigitalPinID::E9,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D7_PE10 = { DigitalPinID::E10,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D8_PE11 = { DigitalPinID::E11,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D9_PE12 = { DigitalPinID::E12,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D10_PE13 = { DigitalPinID::E13,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D11_PE14 = { DigitalPinID::E14,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D12_PE15 = { DigitalPinID::E15,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A0_PF0 = { DigitalPinID::F0,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A1_PF1 = { DigitalPinID::F1,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A2_PF2 = { DigitalPinID::F2,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A3_PF3 = { DigitalPinID::F3,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A4_PF4 = { DigitalPinID::F4,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A5_PF5 = { DigitalPinID::F5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNRAS_PF11 = { DigitalPinID::F11,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A6_PF12 = { DigitalPinID::F12,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A7_PF13 = { DigitalPinID::F13,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A8_PF14 = { DigitalPinID::F14,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A9_PF15 = { DigitalPinID::F15,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A10_PG0 = { DigitalPinID::G0,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A11_PG1 = { DigitalPinID::G1,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A12_PG2 = { DigitalPinID::G2,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A13_PG3 = { DigitalPinID::G3,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A14_BA0_PG4 = { DigitalPinID::G4,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A15_BA1_PG5 = { DigitalPinID::G5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NE3_PG6 = { DigitalPinID::G6,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_INT_PG7 = { DigitalPinID::G7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDCLK_PG8 = { DigitalPinID::G8,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NE2_PG9 = { DigitalPinID::G9,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NE3_PG10 = { DigitalPinID::G10,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NE4_PG12 = { DigitalPinID::G12,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A24_PG13 = { DigitalPinID::G13,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_A25_PG14 = { DigitalPinID::G14,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNCAS_PG15 = { DigitalPinID::G15,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDCKE0_PH2 = { DigitalPinID::H2,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNE0_PH3 = { DigitalPinID::H3,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNWE_PH5 = { DigitalPinID::H5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDNE1_PH6 = { DigitalPinID::H6,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_SDCKE1_PH7 = { DigitalPinID::H7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D16_PH8 = { DigitalPinID::H8,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D17_PH9 = { DigitalPinID::H9,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D18_PH10 = { DigitalPinID::H10,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D19_PH11 = { DigitalPinID::H11,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D20_PH12 = { DigitalPinID::H12,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D21_PH13 = { DigitalPinID::H13,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D22_PH14 = { DigitalPinID::H14,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D23_PH15 = { DigitalPinID::H15,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D24_PI0 = { DigitalPinID::I0,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D25_PI1 = { DigitalPinID::I1,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D26_PI2 = { DigitalPinID::I2,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D27_PI3 = { DigitalPinID::I3,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NBL2_PI4 = { DigitalPinID::I4,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_NBL3_PI5 = { DigitalPinID::I5,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D28_PI6 = { DigitalPinID::I6,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D29_PI7 = { DigitalPinID::I7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D30_PI9 = { DigitalPinID::I9,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_FMC_D31_PI10 = { DigitalPinID::I10,	DigitalPinPeripheralID::AF12 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D0_PC8 = { DigitalPinID::C8,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D1_PC9 = { DigitalPinID::C9,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D2_PC10 = { DigitalPinID::C10,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D3_PC11 = { DigitalPinID::C11,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D4_PB8 = { DigitalPinID::B8,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D5_PB9 = { DigitalPinID::B9,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D6_PC6 = { DigitalPinID::C6,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D7_PC7 = { DigitalPinID::C7,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_CMD_PD2 = { DigitalPinID::D2,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_CK_PC12 = { DigitalPinID::C12,	DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_CKIN_PB8 = { DigitalPinID::B8,	DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_CDIR_PB9 = { DigitalPinID::B9,	DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D0DIR_PC6 = { DigitalPinID::C6,	DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC1_D123DIR_PC7 = { DigitalPinID::C7,	DigitalPinPeripheralID::AF8 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D0_PB14 = { DigitalPinID::B14,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D1_PB15 = { DigitalPinID::B15,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D2_PB3 = { DigitalPinID::B3,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D2_PG11 = { DigitalPinID::G11,	DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D3_PB4 = { DigitalPinID::B4,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D4_PB8 = { DigitalPinID::B8,	DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D5_PB9 = { DigitalPinID::B9,	DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D6_PC6 = { DigitalPinID::C6,	DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_D7_PC7 = { DigitalPinID::C7,	DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_CMD_PA0 = { DigitalPinID::A0,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_CMD_PD7 = { DigitalPinID::D7,	DigitalPinPeripheralID::AF11 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_CK_PC1 = { DigitalPinID::C1,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SDMMC2_CK_PD6 = { DigitalPinID::D6,	DigitalPinPeripheralID::AF11 };

static IFLASHD constexpr PinMuxTarget	PINMUX_QUADSPI_BK1_D0_PF8 = { DigitalPinID::F8,	DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_QUADSPI_BK1_D1_PF9 = { DigitalPinID::F9,	DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_QUADSPI_BK1_D2_PF7 = { DigitalPinID::F7,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_QUADSPI_BK1_D3_PF6 = { DigitalPinID::F6,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_QUADSPI_BK1_CLK_PF10 = { DigitalPinID::F10,	DigitalPinPeripheralID::AF9 };
static IFLASHD constexpr PinMuxTarget	PINMUX_QUADSPI_BK1_NCS_PG6 = { DigitalPinID::G6,	DigitalPinPeripheralID::AF10 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_NSS_PA4 = { DigitalPinID::A4,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_WS_PA4 = { DigitalPinID::A4,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_SCK_PA5 = { DigitalPinID::A5,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_CK_PA5 = { DigitalPinID::A5,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_MISO_PA6 = { DigitalPinID::A6,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_SDI_PA6 = { DigitalPinID::A6,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_MOSI_PA7 = { DigitalPinID::A7,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_SDO_PA7 = { DigitalPinID::A7,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_NSS_PA15 = { DigitalPinID::A15,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_WS_PA15 = { DigitalPinID::A15,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_SCK_PB3 = { DigitalPinID::B3,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_CK_PB3 = { DigitalPinID::B3,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_MISO_PB4 = { DigitalPinID::B4,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_SDI_PB4 = { DigitalPinID::B4,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_MOSI_PB5 = { DigitalPinID::B5,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_SDO_PB5 = { DigitalPinID::B5,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_MOSI_PD7 = { DigitalPinID::D7,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_SDO_PD7 = { DigitalPinID::D7,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_MISO_PG9 = { DigitalPinID::G9,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_SDI_PG9 = { DigitalPinID::G9,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_NSS_PG10 = { DigitalPinID::G10,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_WS_PG10 = { DigitalPinID::G10,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI1_SCK_PG11 = { DigitalPinID::G11,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S1_CK_PG11 = { DigitalPinID::G11,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_SCK_PA9 = { DigitalPinID::A9,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_CK_PA9 = { DigitalPinID::A9,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_NSS_PA11 = { DigitalPinID::A11,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_WS_PA11 = { DigitalPinID::A11,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_SCK_PA12 = { DigitalPinID::A12,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_CK_PA12 = { DigitalPinID::A12,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_NSS_PB4 = { DigitalPinID::B4,		DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_WS_PB4 = { DigitalPinID::B4,		DigitalPinPeripheralID::AF7 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_NSS_PB9 = { DigitalPinID::B9,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_WS_PB9 = { DigitalPinID::B9,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_SCK_PB10 = { DigitalPinID::B10,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_CK_PB10 = { DigitalPinID::B10,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_NSS_PB12 = { DigitalPinID::B12,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_WS_PB12 = { DigitalPinID::B12,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_SCK_PB13 = { DigitalPinID::B13,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_CK_PB13 = { DigitalPinID::B13,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_MISO_PB14 = { DigitalPinID::B14,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_SDI_PB14 = { DigitalPinID::B14,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_MOSI_PB15 = { DigitalPinID::B15,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_SDO_PB15 = { DigitalPinID::B15,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_MOSI_PC1 = { DigitalPinID::C1,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_SDO_PC1 = { DigitalPinID::C1,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_MISO_PC2 = { DigitalPinID::C2,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_SDI_PC2 = { DigitalPinID::C2,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_MOSI_PC3 = { DigitalPinID::C3,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_SDO_PC3 = { DigitalPinID::C3,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_SCK_PD3 = { DigitalPinID::D3,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_CK_PD3 = { DigitalPinID::D3,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_NSS_PI0 = { DigitalPinID::I0,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_WS_PI0 = { DigitalPinID::I0,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_SCK_PI1 = { DigitalPinID::I1,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_CK_PI1 = { DigitalPinID::I1,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_MISO_PI2 = { DigitalPinID::I2,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_SDI_PI2 = { DigitalPinID::I2,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI2_MOSI_PI3 = { DigitalPinID::I3,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S2_SDO_PI3 = { DigitalPinID::I3,		DigitalPinPeripheralID::AF5 };


static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_NSS_PA4 = { DigitalPinID::A4,		DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_WS_PA4 = { DigitalPinID::A4,		DigitalPinPeripheralID::AF6 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_NSS_PA15 = { DigitalPinID::A15,		DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_WS_PA15 = { DigitalPinID::A15,		DigitalPinPeripheralID::AF6 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_MOSI_PB2 = { DigitalPinID::B2,		DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_SDO_PB2 = { DigitalPinID::B2,		DigitalPinPeripheralID::AF7 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_SCK_PB3 = { DigitalPinID::B3,		DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_CK_PB3 = { DigitalPinID::B3,		DigitalPinPeripheralID::AF6 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_MISO_PB4 = { DigitalPinID::B4,		DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_SDI_PB4 = { DigitalPinID::B4,		DigitalPinPeripheralID::AF6 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_MOSI_PB5 = { DigitalPinID::B5,		DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_SDO_PB5 = { DigitalPinID::B5,		DigitalPinPeripheralID::AF7 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_SCK_PC10 = { DigitalPinID::C10,		DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_CK_PC10 = { DigitalPinID::C10,		DigitalPinPeripheralID::AF6 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_MISO_PC11 = { DigitalPinID::C11,		DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_SDI_PC11 = { DigitalPinID::C11,		DigitalPinPeripheralID::AF6 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_MOSI_PC12 = { DigitalPinID::C12,		DigitalPinPeripheralID::AF6 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_SDO_PC12 = { DigitalPinID::C12,		DigitalPinPeripheralID::AF6 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI3_MOSI_PD6 = { DigitalPinID::D6,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_I2S3_SDO_PD6 = { DigitalPinID::D6,		DigitalPinPeripheralID::AF5 };


static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_SCK_PE2 = { DigitalPinID::E2,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_NSS_PE4 = { DigitalPinID::E4,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_MISO_PE5 = { DigitalPinID::E5,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_MOSI_PE6 = { DigitalPinID::E6,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_NSS_PE11 = { DigitalPinID::E11,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_SCK_PE12 = { DigitalPinID::E12,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_MISO_PE13 = { DigitalPinID::E13,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI4_MOSI_PE14 = { DigitalPinID::E14,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_NSS_PF6 = { DigitalPinID::F6,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_SCK_PF7 = { DigitalPinID::F7,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_MISO_PF8 = { DigitalPinID::F8,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_MOSI_PF9 = { DigitalPinID::F9,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_MOSI_PF11 = { DigitalPinID::F11,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_NSS_PH5 = { DigitalPinID::H5,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_SCK_PH6 = { DigitalPinID::H6,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_MISO_PH7 = { DigitalPinID::H7,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_MOSI_PJ10 = { DigitalPinID::J10,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_MISO_PJ11 = { DigitalPinID::J11,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_SCK_PK0 = { DigitalPinID::K0,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI5_NSS_PK1 = { DigitalPinID::K1,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_NSS_PA4 = { DigitalPinID::A4,		DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_SCK_PA5 = { DigitalPinID::A5,		DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_MISO_PA6 = { DigitalPinID::A6,		DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_MOSI_PA7 = { DigitalPinID::A7,		DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_NSS_PA15 = { DigitalPinID::A15,		DigitalPinPeripheralID::AF7 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_SCK_PB3 = { DigitalPinID::B3,		DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_MISO_PB4 = { DigitalPinID::B4,		DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_MOSI_PB5 = { DigitalPinID::B5,		DigitalPinPeripheralID::AF8 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_NSS_PG8 = { DigitalPinID::G8,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_MISO_PG12 = { DigitalPinID::G12,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_SCK_PG13 = { DigitalPinID::G13,		DigitalPinPeripheralID::AF5 };
static IFLASHD constexpr PinMuxTarget	PINMUX_SPI6_MOSI_PG14 = { DigitalPinID::G14,		DigitalPinPeripheralID::AF5 };

static IFLASHD constexpr PinMuxTarget	PINMUX_OTG_FS_ID_PA10 = { DigitalPinID::A10,		DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_OTG_FS_DM_PA11 = { DigitalPinID::A11,		DigitalPinPeripheralID::AF10 };
static IFLASHD constexpr PinMuxTarget	PINMUX_OTG_FS_DP_PA12 = { DigitalPinID::A12,		DigitalPinPeripheralID::AF10 };

static IFLASHD constexpr PinMuxTarget	PINMUX_OTG1_HS_ID_PB12 = { DigitalPinID::B12,		DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_OTG1_HS_DM_PB14 = { DigitalPinID::B14,		DigitalPinPeripheralID::AF12 };
static IFLASHD constexpr PinMuxTarget	PINMUX_OTG1_HS_DP_PB15 = { DigitalPinID::B15,		DigitalPinPeripheralID::AF12 };
