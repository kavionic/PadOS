#pragma once


#define CLK_MUX_PLL_SRC
#define CLK_MUX_SYSCLK_SRC
#define CLK_MUX_RTCCLK_SRC
#define CLK_MUX_PER_SRC
#define CLK_MUX_MCO1_SRC
#define CLK_MUX_MCO2_SRC

enum class STM32H7MuxPLLSrc : uint32_t
{
	HSI = RCC_PLLCKSELR_PLLSRC_HSI,
	CSI = RCC_PLLCKSELR_PLLSRC_CSI,
	HSE = RCC_PLLCKSELR_PLLSRC_HSE,
	NONE = RCC_PLLCKSELR_PLLSRC_NONE
};



class STM32H7System
{
public:
	static void SetClkMuxPLLSrc(STM32H7MuxPLLSrc src) { set_bit_group(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC_Msk, uint32_t(src)); }
	static void SetClkMuxSysClkSrc();
	static void SetClkMuxRTCClkSrc();
	static void SetClkMuxPERClkSrc();
	static void SetClkMuxSPI123();
	static void SetClkMuxSAI1();
	static void SetClkMuxSAI23();
	static void SetClkMuxSAI4A();
	static void SetClkMuxSAI4B();
	static void SetClkMuxRNG();
	static void SetClkMuxI2C123();
	static void SetClkMuxI2C4();
	static void SetClkMuxFMC();
	static void SetClkMuxSPDIFRX1();
	static void SetClkMuxQUADSPI();
	static void SetClkMuxSWP();
	static void SetClkMuxDFSDM();
	static void SetClkMuxSDMMC12();
	static void SetClkMuxUSART16();
	static void SetClkMuxUSART234578();
	static void SetClkMuxLPUART1();
	static void SetClkMuxLPTIM1();
	static void SetClkMuxLPTIM2();
	static void SetClkMuxLPTIM345();
	static void SetClkMuxSPI45();
	static void SetClkMuxSPI6();
	static void SetClkMuxADC();
	static void SetClkMuxUSB();
	static void SetClkMuxCEC();
	static void SetClkMuxFDCAN();
	static void SetClkMuxHRTIM();
	static void SetClkMuxTRACE();
	static void SetClkMuxMCO1Src();
	static void SetClkMuxMCO2Src();
};

