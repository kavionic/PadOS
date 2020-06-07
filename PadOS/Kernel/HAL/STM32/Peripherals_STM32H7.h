// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 07.06.2020 14:25:38

#pragma once
#include <stm32h743xx.h>
#include "Kernel/HAL/DigitalPort.h"

static constexpr IRQn_Type INVALID_IRQ = IRQn_Type(0xffff);

inline IRQn_Type get_peripheral_irq(DigitalPinID portPin)
{
	uint32_t pinNum = DIGITAL_PIN_ID_PIN(portPin);
	switch (pinNum)
	{
		case 0: return EXTI0_IRQn;
		case 1: return EXTI1_IRQn;
		case 2: return EXTI2_IRQn;
		case 3: return EXTI3_IRQn;
		case 4: return EXTI4_IRQn;
		default:
			if (pinNum <= 9) {
				return EXTI9_5_IRQn;
			} else if (pinNum <= 15) {
				return EXTI15_10_IRQn;
			} else {
				return INVALID_IRQ;
			}
	}
}


#if 0
WWDG_IRQn
PVD_AVD_IRQn
TAMP_STAMP_IRQn
RTC_WKUP_IRQn
FLASH_IRQn
RCC_IRQn
DMA1_Stream0_IRQn
DMA1_Stream1_IRQn
DMA1_Stream2_IRQn
DMA1_Stream3_IRQn
DMA1_Stream4_IRQn
DMA1_Stream5_IRQn
DMA1_Stream6_IRQn
ADC_IRQn
FDCAN1_IT0_IRQn
FDCAN2_IT0_IRQn
FDCAN1_IT1_IRQn
FDCAN2_IT1_IRQn
TIM1_BRK_IRQn
TIM1_UP_IRQn
TIM1_TRG_COM_IRQn
TIM1_CC_IRQn
TIM2_IRQn
TIM3_IRQn
TIM4_IRQn
I2C1_EV_IRQn
I2C1_ER_IRQn
I2C2_EV_IRQn
I2C2_ER_IRQn
SPI1_IRQn
SPI2_IRQn
USART1_IRQn
USART2_IRQn
USART3_IRQn
RTC_Alarm_IRQn
TIM8_BRK_TIM12_IRQn
TIM8_UP_TIM13_IRQn
TIM8_TRG_COM_TIM14_IRQn
TIM8_CC_IRQn
DMA1_Stream7_IRQn
FMC_IRQn
SDMMC1_IRQn
TIM5_IRQn
SPI3_IRQn
UART4_IRQn
UART5_IRQn
TIM6_DAC_IRQn
TIM7_IRQn
DMA2_Stream0_IRQn
DMA2_Stream1_IRQn
DMA2_Stream2_IRQn
DMA2_Stream3_IRQn
DMA2_Stream4_IRQn
ETH_IRQn
ETH_WKUP_IRQn
FDCAN_CAL_IRQn
DMA2_Stream5_IRQn
DMA2_Stream6_IRQn
DMA2_Stream7_IRQn
USART6_IRQn
I2C3_EV_IRQn
I2C3_ER_IRQn
OTG_HS_EP1_OUT_IRQn
OTG_HS_EP1_IN_IRQn
OTG_HS_WKUP_IRQn
OTG_HS_IRQn
DCMI_IRQn
RNG_IRQn
FPU_IRQn
UART7_IRQn
UART8_IRQn
SPI4_IRQn
SPI5_IRQn
SPI6_IRQn
SAI1_IRQn
LTDC_IRQn
LTDC_ER_IRQn
DMA2D_IRQn
SAI2_IRQn
QUADSPI_IRQn
LPTIM1_IRQn
CEC_IRQn
I2C4_EV_IRQn
I2C4_ER_IRQn
SPDIF_RX_IRQn
OTG_FS_EP1_OUT_IRQn
OTG_FS_EP1_IN_IRQn
OTG_FS_WKUP_IRQn
OTG_FS_IRQn
DMAMUX1_OVR_IRQn
HRTIM1_Master_IRQn
HRTIM1_TIMA_IRQn
HRTIM1_TIMB_IRQn
HRTIM1_TIMC_IRQn
HRTIM1_TIMD_IRQn
HRTIM1_TIME_IRQn
HRTIM1_FLT_IRQn
DFSDM1_FLT0_IRQn
DFSDM1_FLT1_IRQn
DFSDM1_FLT2_IRQn
DFSDM1_FLT3_IRQn
SAI3_IRQn
SWPMI1_IRQn
TIM15_IRQn
TIM16_IRQn
TIM17_IRQn
MDIOS_WKUP_IRQn
MDIOS_IRQn
JPEG_IRQn
MDMA_IRQn
SDMMC2_IRQn
HSEM1_IRQn
ADC3_IRQn
DMAMUX2_OVR_IRQn
BDMA_Channel0_IRQn
BDMA_Channel1_IRQn
BDMA_Channel2_IRQn
BDMA_Channel3_IRQn
BDMA_Channel4_IRQn
BDMA_Channel5_IRQn
BDMA_Channel6_IRQn
BDMA_Channel7_IRQn
COMP_IRQn
LPTIM2_IRQn
LPTIM3_IRQn
LPTIM4_IRQn
LPTIM5_IRQn
LPUART1_IRQn
CRS_IRQn
ECC_IRQn
SAI4_IRQn
WAKEUP_PIN_IRQn
#endif
