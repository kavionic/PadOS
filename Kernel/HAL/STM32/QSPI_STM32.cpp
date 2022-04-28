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
// Created: 19.04.2022 21:00

#include <algorithm>
#include <Kernel/HAL/STM32/QSPI_STM32.h>
#include <Kernel/HAL/STM32/ResetAndClockControl.h>
#include <Kernel/SpinTimer.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool QSPI_STM32::Setup(uint32_t spiFrequency, uint32_t addressBits, PinMuxTarget pinD0, PinMuxTarget pinD1, PinMuxTarget pinD2, PinMuxTarget pinD3, PinMuxTarget pinCLK, PinMuxTarget pinNCS)
{
    RCC->AHB3RSTR |= RCC_AHB3RSTR_QSPIRST;
    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_QSPIRST;

    SendIO3Reset(pinD3.PINID);
    SendJEDECReset(pinD0.PINID, pinCLK.PINID, pinNCS.PINID);

    DigitalPin::ActivatePeripheralMux(pinD0);
    DigitalPin::ActivatePeripheralMux(pinD1);
    DigitalPin::ActivatePeripheralMux(pinD2);
    DigitalPin::ActivatePeripheralMux(pinD3);
    DigitalPin::ActivatePeripheralMux(pinCLK);
    DigitalPin::ActivatePeripheralMux(pinNCS);

    DigitalPin(pinD0.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(pinD1.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(pinD2.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(pinD3.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(pinCLK.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(pinNCS.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);

    QUADSPI->CR = QUADSPI_CR_SSHIFT | QUADSPI_CR_APMS;
    QUADSPI->DCR = (addressBits - 1) << QUADSPI_DCR_FSIZE_Pos;

    SetSPIFrequency(spiFrequency);

    QUADSPI->CR |= QUADSPI_CR_EN;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool QSPI_STM32::SetSPIFrequency(uint32_t spiFrequency)
{
    uint32_t perifFrequency = 0;

    const RCC_ClockMux_QSPISEL clockMux = ResetAndClockControl::GetClockMux<RCC_ClockMux_QSPISEL>();

    switch (clockMux)
    {
        case RCC_ClockMux_QSPISEL::RCC_HCLK3:       perifFrequency = 0; break;
        case RCC_ClockMux_QSPISEL::PLL1_Q:          perifFrequency = ResetAndClockControl::GetPLLOutFrequency(RCC_PLLID::PLL1, RCC_PLLDivider::DIVQ);   break;
        case RCC_ClockMux_QSPISEL::PLL2_R:          perifFrequency = ResetAndClockControl::GetPLLOutFrequency(RCC_PLLID::PLL2, RCC_PLLDivider::DIVR);   break;
        case RCC_ClockMux_QSPISEL::PERIPHERAL_CLK:  perifFrequency = 0; break;  // From RCC_ClockMux_CKPERSEL
    }
    if (perifFrequency == 0)
    {
        return false;
    }
    const int32_t clockDivider = std::max(1UL, (perifFrequency + spiFrequency - 1) / spiFrequency);

    WaitBusy();
    set_bit_group(QUADSPI->CR, QUADSPI_CR_PRESCALER_Msk, (clockDivider - 1) << QUADSPI_CR_PRESCALER_Pos);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SetDDRMode(bool enableDDR)
{
    if (enableDDR) {
        QUADSPI->CCR |= QUADSPI_CCR_DDRM;
    } else {
        QUADSPI->CCR &= ~QUADSPI_CCR_DDRM;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SetDDRHold(bool hold)
{
    if (hold) {
        QUADSPI->CCR |= QUADSPI_CCR_DHHC;
    } else {
        QUADSPI->CCR &= ~QUADSPI_CCR_DHHC;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SetSendInstrOnlyOnce(bool onlyOnce)
{
    WaitBusy();
    if (onlyOnce) {
        QUADSPI->CCR |= QUADSPI_CCR_SIOO;
    } else {
        QUADSPI->CCR &= ~QUADSPI_CCR_SIOO;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SetAddressLen(QSPI_AddressLength addressLen)
{
    WaitBusy();
    set_bit_group(QUADSPI->CCR, QUADSPI_CCR_ADSIZE_Msk, (int32_t(addressLen) << QUADSPI_CCR_ADSIZE_Pos));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SendCommand(
    uint8_t             cmd,
    QSPI_FunctionalMode functionalMode,
    QSPI_InstrMode      instrMode,
    QSPI_AddressMode    addrMode,
    QSPI_DataMode       dataMode,
    QSPI_AltBytesMode   altBytesMode,
    QSPI_AltBytesLength altBytesLen,
    uint32_t            dummyCycles
) const
{
    const uint32_t mask
    = QUADSPI_CCR_INSTRUCTION_Msk
    | QUADSPI_CCR_FMODE_Msk
    | QUADSPI_CCR_IMODE_Msk
    | QUADSPI_CCR_ADMODE_Msk
    | QUADSPI_CCR_ABMODE_Msk
    | QUADSPI_CCR_DMODE_Msk
    | QUADSPI_CCR_ABSIZE_Msk
    | QUADSPI_CCR_DCYC_Msk;

    const uint32_t flags
    = (cmd << QUADSPI_CCR_INSTRUCTION_Pos)
    | (uint32_t(functionalMode) << QUADSPI_CCR_FMODE_Pos)
    | (uint32_t(instrMode) << QUADSPI_CCR_IMODE_Pos)
    | (uint32_t(addrMode) << QUADSPI_CCR_ADMODE_Pos)
    | (uint32_t(altBytesMode) << QUADSPI_CCR_ABMODE_Pos)
    | (uint32_t(dataMode) << QUADSPI_CCR_DMODE_Pos)
    | (uint32_t(altBytesLen) << QUADSPI_CCR_ABSIZE_Pos)
    | (dummyCycles << QUADSPI_CCR_DCYC_Pos);

    WaitBusy();

    set_bit_group(QUADSPI->CCR, mask, flags);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SetDataLength(uint32_t length) const
{
    WaitBusy();
    QUADSPI->DLR = length - 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::WaitBusy() const
{
    while (QUADSPI->SR & QUADSPI_SR_BUSY) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SendIO3Reset(DigitalPinID pinD3)
{
    kernel::SpinTimer::SleepuS(200);
    DigitalPin(pinD3).SetDirection(DigitalPinDirection_e::Out);
    DigitalPin(pinD3).Write(false);
    kernel::SpinTimer::SleepuS(200);
    DigitalPin(pinD3).SetDirection(DigitalPinDirection_e::Analog);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32::SendJEDECReset(DigitalPinID pinD0, DigitalPinID pinCLK, DigitalPinID pinNCS)
{
    DigitalPin(pinD0).Write(true);
    DigitalPin(pinNCS).Write(true);

    DigitalPin(pinNCS).SetDirection(DigitalPinDirection_e::Out);
    DigitalPin(pinCLK).SetDirection(DigitalPinDirection_e::Out);
    DigitalPin(pinD0).SetDirection(DigitalPinDirection_e::Out);

    for (int i = 0; i < 2; ++i)
    {
        kernel::SpinTimer::SleepuS(100);
        DigitalPin(pinNCS).Write(false);
        kernel::SpinTimer::SleepuS(100);
        DigitalPin(pinD0).Write(false);
        kernel::SpinTimer::SleepuS(100);
        DigitalPin(pinNCS).Write(true);
        kernel::SpinTimer::SleepuS(100);
        DigitalPin(pinD0).Write(true);
    }

    for (int i = 0; i < 4; ++i)
    {
        kernel::SpinTimer::SleepuS(100);
        DigitalPin(pinNCS).Write(false);
        kernel::SpinTimer::SleepuS(100);
        DigitalPin(pinNCS).Write(true);
    }

    DigitalPin(pinNCS).SetDirection(DigitalPinDirection_e::Analog);
    DigitalPin(pinCLK).SetDirection(DigitalPinDirection_e::Analog);
    DigitalPin(pinD0).SetDirection(DigitalPinDirection_e::Analog);
}

