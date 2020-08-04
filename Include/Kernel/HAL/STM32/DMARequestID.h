#pragma once

namespace kernel
{

enum class DMAMUX1_REQUEST : int
{
	REQ_MEM2MEM        =  0,  // memory to memory transfer
	REQ_GENERATOR0     =  1,  // DMAMUX1 request generator 0
	REQ_GENERATOR1     =  2,  // DMAMUX1 request generator 1
	REQ_GENERATOR2     =  3,  // DMAMUX1 request generator 2
	REQ_GENERATOR3     =  4,  // DMAMUX1 request generator 3
	REQ_GENERATOR4     =  5,  // DMAMUX1 request generator 4
	REQ_GENERATOR5     =  6,  // DMAMUX1 request generator 5
	REQ_GENERATOR6     =  7,  // DMAMUX1 request generator 6
	REQ_GENERATOR7     =  8,  // DMAMUX1 request generator 7
	REQ_ADC1           =  9,  // DMAMUX1 ADC1 request
	REQ_ADC2           =  10, // DMAMUX1 ADC2 request
	REQ_TIM1_CH1       =  11, // DMAMUX1 TIM1 CH1 request
	REQ_TIM1_CH2       =  12, // DMAMUX1 TIM1 CH2 request
	REQ_TIM1_CH3       =  13, // DMAMUX1 TIM1 CH3 request
	REQ_TIM1_CH4       =  14, // DMAMUX1 TIM1 CH4 request
	REQ_TIM1_UP        =  15, // DMAMUX1 TIM1 UP request
	REQ_TIM1_TRIG      =  16, // DMAMUX1 TIM1 TRIG request
	REQ_TIM1_COM       =  17, // DMAMUX1 TIM1 COM request
	REQ_TIM2_CH1       =  18, // DMAMUX1 TIM2 CH1 request
	REQ_TIM2_CH2       =  19, // DMAMUX1 TIM2 CH2 request
	REQ_TIM2_CH3       =  20, // DMAMUX1 TIM2 CH3 request
	REQ_TIM2_CH4       =  21, // DMAMUX1 TIM2 CH4 request
	REQ_TIM2_UP        =  22, // DMAMUX1 TIM2 UP request
	REQ_TIM3_CH1       =  23, // DMAMUX1 TIM3 CH1 request
	REQ_TIM3_CH2       =  24, // DMAMUX1 TIM3 CH2 request
	REQ_TIM3_CH3       =  25, // DMAMUX1 TIM3 CH3 request
	REQ_TIM3_CH4       =  26, // DMAMUX1 TIM3 CH4 request
	REQ_TIM3_UP        =  27, // DMAMUX1 TIM3 UP request
	REQ_TIM3_TRIG      =  28, // DMAMUX1 TIM3 TRIG request
	REQ_TIM4_CH1       =  29, // DMAMUX1 TIM4 CH1 request
	REQ_TIM4_CH2       =  30, // DMAMUX1 TIM4 CH2 request
	REQ_TIM4_CH3       =  31, // DMAMUX1 TIM4 CH3 request
	REQ_TIM4_UP        =  32, // DMAMUX1 TIM4 UP request
	REQ_I2C1_RX        =  33, // DMAMUX1 I2C1 RX request
	REQ_I2C1_TX        =  34, // DMAMUX1 I2C1 TX request
	REQ_I2C2_RX        =  35, // DMAMUX1 I2C2 RX request
	REQ_I2C2_TX        =  36, // DMAMUX1 I2C2 TX request
	REQ_SPI1_RX        =  37, // DMAMUX1 SPI1 RX request
	REQ_SPI1_TX        =  38, // DMAMUX1 SPI1 TX request
	REQ_SPI2_RX        =  39, // DMAMUX1 SPI2 RX request
	REQ_SPI2_TX        =  40, // DMAMUX1 SPI2 TX request
	REQ_USART1_RX      =  41, // DMAMUX1 USART1 RX request
	REQ_USART1_TX      =  42, // DMAMUX1 USART1 TX request
	REQ_USART2_RX      =  43, // DMAMUX1 USART2 RX request
	REQ_USART2_TX      =  44, // DMAMUX1 USART2 TX request
	REQ_USART3_RX      =  45, // DMAMUX1 USART3 RX request
	REQ_USART3_TX      =  46, // DMAMUX1 USART3 TX request
	REQ_TIM8_CH1       =  47, // DMAMUX1 TIM8 CH1 request
	REQ_TIM8_CH2       =  48, // DMAMUX1 TIM8 CH2 request
	REQ_TIM8_CH3       =  49, // DMAMUX1 TIM8 CH3 request
	REQ_TIM8_CH4       =  50, // DMAMUX1 TIM8 CH4 request
	REQ_TIM8_UP        =  51, // DMAMUX1 TIM8 UP request
	REQ_TIM8_TRIG      =  52, // DMAMUX1 TIM8 TRIG request
	REQ_TIM8_COM       =  53, // DMAMUX1 TIM8 COM request
	REQ_TIM5_CH1       =  55, // DMAMUX1 TIM5 CH1 request
	REQ_TIM5_CH2       =  56, // DMAMUX1 TIM5 CH2 request
	REQ_TIM5_CH3       =  57, // DMAMUX1 TIM5 CH3 request
	REQ_TIM5_CH4       =  58, // DMAMUX1 TIM5 CH4 request
	REQ_TIM5_UP        =  59, // DMAMUX1 TIM5 UP request
	REQ_TIM5_TRIG      =  60, // DMAMUX1 TIM5 TRIG request
	REQ_SPI3_RX        =  61, // DMAMUX1 SPI3 RX request
	REQ_SPI3_TX        =  62, // DMAMUX1 SPI3 TX request
	REQ_UART4_RX       =  63, // DMAMUX1 UART4 RX request
	REQ_UART4_TX       =  64, // DMAMUX1 UART4 TX request
	REQ_UART5_RX       =  65, // DMAMUX1 UART5 RX request
	REQ_UART5_TX       =  66, // DMAMUX1 UART5 TX request
	REQ_DAC1_CH1       =  67, // DMAMUX1 DAC1 Channel 1 request
	REQ_DAC1_CH2       =  68, // DMAMUX1 DAC1 Channel 2 request
	REQ_TIM6_UP        =  69, // DMAMUX1 TIM6 UP request
	REQ_TIM7_UP        =  70, // DMAMUX1 TIM7 UP request
	REQ_USART6_RX      =  71, // DMAMUX1 USART6 RX request
	REQ_USART6_TX      =  72, // DMAMUX1 USART6 TX request
	REQ_I2C3_RX        =  73, // DMAMUX1 I2C3 RX request
	REQ_I2C3_TX        =  74, // DMAMUX1 I2C3 TX request
	REQ_DCMI           =  75, // DMAMUX1 DCMI request
	REQ_CRYP_IN        =  76, // DMAMUX1 CRYP IN request
	REQ_CRYP_OUT       =  77, // DMAMUX1 CRYP OUT request
	REQ_HASH_IN        =  78, // DMAMUX1 HASH IN request
	REQ_UART7_RX       =  79, // DMAMUX1 UART7 RX request
	REQ_UART7_TX       =  80, // DMAMUX1 UART7 TX request
	REQ_UART8_RX       =  81, // DMAMUX1 UART8 RX request
	REQ_UART8_TX       =  82, // DMAMUX1 UART8 TX request
	REQ_SPI4_RX        =  83, // DMAMUX1 SPI4 RX request
	REQ_SPI4_TX        =  84, // DMAMUX1 SPI4 TX request
	REQ_SPI5_RX        =  85, // DMAMUX1 SPI5 RX request
	REQ_SPI5_TX        =  86, // DMAMUX1 SPI5 TX request
	REQ_SAI1_A         =  87, // DMAMUX1 SAI1 A request
	REQ_SAI1_B         =  88, // DMAMUX1 SAI1 B request
	REQ_SAI2_A         =  89, // DMAMUX1 SAI2 A request
	REQ_SAI2_B         =  90, // DMAMUX1 SAI2 B request
	REQ_SWPMI_RX       =  91, // DMAMUX1 SWPMI RX request
	REQ_SWPMI_TX       =  92, // DMAMUX1 SWPMI TX request
	REQ_SPDIF_RX_DT    =  93, // DMAMUX1 SPDIF RXDT request
	REQ_SPDIF_RX_CS    =  94, // DMAMUX1 SPDIF RXCS request
	REQ_HRTIM_MASTER   =  95, // DMAMUX1 HRTIM1 Master request 1
	REQ_HRTIM_TIMER_A  =  96, // DMAMUX1 HRTIM1 TimerA request 2
	REQ_HRTIM_TIMER_B  =  97, // DMAMUX1 HRTIM1 TimerB request 3
	REQ_HRTIM_TIMER_C  =  98, // DMAMUX1 HRTIM1 TimerC request 4
	REQ_HRTIM_TIMER_D  =  99, // DMAMUX1 HRTIM1 TimerD request 5
	REQ_HRTIM_TIMER_E  = 100, // DMAMUX1 HRTIM1 TimerE request 6
	REQ_DFSDM1_FLT0    = 101, // DMAMUX1 DFSDM Filter0 request
	REQ_DFSDM1_FLT1    = 102, // DMAMUX1 DFSDM Filter1 request
	REQ_DFSDM1_FLT2    = 103, // DMAMUX1 DFSDM Filter2 request
	REQ_DFSDM1_FLT3    = 104, // DMAMUX1 DFSDM Filter3 request
	REQ_TIM15_CH1      = 105, // DMAMUX1 TIM15 CH1 request
	REQ_TIM15_UP       = 106, // DMAMUX1 TIM15 UP request
	REQ_TIM15_TRIG     = 107, // DMAMUX1 TIM15 TRIG request
	REQ_TIM15_COM      = 108, // DMAMUX1 TIM15 COM request
	REQ_TIM16_CH1      = 109, // DMAMUX1 TIM16 CH1 request
	REQ_TIM16_UP       = 110, // DMAMUX1 TIM16 UP request
	REQ_TIM17_CH1      = 111, // DMAMUX1 TIM17 CH1 request
	REQ_TIM17_UP       = 112, // DMAMUX1 TIM17 UP request
	REQ_SAI3_A         = 113, // DMAMUX1 SAI3 A request
	REQ_SAI3_B         = 114, // DMAMUX1 SAI3 B request
	REQ_ADC3           = 115  // DMAMUX1 ADC3 request
};

enum class DMAMUX2_REQUEST : int
{
	BDMA_REQ_MEM2MEM    =  0, // memory to memory transfer
	BDMA_REQ_GENERATOR0 =  1, // DMAMUX2 request generator 0
	BDMA_REQ_GENERATOR1 =  2, // DMAMUX2 request generator 1
	BDMA_REQ_GENERATOR2 =  3, // DMAMUX2 request generator 2
	BDMA_REQ_GENERATOR3 =  4, // DMAMUX2 request generator 3
	BDMA_REQ_GENERATOR4 =  5, // DMAMUX2 request generator 4
	BDMA_REQ_GENERATOR5 =  6, // DMAMUX2 request generator 5
	BDMA_REQ_GENERATOR6 =  7, // DMAMUX2 request generator 6
	BDMA_REQ_GENERATOR7 =  8, // DMAMUX2 request generator 7
	BDMA_REQ_LPUART1_RX =  9, // DMAMUX2 LP_UART1_RX request
	BDMA_REQ_LPUART1_TX = 10, // DMAMUX2 LP_UART1_TX request
	BDMA_REQ_SPI6_RX    = 11, // DMAMUX2 SPI6 RX request
	BDMA_REQ_SPI6_TX    = 12, // DMAMUX2 SPI6 TX request
	BDMA_REQ_I2C4_RX    = 13, // DMAMUX2 I2C4 RX request
	BDMA_REQ_I2C4_TX    = 14, // DMAMUX2 I2C4 TX request
	BDMA_REQ_SAI4_A     = 15, // DMAMUX2 SAI4 A request
	BDMA_REQ_SAI4_B     = 16, // DMAMUX2 SAI4 B request
	BDMA_REQ_ADC3       = 17  // DMAMUX2 ADC3 request
};

} // namespace
