// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 02.04.2022 20:25

#include <string.h>

#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/HAL/PeripheralMapping.h"
#include "../../../Include/Kernel/HAL/STM32/DMARequestID.h"

namespace kernel
{

struct NamedPinMuxTarget
{
    const char* Name;
    PinMuxTarget    Target;
};

#define NAMED_PINMUX(name) {#name, PINMUX_##name}

static const NamedPinMuxTarget g_MuxTargets[] =
{
    NAMED_PINMUX(TIM1_CH1_A8),

    NAMED_PINMUX(TIM2_CH1_A0),
    NAMED_PINMUX(TIM2_CH1_A5),
    NAMED_PINMUX(TIM2_CH1_A15),

    NAMED_PINMUX(TIM2_ETR_A0),
    NAMED_PINMUX(TIM2_ETR_A5),
    NAMED_PINMUX(TIM2_ETR_A15),

    NAMED_PINMUX(TIM3_CH1_A6),
    NAMED_PINMUX(TIM3_CH1_B4),
    NAMED_PINMUX(TIM3_CH1_C6),

    NAMED_PINMUX(TIM4_CH1_B6),
    NAMED_PINMUX(TIM4_CH1_D12),

    NAMED_PINMUX(TIM5_CH1_A0),
    NAMED_PINMUX(TIM5_CH1_H10),

    NAMED_PINMUX(TIM8_CH1_C6),
    NAMED_PINMUX(TIM8_CH1_I5),
    NAMED_PINMUX(TIM8_CH1_J8),

    NAMED_PINMUX(TIM8_CH1N_A7),

    NAMED_PINMUX(USART1_RX_PA10),
    NAMED_PINMUX(USART1_RX_PB7),
    NAMED_PINMUX(USART1_RX_PB15),

    NAMED_PINMUX(USART1_TX_PA9),
    NAMED_PINMUX(USART1_TX_PB6),

    NAMED_PINMUX(USART2_RX_PD6),
    NAMED_PINMUX(USART2_RX_PA3),

    NAMED_PINMUX(USART2_TX_PD5),
    NAMED_PINMUX(USART2_TX_PA2),

    NAMED_PINMUX(I2C1_SCL_PB6),
    NAMED_PINMUX(I2C1_SCL_PB8),
    NAMED_PINMUX(I2C1_SDA_PB7),

    NAMED_PINMUX(I2C2_SCL_PB10),
    NAMED_PINMUX(I2C2_SCL_PF1),
    NAMED_PINMUX(I2C2_SCL_PH4),

    NAMED_PINMUX(I2C2_SDA_PB11),
    NAMED_PINMUX(I2C2_SDA_PF0),
    NAMED_PINMUX(I2C2_SDA_PH5),

    NAMED_PINMUX(I2C2_SMBA_PB12),
    NAMED_PINMUX(I2C2_SMBA_PF2),
    NAMED_PINMUX(I2C2_SMBA_PH6),

    NAMED_PINMUX(I2C4_SCL_PB6),
    NAMED_PINMUX(I2C4_SCL_PB8),
    NAMED_PINMUX(I2C4_SCL_PD12),
    NAMED_PINMUX(I2C4_SCL_PF14),
    NAMED_PINMUX(I2C4_SCL_PH11),

    NAMED_PINMUX(I2C4_SDA_PB7),
    NAMED_PINMUX(I2C4_SDA_PB9),
    NAMED_PINMUX(I2C4_SDA_PD13),
    NAMED_PINMUX(I2C4_SDA_PF15),
    NAMED_PINMUX(I2C4_SDA_PH12),

    NAMED_PINMUX(I2C4_SMBA_PB5),
    NAMED_PINMUX(I2C4_SMBA_PD11),
    NAMED_PINMUX(I2C4_SMBA_PF13),
    NAMED_PINMUX(I2C4_SMBA_PH10),

    NAMED_PINMUX(FMC_SDNWE_PA7),
    NAMED_PINMUX(FMC_SDCKE1_PB5),
    NAMED_PINMUX(FMC_SDNE1_PB6),
    NAMED_PINMUX(FMC_NL_PB7),
    NAMED_PINMUX(FMC_SDNWE_PC0),
    NAMED_PINMUX(FMC_SDNE0_PC2),
    NAMED_PINMUX(FMC_SDCKE0_PC3),
    NAMED_PINMUX(FMC_SDNE0_PC4),
    NAMED_PINMUX(FMC_SDCKE0_PC5),
    NAMED_PINMUX(FMC_NWAIT_PC6),
    NAMED_PINMUX(FMC_NE1_PC7),
    NAMED_PINMUX(FMC_NE2_PC8),
    NAMED_PINMUX(FMC_D2_PD0),
    NAMED_PINMUX(FMC_D3_PD1),
    NAMED_PINMUX(FMC_CLK_PD3),
    NAMED_PINMUX(FMC_NOE_PD4),
    NAMED_PINMUX(FMC_NWE_PD5),
    NAMED_PINMUX(FMC_NWAIT_PD6),
    NAMED_PINMUX(FMC_NE1_PD7),
    NAMED_PINMUX(FMC_D13_PD8),
    NAMED_PINMUX(FMC_D14_PD9),
    NAMED_PINMUX(FMC_D15_PD10),
    NAMED_PINMUX(FMC_A16_PD11),
    NAMED_PINMUX(FMC_A17_PD12),
    NAMED_PINMUX(FMC_A18_PD13),
    NAMED_PINMUX(FMC_D0_PD14),
    NAMED_PINMUX(FMC_D1_PD15),
    NAMED_PINMUX(FMC_NBL0_PE0),
    NAMED_PINMUX(FMC_NBL1_PE1),
    NAMED_PINMUX(FMC_A23_PE2),
    NAMED_PINMUX(FMC_A19_PE3),
    NAMED_PINMUX(FMC_A20_PE4),
    NAMED_PINMUX(FMC_A21_PE5),
    NAMED_PINMUX(FMC_A22_PE6),
    NAMED_PINMUX(FMC_D4_PE7),
    NAMED_PINMUX(FMC_D5_PE8),
    NAMED_PINMUX(FMC_D6_PE9),
    NAMED_PINMUX(FMC_D7_PE10),
    NAMED_PINMUX(FMC_D8_PE11),
    NAMED_PINMUX(FMC_D9_PE12),
    NAMED_PINMUX(FMC_D10_PE13),
    NAMED_PINMUX(FMC_D11_PE14),
    NAMED_PINMUX(FMC_D12_PE15),
    NAMED_PINMUX(FMC_A0_PF0),
    NAMED_PINMUX(FMC_A1_PF1),
    NAMED_PINMUX(FMC_A2_PF2),
    NAMED_PINMUX(FMC_A3_PF3),
    NAMED_PINMUX(FMC_A4_PF4),
    NAMED_PINMUX(FMC_A5_PF5),
    NAMED_PINMUX(FMC_SDNRAS_PF11),
    NAMED_PINMUX(FMC_A6_PF12),
    NAMED_PINMUX(FMC_A7_PF13),
    NAMED_PINMUX(FMC_A8_PF14),
    NAMED_PINMUX(FMC_A9_PF15),
    NAMED_PINMUX(FMC_A10_PG0),
    NAMED_PINMUX(FMC_A11_PG1),
    NAMED_PINMUX(FMC_A12_PG2),
    NAMED_PINMUX(FMC_A13_PG3),
    NAMED_PINMUX(FMC_A14_BA0_PG4),
    NAMED_PINMUX(FMC_A15_BA1_PG5),
    NAMED_PINMUX(FMC_NE3_PG6),
    NAMED_PINMUX(FMC_INT_PG7),
    NAMED_PINMUX(FMC_SDCLK_PG8),
    NAMED_PINMUX(FMC_NE2_PG9),
    NAMED_PINMUX(FMC_NE3_PG10),
    NAMED_PINMUX(FMC_NE4_PG12),
    NAMED_PINMUX(FMC_A24_PG13),
    NAMED_PINMUX(FMC_A25_PG14),
    NAMED_PINMUX(FMC_SDNCAS_PG15),
    NAMED_PINMUX(FMC_SDCKE0_PH2),
    NAMED_PINMUX(FMC_SDNE0_PH3),
    NAMED_PINMUX(FMC_SDNWE_PH5),
    NAMED_PINMUX(FMC_SDNE1_PH6),
    NAMED_PINMUX(FMC_SDCKE1_PH7),
    NAMED_PINMUX(FMC_D16_PH8),
    NAMED_PINMUX(FMC_D17_PH9),
    NAMED_PINMUX(FMC_D18_PH10),
    NAMED_PINMUX(FMC_D19_PH11),
    NAMED_PINMUX(FMC_D20_PH12),
    NAMED_PINMUX(FMC_D21_PH13),
    NAMED_PINMUX(FMC_D22_PH14),
    NAMED_PINMUX(FMC_D23_PH15),
    NAMED_PINMUX(FMC_D24_PI0),
    NAMED_PINMUX(FMC_D25_PI1),
    NAMED_PINMUX(FMC_D26_PI2),
    NAMED_PINMUX(FMC_D27_PI3),
    NAMED_PINMUX(FMC_NBL2_PI4),
    NAMED_PINMUX(FMC_NBL3_PI5),
    NAMED_PINMUX(FMC_D28_PI6),
    NAMED_PINMUX(FMC_D29_PI7),
    NAMED_PINMUX(FMC_D30_PI9),
    NAMED_PINMUX(FMC_D31_PI10),

    NAMED_PINMUX(SDMMC1_D0_PC8),
    NAMED_PINMUX(SDMMC1_D1_PC9),
    NAMED_PINMUX(SDMMC1_D2_PC10),
    NAMED_PINMUX(SDMMC1_D3_PC11),
    NAMED_PINMUX(SDMMC1_D4_PB8),
    NAMED_PINMUX(SDMMC1_D5_PB9),
    NAMED_PINMUX(SDMMC1_D6_PC6),
    NAMED_PINMUX(SDMMC1_D7_PC7),
    NAMED_PINMUX(SDMMC1_CMD_PD2),
    NAMED_PINMUX(SDMMC1_CK_PC12),
    NAMED_PINMUX(SDMMC1_CKIN_PB8),
    NAMED_PINMUX(SDMMC1_CDIR_PB9),
    NAMED_PINMUX(SDMMC1_D0DIR_PC6),
    NAMED_PINMUX(SDMMC1_D123DIR_PC7),

    NAMED_PINMUX(SDMMC2_D0_PB14),
    NAMED_PINMUX(SDMMC2_D1_PB15),
    NAMED_PINMUX(SDMMC2_D2_PB3),
    NAMED_PINMUX(SDMMC2_D2_PG11),
    NAMED_PINMUX(SDMMC2_D3_PB4),
    NAMED_PINMUX(SDMMC2_D4_PB8),
    NAMED_PINMUX(SDMMC2_D5_PB9),
    NAMED_PINMUX(SDMMC2_D6_PC6),
    NAMED_PINMUX(SDMMC2_D7_PC7),
    NAMED_PINMUX(SDMMC2_CMD_PA0),
    NAMED_PINMUX(SDMMC2_CMD_PD7),
    NAMED_PINMUX(SDMMC2_CK_PC1),
    NAMED_PINMUX(SDMMC2_CK_PD6),

    NAMED_PINMUX(SPI1_NSS_PA4),
    NAMED_PINMUX(I2S1_WS_PA4),

    NAMED_PINMUX(SPI1_SCK_PA5),
    NAMED_PINMUX(I2S1_CK_PA5),

    NAMED_PINMUX(SPI1_MISO_PA6),
    NAMED_PINMUX(I2S1_SDI_PA6),

    NAMED_PINMUX(SPI1_MOSI_PA7),
    NAMED_PINMUX(I2S1_SDO_PA7),

    NAMED_PINMUX(SPI1_NSS_PA15),
    NAMED_PINMUX(I2S1_WS_PA15),

    NAMED_PINMUX(SPI1_SCK_PB3),
    NAMED_PINMUX(I2S1_CK_PB3),

    NAMED_PINMUX(SPI1_MISO_PB4),
    NAMED_PINMUX(I2S1_SDI_PB4),

    NAMED_PINMUX(SPI1_MOSI_PB5),
    NAMED_PINMUX(I2S1_SDO_PB5),

    NAMED_PINMUX(SPI1_MOSI_PD7),
    NAMED_PINMUX(I2S1_SDO_PD7),

    NAMED_PINMUX(SPI1_MISO_PG9),
    NAMED_PINMUX(I2S1_SDI_PG9),

    NAMED_PINMUX(SPI1_NSS_PG10),
    NAMED_PINMUX(I2S1_WS_PG10),

    NAMED_PINMUX(SPI1_SCK_PG11),
    NAMED_PINMUX(I2S1_CK_PG11),

    NAMED_PINMUX(SPI2_SCK_PA9),
    NAMED_PINMUX(I2S2_CK_PA9),

    NAMED_PINMUX(SPI2_NSS_PA11),
    NAMED_PINMUX(I2S2_WS_PA11),

    NAMED_PINMUX(SPI2_SCK_PA12),
    NAMED_PINMUX(I2S2_CK_PA12),

    NAMED_PINMUX(SPI2_NSS_PB4),
    NAMED_PINMUX(I2S2_WS_PB4),

    NAMED_PINMUX(SPI2_NSS_PB9),
    NAMED_PINMUX(I2S2_WS_PB9),

    NAMED_PINMUX(SPI2_SCK_PB10),
    NAMED_PINMUX(I2S2_CK_PB10),

    NAMED_PINMUX(SPI2_NSS_PB12),
    NAMED_PINMUX(I2S2_WS_PB12),

    NAMED_PINMUX(SPI2_SCK_PB13),
    NAMED_PINMUX(I2S2_CK_PB13),

    NAMED_PINMUX(SPI2_MISO_PB14),
    NAMED_PINMUX(I2S2_SDI_PB14),

    NAMED_PINMUX(SPI2_MOSI_PB15),
    NAMED_PINMUX(I2S2_SDO_PB15),

    NAMED_PINMUX(SPI2_MOSI_PC1),
    NAMED_PINMUX(I2S2_SDO_PC1),

    NAMED_PINMUX(SPI2_MISO_PC2),
    NAMED_PINMUX(I2S2_SDI_PC2),

    NAMED_PINMUX(SPI2_MOSI_PC3),
    NAMED_PINMUX(I2S2_SDO_PC3),

    NAMED_PINMUX(SPI2_SCK_PD3),
    NAMED_PINMUX(I2S2_CK_PD3),

    NAMED_PINMUX(SPI2_NSS_PI0),
    NAMED_PINMUX(I2S2_WS_PI0),

    NAMED_PINMUX(SPI2_SCK_PI1),
    NAMED_PINMUX(I2S2_CK_PI1),

    NAMED_PINMUX(SPI2_MISO_PI2),
    NAMED_PINMUX(I2S2_SDI_PI2),

    NAMED_PINMUX(SPI2_MOSI_PI3),
    NAMED_PINMUX(I2S2_SDO_PI3),

    NAMED_PINMUX(SPI3_NSS_PA4),
    NAMED_PINMUX(I2S3_WS_PA4),

    NAMED_PINMUX(SPI3_NSS_PA15),
    NAMED_PINMUX(I2S3_WS_PA15),

    NAMED_PINMUX(SPI3_MOSI_PB2),
    NAMED_PINMUX(I2S3_SDO_PB2),

    NAMED_PINMUX(SPI3_SCK_PB3),
    NAMED_PINMUX(I2S3_CK_PB3),

    NAMED_PINMUX(SPI3_MISO_PB4),
    NAMED_PINMUX(I2S3_SDI_PB4),

    NAMED_PINMUX(SPI3_MOSI_PB5),
    NAMED_PINMUX(I2S3_SDO_PB5),

    NAMED_PINMUX(SPI3_SCK_PC10),
    NAMED_PINMUX(I2S3_CK_PC10),

    NAMED_PINMUX(SPI3_MISO_PC11),
    NAMED_PINMUX(I2S3_SDI_PC11),

    NAMED_PINMUX(SPI3_MOSI_PC12),
    NAMED_PINMUX(I2S3_SDO_PC12),

    NAMED_PINMUX(SPI3_MOSI_PD6),
    NAMED_PINMUX(I2S3_SDO_PD6),

    NAMED_PINMUX(SPI4_SCK_PE2),
    NAMED_PINMUX(SPI4_NSS_PE4),
    NAMED_PINMUX(SPI4_MISO_PE5),
    NAMED_PINMUX(SPI4_MOSI_PE6),
    NAMED_PINMUX(SPI4_NSS_PE11),
    NAMED_PINMUX(SPI4_SCK_PE12),
    NAMED_PINMUX(SPI4_MISO_PE13),
    NAMED_PINMUX(SPI4_MOSI_PE14),

    NAMED_PINMUX(SPI5_NSS_PF6),
    NAMED_PINMUX(SPI5_SCK_PF7),
    NAMED_PINMUX(SPI5_MISO_PF8),
    NAMED_PINMUX(SPI5_MOSI_PF9),
    NAMED_PINMUX(SPI5_MOSI_PF11),
    NAMED_PINMUX(SPI5_NSS_PH5),
    NAMED_PINMUX(SPI5_SCK_PH6),
    NAMED_PINMUX(SPI5_MISO_PH7),
    NAMED_PINMUX(SPI5_MOSI_PJ10),
    NAMED_PINMUX(SPI5_MISO_PJ11),
    NAMED_PINMUX(SPI5_SCK_PK0),
    NAMED_PINMUX(SPI5_NSS_PK1),

    NAMED_PINMUX(SPI6_NSS_PA4),
    NAMED_PINMUX(SPI6_SCK_PA5),
    NAMED_PINMUX(SPI6_MISO_PA6),
    NAMED_PINMUX(SPI6_MOSI_PA7),
    NAMED_PINMUX(SPI6_NSS_PA15),
    NAMED_PINMUX(SPI6_SCK_PB3),
    NAMED_PINMUX(SPI6_MISO_PB4),
    NAMED_PINMUX(SPI6_MOSI_PB5),
    NAMED_PINMUX(SPI6_NSS_PG8),
    NAMED_PINMUX(SPI6_MISO_PG12),
    NAMED_PINMUX(SPI6_SCK_PG13),
    NAMED_PINMUX(SPI6_MOSI_PG14)
};

PinMuxTarget pinmux_target_from_name(const char* name)
{
    for (size_t i = 0; i < ARRAY_COUNT(g_MuxTargets); ++i)
    {
        if (strcmp(name, g_MuxTargets[i].Name) == 0)
        {
            return g_MuxTargets[i].Target;
        }
    }
    return PINMUX_NONE;
}

DigitalPinID pin_id_from_name(const char* name)
{
    if (name[0] < 'A' || name[0] > ('A' + e_DigitalPortID_Count) || !isdigit(name[1]) || (name[2] != '\0' && (name[2] < '0' || name[2] > '5')))
    {
        if (strcmp(name, "None") != 0) {
            printf("ERROR: pin_id_from_name() failed to convert '%s'\n", name);
        }
        return DigitalPinID::None;
    }
    DigitalPortID   portID = DigitalPortID(name[0] - 'A');
    int32_t         pinIndex = atoi(name + 1);

    return DigitalPinID(MAKE_DIGITAL_PIN_ID(portID, pinIndex));
}

HWTimerID timer_id_from_name(const char* name)
{
    if (name[0] != 'T' || name[1] != 'I' || name[2] != 'M')
    {
        printf("ERROR: timer_id_from_name() failed to convert '%s'\n", name);
        return HWTimerID::None;
    }
    return HWTimerID(atoi(name + 3));
}

TIM_TypeDef* get_timer_from_id(HWTimerID timerID)
{
    switch (timerID)
    {
        case HWTimerID::Timer1:     return TIM1;
        case HWTimerID::Timer2:     return TIM2;
        case HWTimerID::Timer3:     return TIM3;
        case HWTimerID::Timer4:     return TIM4;
        case HWTimerID::Timer5:     return TIM5;
        case HWTimerID::Timer6:     return TIM6;
        case HWTimerID::Timer7:     return TIM7;
        case HWTimerID::Timer8:     return TIM8;
        case HWTimerID::Timer12:    return TIM12;
        case HWTimerID::Timer13:    return TIM13;
        case HWTimerID::Timer14:    return TIM14;
        case HWTimerID::Timer15:    return TIM15;
        case HWTimerID::Timer16:    return TIM16;
        case HWTimerID::Timer17:    return TIM17;
        default:
            printf("ERROR: get_timer_from_id() unknown timer '%d'\n", int(timerID));
            return nullptr;
    }
}

IRQn_Type get_timer_irq(HWTimerID timerID, HWTimerIRQType irqType)
{
    switch (timerID)
    {
        case HWTimerID::Timer1:
            switch (irqType)
            {
                case HWTimerIRQType::Break:             return TIM1_BRK_IRQn;
                case HWTimerIRQType::Update:            return TIM1_UP_IRQn;
                case HWTimerIRQType::Trigger:           return TIM1_TRG_COM_IRQn;
                case HWTimerIRQType::Commutation:       return TIM1_TRG_COM_IRQn;
                case HWTimerIRQType::CaptureCompare:    return TIM1_CC_IRQn;
                default:    return IRQn_Type(IRQ_COUNT);
            }
        case HWTimerID::Timer2:     return TIM2_IRQn;
        case HWTimerID::Timer3:     return TIM3_IRQn;
        case HWTimerID::Timer4:     return TIM4_IRQn;
        case HWTimerID::Timer5:     return TIM5_IRQn;
        case HWTimerID::Timer6:     return TIM6_DAC_IRQn;
        case HWTimerID::Timer7:     return TIM7_IRQn;
        case HWTimerID::Timer8:
            switch (irqType)
            {
                case HWTimerIRQType::Break:             return TIM8_BRK_TIM12_IRQn;
                case HWTimerIRQType::Update:            return TIM8_UP_TIM13_IRQn;
                case HWTimerIRQType::Trigger:           return TIM8_TRG_COM_TIM14_IRQn;
                case HWTimerIRQType::Commutation:       return TIM8_TRG_COM_TIM14_IRQn;
                case HWTimerIRQType::CaptureCompare:    return TIM8_CC_IRQn;
                default:    return IRQn_Type(IRQ_COUNT);
            }
        case HWTimerID::Timer12:    return TIM8_BRK_TIM12_IRQn;
        case HWTimerID::Timer13:    return TIM8_UP_TIM13_IRQn;
        case HWTimerID::Timer14:    return TIM8_TRG_COM_TIM14_IRQn;
        case HWTimerID::Timer15:    return TIM15_IRQn;
        case HWTimerID::Timer16:    return TIM16_IRQn;
        case HWTimerID::Timer17:    return TIM17_IRQn;
        default:
            printf("ERROR: get_timer_irq() unknown timer '%d'\n", int(timerID));
            return IRQn_Type(IRQ_COUNT);
    }
}

USARTID usart_id_from_name(const char* name)
{
    if (strcmp(name, "LPUART1") == 0) { return USARTID::LPUART_1; }
    else if (strcmp(name, "USART1") == 0) { return USARTID::USART_1; }
    else if (strcmp(name, "USART2") == 0) { return USARTID::USART_2; }
    else if (strcmp(name, "USART3") == 0) { return USARTID::USART_3; }
    else if (strcmp(name, "UART4") == 0)  { return USARTID::UART_4; }
    else if (strcmp(name, "UART5") == 0)  { return USARTID::UART_5; }
    else if (strcmp(name, "USART6") == 0) { return USARTID::USART_6; }
    else if (strcmp(name, "UART7") == 0)  { return USARTID::UART_7; }
    else if (strcmp(name, "UART8") == 0)  { return USARTID::UART_8; }
    else return USARTID::None;
}

USART_TypeDef* get_usart_from_id(USARTID id)
{
    switch (id)
    {
        case USARTID::LPUART_1: return LPUART1;
        case USARTID::USART_1:  return USART1;
        case USARTID::USART_2:  return USART2;
        case USARTID::USART_3:  return USART3;
        case USARTID::UART_4:   return UART4;
        case USARTID::UART_5:   return UART5;
        case USARTID::USART_6:  return USART6;
        case USARTID::UART_7:   return UART7;
        case USARTID::UART_8:   return UART8;
        default:    return nullptr;
    }
}

IRQn_Type get_usart_irq(USARTID id)
{
    switch (id)
    {
        case USARTID::LPUART_1: return LPUART1_IRQn;
        case USARTID::USART_1:  return USART1_IRQn;
        case USARTID::USART_2:  return USART2_IRQn;
        case USARTID::USART_3:  return USART3_IRQn;
        case USARTID::UART_4:   return UART4_IRQn;
        case USARTID::UART_5:   return UART5_IRQn;
        case USARTID::USART_6:  return USART6_IRQn;
        case USARTID::UART_7:   return UART7_IRQn;
        case USARTID::UART_8:   return UART8_IRQn;
        default:    return IRQn_Type(IRQ_COUNT);
    }
}

bool get_usart_dma_requests(USARTID id, DMAMUX_REQUEST& rx, DMAMUX_REQUEST& tx)
{
    switch (id)
    {
        case USARTID::LPUART_1:
            rx = DMAMUX_REQUEST::BDMA_REQ_LPUART1_RX;
            tx = DMAMUX_REQUEST::BDMA_REQ_LPUART1_TX;
            return true;
        case USARTID::USART_1:
            rx = DMAMUX_REQUEST::REQ_USART1_RX;
            tx = DMAMUX_REQUEST::REQ_USART1_TX;
            return true;
        case USARTID::USART_2:
            rx = DMAMUX_REQUEST::REQ_USART2_RX;
            tx = DMAMUX_REQUEST::REQ_USART2_TX;
            return true;
        case USARTID::USART_3:
            rx = DMAMUX_REQUEST::REQ_USART3_RX;
            tx = DMAMUX_REQUEST::REQ_USART3_TX;
            return true;
        case USARTID::UART_4:
            rx = DMAMUX_REQUEST::REQ_UART4_RX;
            tx = DMAMUX_REQUEST::REQ_UART4_TX;
            return true;
        case USARTID::UART_5:
            rx = DMAMUX_REQUEST::REQ_UART5_RX;
            tx = DMAMUX_REQUEST::REQ_UART5_TX;
            return true;
        case USARTID::USART_6:
            rx = DMAMUX_REQUEST::REQ_USART6_RX;
            tx = DMAMUX_REQUEST::REQ_USART6_TX;
            return true;
        case USARTID::UART_7:
            rx = DMAMUX_REQUEST::REQ_UART7_RX;
            tx = DMAMUX_REQUEST::REQ_UART7_TX;
            return true;
        case USARTID::UART_8:
            rx = DMAMUX_REQUEST::REQ_UART8_RX;
            tx = DMAMUX_REQUEST::REQ_UART8_TX;
            return true;
        default:    return false;
    }
}

I2CID i2c_id_from_name(const char* name)
{
    if (strncmp(name, "I2C", 3) == 0 && name[3] >= '1' && name[3] <= '4') {
        return I2CID(int(name[3] - '0'));
    } else {
        return I2CID::None;
    }
}

I2C_TypeDef* get_i2c_from_id(I2CID id)
{
    switch (id)
    {
        case kernel::I2CID::I2C_1:  return I2C1;
        case kernel::I2CID::I2C_2:  return I2C2;
        case kernel::I2CID::I2C_3:  return I2C3;
        case kernel::I2CID::I2C_4:  return I2C4;
        default: return nullptr;
    }
}


IRQn_Type get_i2c_irq(I2CID id, I2CIRQType type)
{
    if (type == I2CIRQType::Event)
    {
        switch (id)
        {
            case kernel::I2CID::I2C_1:  return I2C1_EV_IRQn;
            case kernel::I2CID::I2C_2:  return I2C2_EV_IRQn;
            case kernel::I2CID::I2C_3:  return I2C3_EV_IRQn;
            case kernel::I2CID::I2C_4:  return I2C4_EV_IRQn;
            default: break;
        }
    }
    else if (type == I2CIRQType::Error)
    {
        switch (id)
        {
            case kernel::I2CID::I2C_1:  return I2C1_ER_IRQn;
            case kernel::I2CID::I2C_2:  return I2C2_ER_IRQn;
            case kernel::I2CID::I2C_3:  return I2C3_ER_IRQn;
            case kernel::I2CID::I2C_4:  return I2C4_ER_IRQn;
            default: break;
        }
    }
        
    return IRQn_Type(IRQ_COUNT);
}


SPIID spi_id_from_name(const char* name)
{
    if (strncmp(name, "SPI", 3) == 0 && name[3] >= '1' && name[3] <= '6') {
        return SPIID(int(name[3] - '0'));
    } else {
        return SPIID::None;
    }
}

SPI_TypeDef* get_spi_from_id(SPIID id)
{
    switch (id)
    {
        case kernel::SPIID::SPI_1:  return SPI1;
        case kernel::SPIID::SPI_2:  return SPI2;
        case kernel::SPIID::SPI_3:  return SPI3;
        case kernel::SPIID::SPI_4:  return SPI4;
        case kernel::SPIID::SPI_5:  return SPI5;
        case kernel::SPIID::SPI_6:  return SPI6;
        default: return nullptr;
    }
}


IRQn_Type get_spi_irq(SPIID id)
{
    switch (id)
    {
        case kernel::SPIID::SPI_1:  return SPI1_IRQn;
        case kernel::SPIID::SPI_2:  return SPI2_IRQn;
        case kernel::SPIID::SPI_3:  return SPI3_IRQn;
        case kernel::SPIID::SPI_4:  return SPI4_IRQn;
        case kernel::SPIID::SPI_5:  return SPI5_IRQn;
        case kernel::SPIID::SPI_6:  return SPI6_IRQn;
        default: break;
    }
    return IRQn_Type(IRQ_COUNT);
}

bool get_spi_dma_requests(SPIID id, DMAMUX_REQUEST& rx, DMAMUX_REQUEST& tx)
{
    switch (id)
    {
        case SPIID::SPI_1:
            rx = DMAMUX_REQUEST::REQ_SPI1_RX;
            tx = DMAMUX_REQUEST::REQ_SPI1_TX;
            return true;
        case SPIID::SPI_2:
            rx = DMAMUX_REQUEST::REQ_SPI2_RX;
            tx = DMAMUX_REQUEST::REQ_SPI2_TX;
            return true;
        case SPIID::SPI_3:
            rx = DMAMUX_REQUEST::REQ_SPI3_RX;
            tx = DMAMUX_REQUEST::REQ_SPI3_TX;
            return true;
        case SPIID::SPI_4:
            rx = DMAMUX_REQUEST::REQ_SPI4_RX;
            tx = DMAMUX_REQUEST::REQ_SPI4_TX;
            return true;
        case SPIID::SPI_5:
            rx = DMAMUX_REQUEST::REQ_SPI5_RX;
            tx = DMAMUX_REQUEST::REQ_SPI5_TX;
            return true;
        case SPIID::SPI_6:
            rx = DMAMUX_REQUEST::BDMA_REQ_SPI6_RX;
            tx = DMAMUX_REQUEST::BDMA_REQ_SPI6_TX;
            return true;
        default:    return false;
    }
}

} // namespace kernel
