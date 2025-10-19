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
// Created: 14.04.2022 15:30

#pragma once

#ifdef STM32H7

#include <Utils/Utils.h>


enum class RCC_ClockID : int
{
    HSE,    // High speed external (4MHz-48MHz).
    LSE,    // Low speed external (32768Hz).
    HSI,    // High speed internal RC (64MHz).
    CSI     // Low power internal RC (4MHz).
};

enum class RCC_PLLID
{
    PLL1,
    PLL2,
    PLL3
};

enum class RCC_PLLSource : int
{
    HSI = 0,
    CSI = 1,
    HSE = 2,
    None = 3
};

enum class RCC_PLLDivider
{
    DIVM,   // Input divider.
    DIVN,   // Feedback divider (multiplier).
    DIVP,   // Output divider P.
    DIVQ,   // Output divider Q.
    DIVR    // Output divider R.
};

enum class RCC_PLL_OscRange : int
{
    Wide,   // 192MHz - 836MHz
    Medium  // 150MHz - 420MHz
};

enum class RCC_PLL_InputRange : int
{
    R_1_2 = 0,  // 1MHz - 2MHz
    R_2_4 = 1,  // 2MHz - 4MHz
    R_4_8 = 2,  // 4MHz - 8MHz
    R_8_16 = 3  // 8MHz - 16MHz
};

enum class RCC_PrescaledID : int
{
    HPRE,
    SYSTICK,
    D1PPRE,
    D1CPRE,
    D2PPRE1,
    D2PPRE2,
    D3PPRE
};

enum class RCC_PrescaleHPRE : uint32_t
{
    DIV1   = RCC_D1CFGR_HPRE_DIV1,   // AHB3 Clock undivided.
    DIV2   = RCC_D1CFGR_HPRE_DIV2,   // AHB3 Clock divided by 2.
    DIV4   = RCC_D1CFGR_HPRE_DIV4,   // AHB3 Clock divided by 4.
    DIV8   = RCC_D1CFGR_HPRE_DIV8,   // AHB3 Clock divided by 8.
    DIV16  = RCC_D1CFGR_HPRE_DIV16,  // AHB3 Clock divided by 16.
    DIV64  = RCC_D1CFGR_HPRE_DIV64,  // AHB3 Clock divided by 64.
    DIV128 = RCC_D1CFGR_HPRE_DIV128, // AHB3 Clock divided by 128.
    DIV256 = RCC_D1CFGR_HPRE_DIV256, // AHB3 Clock divided by 256.
    DIV512 = RCC_D1CFGR_HPRE_DIV512  // AHB3 Clock divided by 512.
};

enum class RCC_PrescaleD1PPRE : uint32_t
{
    DIV1  = RCC_D1CFGR_D1PPRE_DIV1, // APB3 clock undivided.
    DIV2  = RCC_D1CFGR_D1PPRE_DIV2, // APB3 clock divided by 2.
    DIV4  = RCC_D1CFGR_D1PPRE_DIV4, // APB3 clock divided by 4.
    DIV8  = RCC_D1CFGR_D1PPRE_DIV8, // APB3 clock divided by 8.
    DIV16 = RCC_D1CFGR_D1PPRE_DIV16 // APB3 clock divided by 16.
};

enum class RCC_PrescaleD1CPRE : uint32_t
{
    DIV1   = RCC_D1CFGR_D1CPRE_DIV1,   // Domain 1 Core clock undivided.
    DIV2   = RCC_D1CFGR_D1CPRE_DIV2,   // Domain 1 Core clock divided by 2.
    DIV4   = RCC_D1CFGR_D1CPRE_DIV4,   // Domain 1 Core clock divided by 4.
    DIV8   = RCC_D1CFGR_D1CPRE_DIV8,   // Domain 1 Core clock divided by 8.
    DIV16  = RCC_D1CFGR_D1CPRE_DIV16,  // Domain 1 Core clock divided by 16.
    DIV64  = RCC_D1CFGR_D1CPRE_DIV64,  // Domain 1 Core clock divided by 64.
    DIV128 = RCC_D1CFGR_D1CPRE_DIV128, // Domain 1 Core clock divided by 128.
    DIV256 = RCC_D1CFGR_D1CPRE_DIV256, // Domain 1 Core clock divided by 256.
    DIV512 = RCC_D1CFGR_D1CPRE_DIV512  // Domain 1 Core clock divided by 512.
};

enum class RCC_PrescaleD2PPRE1 : uint32_t
{
    DIV1  = RCC_D2CFGR_D2PPRE1_DIV1,  // APB1 clock undivided.
    DIV2  = RCC_D2CFGR_D2PPRE1_DIV2,  // APB1 clock divided by 2.
    DIV4  = RCC_D2CFGR_D2PPRE1_DIV4,  // APB1 clock divided by 4.
    DIV8  = RCC_D2CFGR_D2PPRE1_DIV8,  // APB1 clock divided by 8.
    DIV16 = RCC_D2CFGR_D2PPRE1_DIV16  // APB1 clock divided by 16.
};

