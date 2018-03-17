// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 01.11.2017 09:57:07

#pragma once


#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"
#include "SAME70TimerDefines.h"

static const uint32_t CLOCK_CRYSTAL_FREQUENCY = 12000000;
static const uint32_t CLOCK_CPU_FREQUENCY     = 300000000;
static const uint32_t CLOCK_PERIF_FREQUENCY   = CLOCK_CPU_FREQUENCY / 3;

//void PreInitSetup();
void SetupEBIPeripherals();
void SetupMemoryRegions();

#define SDRAM_SIZE (64*1024*1024)
#define SDRAM_START SDRAM_CS_ADDR
#define SDRAM_END   (SDRAM_START + SDRAM_SIZE - 1)

#define SYSTEM_TIMER TC1
//#define SYSTEM_TIMER_REALTIME_CHANNEL 0
#define SYSTEM_TIMER_SPIN_TIMER_L     1
//#define SYSTEM_TIMER_SPIN_TIMER_H     2

#define SYSTEM_TIMER_PERIPHERALID1 ID_TC1_CHANNEL0
#define SYSTEM_TIMER_PERIPHERALID2 ID_TC1_CHANNEL1
/*
#define LCD_TIMER TC0
#define LCD_TIMER_CHANNEL 0
#define LCD_TIMER_COUNT_CHANNEL 1

#define LCD_TIMER_COUNTER_CLOCK_ROUTING TC_BMR_TC1XC1S_TIOA0 // Clock XC1 is TIOA0
#define LCD_TIMER_COUNTER_CLOCK         TC_CMR_TCCLKS_XC1

#define LCD_TIMER_PERIPHERAL_ID1 ID_TC0_CHANNEL0
#define LCD_TIMER_PERIPHERAL_ID2 ID_TC0_CHANNEL1

#define LCD_DATA_PORT PIOD

#define LCD_DATA_B00 0
#define LCD_DATA_B01 1
#define LCD_DATA_B02 2
#define LCD_DATA_B03 3
#define LCD_DATA_B04 4
#define LCD_DATA_B05 5
#define LCD_DATA_B06 6
#define LCD_DATA_B07 7
#define LCD_DATA_B08 8
#define LCD_DATA_B09 9
#define LCD_DATA_B10 10
#define LCD_DATA_B11 11
#define LCD_DATA_B12 12
#define LCD_DATA_B13 13
#define LCD_DATA_B14 14
#define LCD_DATA_B15 15

#define LCD_DATA_B00_bm BIT32(LCD_DATA_B00, 1)
#define LCD_DATA_B01_bm BIT32(LCD_DATA_B01, 1)
#define LCD_DATA_B02_bm BIT32(LCD_DATA_B02, 1)
#define LCD_DATA_B03_bm BIT32(LCD_DATA_B03, 1)
#define LCD_DATA_B04_bm BIT32(LCD_DATA_B04, 1)
#define LCD_DATA_B05_bm BIT32(LCD_DATA_B05, 1)
#define LCD_DATA_B06_bm BIT32(LCD_DATA_B06, 1)
#define LCD_DATA_B07_bm BIT32(LCD_DATA_B07, 1)
#define LCD_DATA_B08_bm BIT32(LCD_DATA_B08, 1)
#define LCD_DATA_B09_bm BIT32(LCD_DATA_B09, 1)
#define LCD_DATA_B10_bm BIT32(LCD_DATA_B10, 1)
#define LCD_DATA_B11_bm BIT32(LCD_DATA_B11, 1)
#define LCD_DATA_B12_bm BIT32(LCD_DATA_B12, 1)
#define LCD_DATA_B13_bm BIT32(LCD_DATA_B13, 1)
#define LCD_DATA_B14_bm BIT32(LCD_DATA_B14, 1)
#define LCD_DATA_B15_bm BIT32(LCD_DATA_B15, 1)


#define LCD_DATA_MASK (LCD_DATA_B00_bm | LCD_DATA_B01_bm | LCD_DATA_B02_bm | LCD_DATA_B03_bm | LCD_DATA_B04_bm | LCD_DATA_B05_bm | LCD_DATA_B06_bm | LCD_DATA_B07_bm | LCD_DATA_B08_bm | LCD_DATA_B09_bm | LCD_DATA_B10_bm | LCD_DATA_B11_bm | LCD_DATA_B12_bm | LCD_DATA_B13_bm | LCD_DATA_B14_bm | LCD_DATA_B15_bm)

#define EB(data, src, dst) (((src < dst) ? (uint32_t(data) << (dst-src)) : (uint32_t(data) >> (src-dst))) & (0x1<<dst))


#define LCD_RESET PIN17_bm
#define LCD_CS PIN19_bm
#define LCD_RD PIN18_bm
#define LCD_RS PIN16_bm

#define LCD_CTRL_PORT  e_DigitalPortID_D
#define LCD_CTRL_PORT2 e_DigitalPortID_D

#define LCD_WR_PORT    e_DigitalPortID_A
#define LCD_WR PIN0_bm

#define LCD_LEDA PIN21_bm
#define LCD_LEDA_LOW()  LCD_CTRL_PORT2->PIO_CODR = LCD_LEDA
#define LCD_LEDA_HIGH() LCD_CTRL_PORT2->PIO_SODR = LCD_LEDA
*/

