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

#ifdef STM32H7

#include <stm32h7xx.h>

#include "Kernel/HAL/STM32/ResetAndClockControl.h"
#include "Utils/Utils.h"

uint32_t ResetAndClockControl::s_HSEFrequency = 0;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ClockMuxInfo::ClockMuxInfo(__IO uint32_t* InRegister, uint32_t InValueMask, uint32_t InValuePosition) : Register(InRegister)
, ValueMask(InValueMask)
, ValuePosition(InValuePosition)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetHSEFrequency(uint32_t freq)
{
    s_HSEFrequency = freq;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::EnableClock(RCC_ClockID clock, bool enable, bool waitForStartup)
{
    if (enable)
    {
        switch (clock)
        {
            case RCC_ClockID::HSE:  RCC->CR |= RCC_CR_HSEON_Msk;        break;
            case RCC_ClockID::LSE:
                PWR->CR1 |= PWR_CR1_DBP; // Disable Back-up domain protection.
                RCC->BDCR |= RCC_BDCR_LSEON_Msk;
                PWR->CR1 &= ~PWR_CR1_DBP; // Enable Back-up domain protection.
                break;
            case RCC_ClockID::HSI:  RCC->CR |= RCC_CR_HSION_Msk;        break;
            case RCC_ClockID::CSI:  RCC->CR |= RCC_CR_CSION_Msk;        break;
        }
        if (waitForStartup)
        {
            WaitForClockStartup(clock);
        }
    }
    else
    {
        switch (clock)
        {
            case RCC_ClockID::HSE:  RCC->CR &= ~RCC_CR_HSEON_Msk;       break;
            case RCC_ClockID::LSE:
                PWR->CR1 |= PWR_CR1_DBP; // Disable Back-up domain protection.
                RCC->BDCR &= ~RCC_BDCR_LSEON_Msk;
                PWR->CR1 &= ~PWR_CR1_DBP; // Enable Back-up domain protection.
                break;
            case RCC_ClockID::HSI:  RCC->CR &= ~RCC_CR_HSION_Msk;       break;
            case RCC_ClockID::CSI:  RCC->CR &= ~RCC_CR_CSION_Msk;       break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ResetAndClockControl::IsClockEnabled(RCC_ClockID clock)
{
    switch (clock)
    {
        case RCC_ClockID::HSE:  return (RCC->CR & RCC_CR_HSEON_Msk) != 0;
        case RCC_ClockID::LSE:  return (RCC->BDCR & RCC_BDCR_LSEON_Msk) != 0;
        case RCC_ClockID::HSI:  return (RCC->CR & RCC_CR_HSION_Msk) != 0;
        case RCC_ClockID::CSI:  return (RCC->CR & RCC_CR_CSION_Msk) != 0;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::WaitForClockStartup(RCC_ClockID clock)
{
    if (IsClockEnabled(clock))
    {
        switch (clock)
        {
            case RCC_ClockID::HSE:  while ((RCC->CR & RCC_CR_HSERDY_Msk) == 0) {}   break;
            case RCC_ClockID::LSE:  while ((RCC->BDCR & RCC_BDCR_LSERDY_Msk) == 0) {} break;
            case RCC_ClockID::HSI:  while ((RCC->CR & RCC_CR_HSIRDY_Msk) == 0) {}   break;
            case RCC_ClockID::CSI:  while ((RCC->CR & RCC_CR_CSIRDY_Msk) == 0) {}   break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t ResetAndClockControl::GetHSIFrequency()
{
    switch (RCC->CR & RCC_CR_HSIDIV_Msk)
    {
        case RCC_CR_HSIDIV_1:   return 64000000;
        case RCC_CR_HSIDIV_2:   return 32000000;
        case RCC_CR_HSIDIV_4:   return 16000000;
        case RCC_CR_HSIDIV_8:   return 8000000;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPLLSource(RCC_PLLSource source)
{
    set_bit_group(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC_Msk, uint32_t(source) << RCC_PLLCKSELR_PLLSRC_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

RCC_PLLSource ResetAndClockControl::GetPLLSource()
{
    return RCC_PLLSource((RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC_Msk) >> RCC_PLLCKSELR_PLLSRC_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPLLDivider(RCC_PLLID pll, RCC_PLLDivider divider, uint32_t value)
{
    switch (pll)
    {
        case RCC_PLLID::PLL1:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  set_bit_group(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM1_Msk, value << RCC_PLLCKSELR_DIVM1_Pos);   break;
                case RCC_PLLDivider::DIVN:  set_bit_group(RCC->PLL1DIVR, RCC_PLL1DIVR_N1_Msk, (value - 1) << RCC_PLL1DIVR_N1_Pos);   break;
                case RCC_PLLDivider::DIVP:  set_bit_group(RCC->PLL1DIVR, RCC_PLL1DIVR_P1_Msk, (value - 1) << RCC_PLL1DIVR_P1_Pos);   break;
                case RCC_PLLDivider::DIVQ:  set_bit_group(RCC->PLL1DIVR, RCC_PLL1DIVR_Q1_Msk, (value - 1) << RCC_PLL1DIVR_Q1_Pos);   break;
                case RCC_PLLDivider::DIVR:  set_bit_group(RCC->PLL1DIVR, RCC_PLL1DIVR_R1_Msk, (value - 1) << RCC_PLL1DIVR_R1_Pos);   break;
            }
            break;
        case RCC_PLLID::PLL2:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  set_bit_group(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM2_Msk, value << RCC_PLLCKSELR_DIVM2_Pos);   break;
                case RCC_PLLDivider::DIVN:  set_bit_group(RCC->PLL2DIVR, RCC_PLL2DIVR_N2_Msk, (value - 1) << RCC_PLL2DIVR_N2_Pos);   break;
                case RCC_PLLDivider::DIVP:  set_bit_group(RCC->PLL2DIVR, RCC_PLL2DIVR_P2_Msk, (value - 1) << RCC_PLL2DIVR_P2_Pos);   break;
                case RCC_PLLDivider::DIVQ:  set_bit_group(RCC->PLL2DIVR, RCC_PLL2DIVR_Q2_Msk, (value - 1) << RCC_PLL2DIVR_Q2_Pos);   break;
                case RCC_PLLDivider::DIVR:  set_bit_group(RCC->PLL2DIVR, RCC_PLL2DIVR_R2_Msk, (value - 1) << RCC_PLL2DIVR_R2_Pos);   break;
            }
            break;
        case RCC_PLLID::PLL3:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  set_bit_group(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM3_Msk, value << RCC_PLLCKSELR_DIVM3_Pos);   break;
                case RCC_PLLDivider::DIVN:  set_bit_group(RCC->PLL3DIVR, RCC_PLL3DIVR_N3_Msk, (value - 1) << RCC_PLL3DIVR_N3_Pos);   break;
                case RCC_PLLDivider::DIVP:  set_bit_group(RCC->PLL3DIVR, RCC_PLL3DIVR_P3_Msk, (value - 1) << RCC_PLL3DIVR_P3_Pos);   break;
                case RCC_PLLDivider::DIVQ:  set_bit_group(RCC->PLL3DIVR, RCC_PLL3DIVR_Q3_Msk, (value - 1) << RCC_PLL3DIVR_Q3_Pos);   break;
                case RCC_PLLDivider::DIVR:  set_bit_group(RCC->PLL3DIVR, RCC_PLL3DIVR_R3_Msk, (value - 1) << RCC_PLL3DIVR_R3_Pos);   break;
            }
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t ResetAndClockControl::GetPLLDivider(RCC_PLLID pll, RCC_PLLDivider divider)
{
    switch (pll)
    {
        case RCC_PLLID::PLL1:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  return ((RCC->PLLCKSELR & RCC_PLLCKSELR_DIVM1_Msk) >> RCC_PLLCKSELR_DIVM1_Pos);
                case RCC_PLLDivider::DIVN:  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_N1_Msk) >> RCC_PLL1DIVR_N1_Pos) + 1;
                case RCC_PLLDivider::DIVP:  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_P1_Msk) >> RCC_PLL1DIVR_P1_Pos) + 1;
                case RCC_PLLDivider::DIVQ:  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_Q1_Msk) >> RCC_PLL1DIVR_Q1_Pos) + 1;
                case RCC_PLLDivider::DIVR:  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_R1_Msk) >> RCC_PLL1DIVR_R1_Pos) + 1;
            }
            break;
        case RCC_PLLID::PLL2:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  return ((RCC->PLLCKSELR & RCC_PLLCKSELR_DIVM2_Msk) >> RCC_PLLCKSELR_DIVM2_Pos);
                case RCC_PLLDivider::DIVN:  return ((RCC->PLL2DIVR & RCC_PLL2DIVR_N2_Msk) >> RCC_PLL2DIVR_N2_Pos) + 1;
                case RCC_PLLDivider::DIVP:  return ((RCC->PLL2DIVR & RCC_PLL2DIVR_P2_Msk) >> RCC_PLL2DIVR_P2_Pos) + 1;
                case RCC_PLLDivider::DIVQ:  return ((RCC->PLL2DIVR & RCC_PLL2DIVR_Q2_Msk) >> RCC_PLL2DIVR_Q2_Pos) + 1;
                case RCC_PLLDivider::DIVR:  return ((RCC->PLL2DIVR & RCC_PLL2DIVR_R2_Msk) >> RCC_PLL2DIVR_R2_Pos) + 1;
            }
            break;
        case RCC_PLLID::PLL3:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  return ((RCC->PLLCKSELR & RCC_PLLCKSELR_DIVM3_Msk) >> RCC_PLLCKSELR_DIVM3_Pos);
                case RCC_PLLDivider::DIVN:  return ((RCC->PLL3DIVR & RCC_PLL3DIVR_N3_Msk) >> RCC_PLL3DIVR_N3_Pos) + 1;
                case RCC_PLLDivider::DIVP:  return ((RCC->PLL3DIVR & RCC_PLL3DIVR_P3_Msk) >> RCC_PLL3DIVR_P3_Pos) + 1;
                case RCC_PLLDivider::DIVQ:  return ((RCC->PLL3DIVR & RCC_PLL3DIVR_Q3_Msk) >> RCC_PLL3DIVR_Q3_Pos) + 1;
                case RCC_PLLDivider::DIVR:  return ((RCC->PLL3DIVR & RCC_PLL3DIVR_R3_Msk) >> RCC_PLL3DIVR_R3_Pos) + 1;
            }
            break;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t ResetAndClockControl::GetPLLOutFrequency(RCC_PLLID pll, RCC_PLLDivider divider)
{
    uint32_t frequency = 0;

    switch (GetPLLSource())
    {
        case RCC_PLLSource::HSI:    frequency = GetHSIFrequency();  break;
        case RCC_PLLSource::CSI:    frequency = 4000000;            break;
        case RCC_PLLSource::HSE:    frequency = s_HSEFrequency;     break;
        case RCC_PLLSource::None:   frequency = 0;                  break;
    }
    return uint32_t(double(frequency) / double(GetPLLDivider(pll, RCC_PLLDivider::DIVM)) * double(GetPLLDivider(pll, RCC_PLLDivider::DIVN)) / double(GetPLLDivider(pll, divider)));   
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPLLOscillatorRange(RCC_PLLID pll, RCC_PLL_OscRange range)
{
    switch (pll)
    {
        case RCC_PLLID::PLL1:
            switch (range)
            {
                case RCC_PLL_OscRange::Wide:    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL1VCOSEL;    break;
                case RCC_PLL_OscRange::Medium:  RCC->PLLCFGR |= RCC_PLLCFGR_PLL1VCOSEL;     break;
            }
            break;
        case RCC_PLLID::PLL2:
            switch (range)
            {
                case RCC_PLL_OscRange::Wide:    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL2VCOSEL;    break;
                case RCC_PLL_OscRange::Medium:  RCC->PLLCFGR |= RCC_PLLCFGR_PLL2VCOSEL;     break;
            }
            break;
        case RCC_PLLID::PLL3:
            switch (range)
            {
                case RCC_PLL_OscRange::Wide:    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLL3VCOSEL;    break;
                case RCC_PLL_OscRange::Medium:  RCC->PLLCFGR |= RCC_PLLCFGR_PLL3VCOSEL;     break;
            }
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPLLInputRange(RCC_PLLID pll, RCC_PLL_InputRange range)
{
    switch (pll)
    {
        case RCC_PLLID::PLL1:   set_bit_group(RCC->PLLCFGR, RCC_PLLCFGR_PLL1RGE_Msk, uint32_t(range) << RCC_PLLCFGR_PLL1RGE_Pos);   break;
        case RCC_PLLID::PLL2:   set_bit_group(RCC->PLLCFGR, RCC_PLLCFGR_PLL2RGE_Msk, uint32_t(range) << RCC_PLLCFGR_PLL2RGE_Pos);   break;
        case RCC_PLLID::PLL3:   set_bit_group(RCC->PLLCFGR, RCC_PLLCFGR_PLL3RGE_Msk, uint32_t(range) << RCC_PLLCFGR_PLL3RGE_Pos);   break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::EnablePLLOutput(RCC_PLLID pll, RCC_PLLDivider divider, bool enable)
{
    uint32_t flag = 0;
    switch (pll)
    {
        case RCC_PLLID::PLL1:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  break;
                case RCC_PLLDivider::DIVN:  break;
                case RCC_PLLDivider::DIVP:  flag = RCC_PLLCFGR_DIVP1EN;    break;
                case RCC_PLLDivider::DIVQ:  flag = RCC_PLLCFGR_DIVQ1EN;    break;
                case RCC_PLLDivider::DIVR:  flag = RCC_PLLCFGR_DIVR1EN;    break;
            }
            break;
        case RCC_PLLID::PLL2:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  break;
                case RCC_PLLDivider::DIVN:  break;
                case RCC_PLLDivider::DIVP:  flag = RCC_PLLCFGR_DIVP2EN;    break;
                case RCC_PLLDivider::DIVQ:  flag = RCC_PLLCFGR_DIVQ2EN;    break;
                case RCC_PLLDivider::DIVR:  flag = RCC_PLLCFGR_DIVR2EN;    break;
            }
            break;
        case RCC_PLLID::PLL3:
            switch (divider)
            {
                case RCC_PLLDivider::DIVM:  break;
                case RCC_PLLDivider::DIVN:  break;
                case RCC_PLLDivider::DIVP:  flag = RCC_PLLCFGR_DIVP3EN;    break;
                case RCC_PLLDivider::DIVQ:  flag = RCC_PLLCFGR_DIVQ3EN;    break;
                case RCC_PLLDivider::DIVR:  flag = RCC_PLLCFGR_DIVR3EN;    break;
            }
            break;
        default:
            break;
    }
    if (enable)
    {
        RCC->PLLCFGR |= flag;
    }
    else
    {
        RCC->PLLCFGR &= ~flag;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::EnablePLL(RCC_PLLID pll, bool enable, bool wait)
{
    uint32_t flag = 0;
    switch (pll)
    {
        case RCC_PLLID::PLL1:   flag = RCC_CR_PLL1ON;   break;
        case RCC_PLLID::PLL2:   flag = RCC_CR_PLL2ON;   break;
        case RCC_PLLID::PLL3:   flag = RCC_CR_PLL3ON;   break;
    }
    if (enable)
    {
        RCC->CR |= flag;
        if (wait)
        {
            WaitForPLLStartup(pll);
        }
    }
    else
    {
        RCC->CR &= ~flag;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ResetAndClockControl::IsPLLEnabled(RCC_PLLID pll)
{
    uint32_t flag = 0;
    switch (pll)
    {
        case RCC_PLLID::PLL1:   flag = RCC_CR_PLL1ON;   break;
        case RCC_PLLID::PLL2:   flag = RCC_CR_PLL2ON;   break;
        case RCC_PLLID::PLL3:   flag = RCC_CR_PLL3ON;   break;
    }
    return (RCC->CR & flag) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::WaitForPLLStartup(RCC_PLLID pll)
{
    uint32_t flag = 0;
    switch (pll)
    {
        case RCC_PLLID::PLL1:   flag = RCC_CR_PLL1RDY;   break;
        case RCC_PLLID::PLL2:   flag = RCC_CR_PLL2RDY;   break;
        case RCC_PLLID::PLL3:   flag = RCC_CR_PLL3RDY;   break;
    }
    if (flag != 0)
    {
        while ((RCC->CR & flag) == 0) {}
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPrescaleHPRE(RCC_PrescaleHPRE scale)
{
    set_bit_group(RCC->D1CFGR, RCC_D1CFGR_HPRE_Msk, uint32_t(scale));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPrescaleD1PPRE(RCC_PrescaleD1PPRE scale)
{
    set_bit_group(RCC->D1CFGR, RCC_D1CFGR_D1PPRE_Msk, uint32_t(scale));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPrescaleD1CPRE(RCC_PrescaleD1CPRE scale)
{
    set_bit_group(RCC->D1CFGR, RCC_D1CFGR_D1CPRE_Msk, uint32_t(scale));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPrescaleD2PPRE1(RCC_PrescaleD2PPRE1 scale)
{
    set_bit_group(RCC->D2CFGR, RCC_D2CFGR_D2PPRE1_Msk, uint32_t(scale));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPrescaleD2PPRE2(RCC_PrescaleD2PPRE2 scale)
{
    set_bit_group(RCC->D2CFGR, RCC_D2CFGR_D2PPRE2_Msk, uint32_t(scale));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SetPrescaleD3PPRE(RCC_PrescaleD3PPRE scale)
{
    set_bit_group(RCC->D3CFGR, RCC_D3CFGR_D3PPRE_Msk, uint32_t(scale));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ResetAndClockControl::SelectSysClock(RCC_SysClockSource source)
{
    set_bit_group(RCC->CFGR, RCC_CFGR_SW_Msk, uint32_t(source));
}



template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_FMCSEL>()      { return ClockMuxInfo(&RCC->D1CCIPR,    RCC_D1CCIPR_FMCSEL_Msk,         RCC_D1CCIPR_FMCSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_QSPISEL>()     { return ClockMuxInfo(&RCC->D1CCIPR,    RCC_D1CCIPR_QSPISEL_Msk,        RCC_D1CCIPR_QSPISEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SDMMCSEL>()    { return ClockMuxInfo(&RCC->D1CCIPR,    RCC_D1CCIPR_SDMMCSEL_Msk,       RCC_D1CCIPR_SDMMCSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_CKPERSEL>()    { return ClockMuxInfo(&RCC->D1CCIPR,    RCC_D1CCIPR_CKPERSEL_Msk,       RCC_D1CCIPR_CKPERSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SAI1SEL>()     { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_SAI1SEL_Msk,       RCC_D2CCIP1R_SAI1SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SAI23SEL>()    { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_SAI23SEL_Msk,      RCC_D2CCIP1R_SAI23SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SPI123SEL>()   { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_SPI123SEL_Msk,     RCC_D2CCIP1R_SPI123SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SPI45SEL>()    { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_SPI45SEL_Msk,      RCC_D2CCIP1R_SPI45SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SPDIFSEL>()    { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_SPDIFSEL_Msk,      RCC_D2CCIP1R_SPDIFSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_DFSDM1SEL>()   { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_DFSDM1SEL_Msk,     RCC_D2CCIP1R_DFSDM1SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_FDCANSEL>()    { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_FDCANSEL_Msk,      RCC_D2CCIP1R_FDCANSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SWPSEL>()      { return ClockMuxInfo(&RCC->D2CCIP1R,   RCC_D2CCIP1R_SWPSEL_Msk,        RCC_D2CCIP1R_SWPSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_USART16SEL>()  { return ClockMuxInfo(&RCC->D2CCIP2R,   RCC_D2CCIP2R_USART16SEL_Msk,    RCC_D2CCIP2R_USART16SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_USART28SEL>()  { return ClockMuxInfo(&RCC->D2CCIP2R,   RCC_D2CCIP2R_USART28SEL_Msk,    RCC_D2CCIP2R_USART28SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_RNGSEL>()      { return ClockMuxInfo(&RCC->D2CCIP2R,   RCC_D2CCIP2R_RNGSEL_Msk,        RCC_D2CCIP2R_RNGSEL_Pos);}
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_I2C123SEL>()   { return ClockMuxInfo(&RCC->D2CCIP2R,   RCC_D2CCIP2R_I2C123SEL_Msk,     RCC_D2CCIP2R_I2C123SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_USBSEL>()      { return ClockMuxInfo(&RCC->D2CCIP2R,   RCC_D2CCIP2R_USBSEL_Msk,        RCC_D2CCIP2R_USBSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_CECSEL>()      { return ClockMuxInfo(&RCC->D2CCIP2R,   RCC_D2CCIP2R_CECSEL_Msk,        RCC_D2CCIP2R_CECSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_LPTIM1SEL>()   { return ClockMuxInfo(&RCC->D2CCIP2R,   RCC_D2CCIP2R_LPTIM1SEL_Msk,     RCC_D2CCIP2R_LPTIM1SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_LPUART1SEL>()  { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_LPUART1SEL_Msk,     RCC_D3CCIPR_LPUART1SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_I2C4SEL>()     { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_I2C4SEL_Msk,        RCC_D3CCIPR_I2C4SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_LPTIM2SEL>()   { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_LPTIM2SEL_Msk,      RCC_D3CCIPR_LPTIM2SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_LPTIM345SEL>() { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_LPTIM345SEL_Msk,    RCC_D3CCIPR_LPTIM345SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SAI4ASEL>()    { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_SAI4ASEL_Msk,       RCC_D3CCIPR_SAI4ASEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SAI4BSEL>()    { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_SAI4BSEL_Msk,       RCC_D3CCIPR_SAI4BSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_ADCSEL>()      { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_ADCSEL_Msk,         RCC_D3CCIPR_ADCSEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_SPI6SEL>()     { return ClockMuxInfo(&RCC->D3CCIPR,    RCC_D3CCIPR_SPI6SEL_Msk,        RCC_D3CCIPR_SPI6SEL_Pos); }
template<> IFLASHC ClockMuxInfo GetClockMuxInfo<RCC_ClockMux_RTCSEL>()      { return ClockMuxInfo(&RCC->BDCR,       RCC_BDCR_RTCSEL_Msk,            RCC_BDCR_RTCSEL_Pos); }

#endif // STM32H7