enum class RCC_PrescaleD2PPRE2 : uint32_t
{
    DIV1  = RCC_D2CFGR_D2PPRE2_DIV1,  // APB2 clock undivided.
    DIV2  = RCC_D2CFGR_D2PPRE2_DIV2,  // APB2 clock divided by 2.
    DIV4  = RCC_D2CFGR_D2PPRE2_DIV4,  // APB2 clock divided by 4.
    DIV8  = RCC_D2CFGR_D2PPRE2_DIV8,  // APB2 clock divided by 8.
    DIV16 = RCC_D2CFGR_D2PPRE2_DIV16  // APB2 clock divided by 16.
};

enum class RCC_PrescaleD3PPRE : uint32_t
{
    DIV1  = RCC_D3CFGR_D3PPRE_DIV1, // APB4 clock undivided.
    DIV2  = RCC_D3CFGR_D3PPRE_DIV2, // APB4 clock divided by 2.
    DIV4  = RCC_D3CFGR_D3PPRE_DIV4, // APB4 clock divided by 4.
    DIV8  = RCC_D3CFGR_D3PPRE_DIV8, // APB4 clock divided by 8.
    DIV16 = RCC_D3CFGR_D3PPRE_DIV16 // APB4 clock divided by 16.
};

enum class RCC_SysClockSource : uint32_t
{
    HSI  = RCC_CFGR_SW_HSI,
    CSI  = RCC_CFGR_SW_CSI,
    HSE  = RCC_CFGR_SW_HSE,
    PLL1 = RCC_CFGR_SW_PLL1
};