#define LCD_TP_SDA_bm     PIN3_bm
#define LCD_TP_SDA_Pin    DigitalPin(e_DigitalPortID_A, PIN3_bp)
#define LCD_TP_SDA_port   e_DigitalPortID_A
#define LCD_TP_SDA_perif  DigitalPinPeripheralID::A

#define LCD_TP_SCL_bm     PIN4_bm
#define LCD_TP_SCL_Pin    DigitalPin(e_DigitalPortID_A, PIN4_bp)
#define LCD_TP_SCL_port   e_DigitalPortID_A
#define LCD_TP_SCL_perif  DigitalPinPeripheralID::A

#define LCD_TP_WAKE_Pin  DigitalPin(e_DigitalPortID_A, PIN23_bp)
#define LCD_TP_INT_Pin   DigitalPin(e_DigitalPortID_A, PIN2_bp)
#define LCD_TP_RESET_Pin DigitalPin(e_DigitalPortID_A, PIN22_bp)

//#define LCD_WR_CYCLE_TIME 8

#define RGBLED_R DigitalPin(e_DigitalPortID_D, PIN1_bp)
#define RGBLED_G DigitalPin(e_DigitalPortID_D, PIN3_bp)
#define RGBLED_B DigitalPin(e_DigitalPortID_D, PIN5_bp)

#define EBI_AB2_Pin  DigitalPin(e_DigitalPortID_C, PIN20_bp)
#define EBI_AB3_Pin  DigitalPin(e_DigitalPortID_C, PIN21_bp)
#define EBI_AB4_Pin  DigitalPin(e_DigitalPortID_C, PIN22_bp)
#define EBI_AB5_Pin  DigitalPin(e_DigitalPortID_C, PIN23_bp)
#define EBI_AB6_Pin  DigitalPin(e_DigitalPortID_C, PIN24_bp)
#define EBI_AB7_Pin  DigitalPin(e_DigitalPortID_C, PIN25_bp)
#define EBI_AB8_Pin  DigitalPin(e_DigitalPortID_C, PIN26_bp)
#define EBI_AB9_Pin  DigitalPin(e_DigitalPortID_C, PIN27_bp)
#define EBI_AB10_Pin DigitalPin(e_DigitalPortID_C, PIN28_bp)
#define EBI_AB11_Pin DigitalPin(e_DigitalPortID_C, PIN29_bp)
#define EBI_AB12_Pin DigitalPin(e_DigitalPortID_D, PIN13_bp)
#define EBI_AB13_Pin DigitalPin(e_DigitalPortID_C, PIN31_bp)
#define EBI_AB14_Pin DigitalPin(e_DigitalPortID_A, PIN18_bp)

#define EBI_DQMH_Pin DigitalPin(e_DigitalPortID_D, PIN15_bp)
#define EBI_DQML_Pin DigitalPin(e_DigitalPortID_C, PIN18_bp)