enum class RCC_ClockMux_FMCSEL : uint32_t
{
    RCC_HCLK3 = 0,
    PLL1_Q = 1,
    PLL2_R = 2,
    PERIPHERAL_CLK = 3  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_QSPISEL : uint32_t
{
    RCC_HCLK3 = 0,
    PLL1_Q = 1,
    PLL2_R = 2,
    PERIPHERAL_CLK = 3  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_SDMMCSEL : uint32_t
{
    PLL1_Q,
    PLL2_R
};

enum class RCC_ClockMux_CKPERSEL : uint32_t
{
    HSI_KER,
    CSI_KER,
    HSE,
};

enum class RCC_ClockMux_SAI1SEL : uint32_t
{
    PLL1_Q,
    PLL2_P,
    PLL3_P,
    I2S_CKIN,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_SAI23SEL : uint32_t
{
    PLL1_Q,
    PLL2_P,
    PLL3_P,
    I2S_CKIN,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_SPI123SEL : uint32_t
{
    PLL1_Q,
    PLL2_P,
    PLL3_P,
    I2S_CKIN,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_SPI45SEL : uint32_t
{
    APB,
    PLL2_Q,
    PLL3_Q,
    HSI_KER,
    CSI_KER,
    HSE_CK
};

enum class RCC_ClockMux_SPDIFSEL : uint32_t
{
    PLL1_Q,
    PLL2_R,
    PLL3_R,
    HSI_KER
};

enum class RCC_ClockMux_DFSDM1SEL : uint32_t
{
    RCC_PCLK2,
    SYS_CK
};

enum class RCC_ClockMux_FDCANSEL : uint32_t
{
    HSE,
    PLL1_Q,
    PLL2_Q
};

enum class RCC_ClockMux_SWPSEL : uint32_t
{
    PCLK,
    HSI_KER
};

enum class RCC_ClockMux_USART16SEL : uint32_t
{
    RCC_PCLK2,
    PLL2_Q,
    PLL3_Q,
    HSI_KER,
    CSI_KER,
    LSE
};

enum class RCC_ClockMux_USART28SEL : uint32_t
{
    RCC_PCLK1,
    PLL2_Q,
    PLL3_Q,
    HSI_KER,
    CSI_KER,
    LSE
};

enum class RCC_ClockMux_RNGSEL : uint32_t
{
    HSI8,
    PLL1_Q,
    LSE,
    LSI
};

enum class RCC_ClockMux_I2C123SEL : uint32_t
{
    RCC_PCLK1,
    PLL3_R,
    HSI_KER,
    CSI_KER
};

enum class RCC_ClockMux_USBSEL : uint32_t
{
    Disabled,
    PLL1_Q,
    PLL3_Q,
    HSI48
};

enum class RCC_ClockMux_CECSEL : uint32_t
{
    LSE,
    LSI,
    CSI_KER
};

enum class RCC_ClockMux_LPTIM1SEL : uint32_t
{
    RCC_PCLK1,
    PLL2_P,
    PLL3_R,
    LSE,
    LSI,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_LPUART1SEL : uint32_t
{
    RCC_PCLK_D3,
    PLL2_Q,
    PLL3_Q,
    HSI_KER,
    CSI_KER,
    LSE
};

enum class RCC_ClockMux_I2C4SEL : uint32_t
{
    RCC_PCLK4,
    PLL3_R,
    HSI_KER,
    CSI_KER
};

enum class RCC_ClockMux_LPTIM2SEL : uint32_t
{
    RCC_PCLK4,
    PLL2_P,
    PLL3_R,
    LSE,
    LSI,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_LPTIM345SEL : uint32_t
{
    RCC_PCLK4,
    PLL2_P,
    PLL3_R,
    LSE,
    LSI,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_SAI4ASEL : uint32_t
{
    PLL1_Q,
    PLL2_P,
    PLL3_P,
    I2S_CKIN,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_SAI4BSEL : uint32_t
{
    PLL1_Q,
    PLL2_P,
    PLL3_P,
    I2S_CKIN,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_ADCSEL : uint32_t
{
    PLL2_P,
    PLL3_R,
    PERIPHERAL_CLK  // From RCC_ClockMux_CKPERSEL
};

enum class RCC_ClockMux_SPI6SEL : uint32_t
{
    RCC_PCLK4,
    PLL2_Q,
    PLL3_Q,
    HSI_KER,
    CSI_KER,
    HSE
};

enum class RCC_ClockMux_RTCSEL : uint32_t
{
    Disabled,
    LSE,
    LSI,
    HSE
};

struct ClockMuxInfo
{
    ClockMuxInfo(__IO uint32_t* InRegister, uint32_t InValueMask, uint32_t InValuePosition);

    __IO uint32_t*  Register;
    uint32_t        ValueMask;
    uint32_t        ValuePosition;
};

template<typename T> ClockMuxInfo GetClockMuxInfo();


class ResetAndClockControl
{
public:
    static void SetHSEFrequency(uint32_t freq);

    static void EnableClock(RCC_ClockID clock, bool enable, bool waitForStartup = true);
    static bool IsClockEnabled(RCC_ClockID clock);
    static void WaitForClockStartup(RCC_ClockID clock);

    static uint32_t GetHSIFrequency();

    static void             SetPLLSource(RCC_PLLSource source);
    static RCC_PLLSource    GetPLLSource();
    static void             SetPLLDivider(RCC_PLLID pll, RCC_PLLDivider divider, uint32_t value);
    static uint32_t         GetPLLDivider(RCC_PLLID pll, RCC_PLLDivider divider);

    static uint32_t         GetPLLOutFrequency(RCC_PLLID pll, RCC_PLLDivider divider);

    static void SetPLLOscillatorRange(RCC_PLLID pll, RCC_PLL_OscRange range);
    static void SetPLLInputRange(RCC_PLLID pll, RCC_PLL_InputRange range);

    static void EnablePLLOutput(RCC_PLLID pll, RCC_PLLDivider divider, bool enable);

    static void EnablePLL(RCC_PLLID pll, bool enable, bool wait = true);
    static bool IsPLLEnabled(RCC_PLLID pll);
    static void WaitForPLLStartup(RCC_PLLID pll);

    static void SetPrescaleHPRE(RCC_PrescaleHPRE scale);
    static void SetPrescaleD1PPRE(RCC_PrescaleD1PPRE scale);
    static void SetPrescaleD1CPRE(RCC_PrescaleD1CPRE scale);
    static void SetPrescaleD2PPRE1(RCC_PrescaleD2PPRE1 scale);
    static void SetPrescaleD2PPRE2(RCC_PrescaleD2PPRE2 scale);
    static void SetPrescaleD3PPRE(RCC_PrescaleD3PPRE scale);

    static void SelectSysClock(RCC_SysClockSource clock);

    template<typename T> static void SetClockMux(T source)
    {
        ClockMuxInfo muxInfo = GetClockMuxInfo<T>();
//        set_bit_group(*muxInfo.Register, muxInfo.ValueMask, uint32_t(source) << muxInfo.ValuePosition);
        *muxInfo.Register = (*muxInfo.Register & ~muxInfo.ValueMask) | ((uint32_t(source) << muxInfo.ValuePosition) & muxInfo.ValueMask);
    }

    template<typename T> static T GetClockMux()
    {
        ClockMuxInfo muxInfo = GetClockMuxInfo<T>();
        return T(((*muxInfo.Register) & muxInfo.ValueMask) >> muxInfo.ValuePosition);
    }

private:
    static uint32_t s_HSEFrequency;
};


#endif // STM32H7