#define EBI_SDRAM_BANK0_Pin DigitalPin(e_DigitalPortID_A, PIN20_bp)
#define EBI_SDRAM_BANK1_Pin DigitalPin(e_DigitalPortID_A, PIN0_bp)

#define EBI_SDRAM_CAS_Pin   DigitalPin(e_DigitalPortID_D, PIN17_bp)
#define EBI_SDRAM_CKE_Pin   DigitalPin(e_DigitalPortID_D, PIN14_bp)
#define EBI_SDRAM_CLK_Pin   DigitalPin(e_DigitalPortID_D, PIN23_bp)
#define EBI_SDRAM_CS_Pin    DigitalPin(e_DigitalPortID_C, PIN15_bp)
#define EBI_SDRAM_RAS_Pin   DigitalPin(e_DigitalPortID_D, PIN16_bp)
#define EBI_SDRAM_WE_Pin    DigitalPin(e_DigitalPortID_D, PIN29_bp)
#define EBI_DB0_Pin         DigitalPin(e_DigitalPortID_C, PIN0_bp)
#define EBI_DB1_Pin         DigitalPin(e_DigitalPortID_C, PIN1_bp)
#define EBI_DB2_Pin         DigitalPin(e_DigitalPortID_C, PIN2_bp)
#define EBI_DB3_Pin         DigitalPin(e_DigitalPortID_C, PIN3_bp)
#define EBI_DB4_Pin         DigitalPin(e_DigitalPortID_C, PIN4_bp)
#define EBI_DB5_Pin         DigitalPin(e_DigitalPortID_C, PIN5_bp)
#define EBI_DB6_Pin         DigitalPin(e_DigitalPortID_C, PIN6_bp)
#define EBI_DB7_Pin         DigitalPin(e_DigitalPortID_C, PIN7_bp)
#define EBI_DB8_Pin         DigitalPin(e_DigitalPortID_E, PIN0_bp)
#define EBI_DB9_Pin         DigitalPin(e_DigitalPortID_E, PIN1_bp)
#define EBI_DB10_Pin        DigitalPin(e_DigitalPortID_E, PIN2_bp)
#define EBI_DB11_Pin        DigitalPin(e_DigitalPortID_E, PIN3_bp)
#define EBI_DB12_Pin        DigitalPin(e_DigitalPortID_E, PIN4_bp)
#define EBI_DB13_Pin        DigitalPin(e_DigitalPortID_E, PIN5_bp)
#define EBI_DB14_Pin        DigitalPin(e_DigitalPortID_A, PIN15_bp)
#define EBI_DB15_Pin        DigitalPin(e_DigitalPortID_A, PIN16_bp)
#define EBI_LCD_CS_Pin      DigitalPin(e_DigitalPortID_C, PIN12_bp)
#define EBI_LCD_RD_Pin      DigitalPin(e_DigitalPortID_C, PIN11_bp)
#define EBI_LCD_RS_Pin      DigitalPin(e_DigitalPortID_C, PIN19_bp)
#define EBI_LCD_WR_Pin      DigitalPin(e_DigitalPortID_C, PIN8_bp)

#define EBI_AB2_Periph          DigitalPinPeripheralID::A
#define EBI_AB3_Periph          DigitalPinPeripheralID::A
#define EBI_AB4_Periph          DigitalPinPeripheralID::A
#define EBI_AB5_Periph          DigitalPinPeripheralID::A
#define EBI_AB6_Periph          DigitalPinPeripheralID::A
#define EBI_AB7_Periph          DigitalPinPeripheralID::A
#define EBI_AB8_Periph          DigitalPinPeripheralID::A
#define EBI_AB9_Periph          DigitalPinPeripheralID::A
#define EBI_AB10_Periph         DigitalPinPeripheralID::A
#define EBI_AB11_Periph         DigitalPinPeripheralID::A
#define EBI_AB12_Periph         DigitalPinPeripheralID::C
#define EBI_AB13_Periph         DigitalPinPeripheralID::A
#define EBI_AB14_Periph         DigitalPinPeripheralID::C
#define EBI_DQMH_Periph         DigitalPinPeripheralID::C
#define EBI_DQML_Periph         DigitalPinPeripheralID::A
#define EBI_SDRAM_BANK0_Periph  DigitalPinPeripheralID::C
#define EBI_SDRAM_BANK1_Periph  DigitalPinPeripheralID::C    
#define EBI_SDRAM_CAS_Periph    DigitalPinPeripheralID::C
#define EBI_SDRAM_CKE_Periph    DigitalPinPeripheralID::C
#define EBI_SDRAM_CLK_Periph    DigitalPinPeripheralID::C
#define EBI_SDRAM_CS_Periph     DigitalPinPeripheralID::A
#define EBI_SDRAM_RAS_Periph    DigitalPinPeripheralID::C
#define EBI_SDRAM_WE_Periph     DigitalPinPeripheralID::C
#define EBI_DB0_Periph          DigitalPinPeripheralID::A
#define EBI_DB1_Periph          DigitalPinPeripheralID::A
#define EBI_DB2_Periph          DigitalPinPeripheralID::A
#define EBI_DB3_Periph          DigitalPinPeripheralID::A
#define EBI_DB4_Periph          DigitalPinPeripheralID::A
#define EBI_DB5_Periph          DigitalPinPeripheralID::A
#define EBI_DB6_Periph          DigitalPinPeripheralID::A
#define EBI_DB7_Periph          DigitalPinPeripheralID::A
#define EBI_DB8_Periph          DigitalPinPeripheralID::A
#define EBI_DB9_Periph          DigitalPinPeripheralID::A
#define EBI_DB10_Periph         DigitalPinPeripheralID::A
#define EBI_DB11_Periph         DigitalPinPeripheralID::A
#define EBI_DB12_Periph         DigitalPinPeripheralID::A
#define EBI_DB13_Periph         DigitalPinPeripheralID::A
#define EBI_DB14_Periph         DigitalPinPeripheralID::A
#define EBI_DB15_Periph         DigitalPinPeripheralID::A
#define EBI_LCD_CS_Periph       DigitalPinPeripheralID::A
#define EBI_LCD_RD_Periph       DigitalPinPeripheralID::A
#define EBI_LCD_RS_Periph       DigitalPinPeripheralID::A
#define EBI_LCD_WR_Periph       DigitalPinPeripheralID::A

#define LCD_RESET_Pin   DigitalPin(e_DigitalPortID_C, PIN10_bp)
#define LCD_INT_Pin     DigitalPin(e_DigitalPortID_C, PIN14_bp)
#define LCD_WAIT_Pin    DigitalPin(e_DigitalPortID_C, PIN9_bp)
#define LCD_BL_CTRL_Pin DigitalPin(e_DigitalPortID_D, PIN11_bp)
#define LCD_C86_Pin     DigitalPin(e_DigitalPortID_C, PIN16_bp)

struct LCDRegisters
{
    volatile uint16_t DATA;
    volatile uint16_t CMD;
};

#define LCD_REGISTERS ((LCDRegisters*)EBI_CS3_ADDR)
#define LCD_REG_DATA LCD_REGISTERS->DATA // (*((uint16_t volatile*)EBI_CS3_ADDR))
#define LCD_REG_CMD  LCD_REGISTERS->CMD // (*(((uint16_t volatile*)EBI_CS3_ADDR)+2))

class GfxDriverIO
{
public:

    static void Setup();
    
    static void LCD_RESET_LOW()  { LCD_RESET_Pin.Write(false); }
    static void LCD_RESET_HIGH() { LCD_RESET_Pin.Write(true); }

    static uint16_t LCD_READ_CMD()  { return LCD_REGISTERS->CMD; }
    static uint16_t LCD_READ_DATA() { return LCD_REGISTERS->DATA; }

    static void LCD_WRITE_CMD(uint16_t data)  { LCD_REGISTERS->CMD = data; }
    static void LCD_WRITE_DATA(uint16_t data)  { LCD_REGISTERS->DATA = data; }
};

typedef kernel::GfxDriver Display;

class SystemSetup
{
public:

};
