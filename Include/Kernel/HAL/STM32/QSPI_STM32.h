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

#pragma once

#include "Kernel/HAL/DigitalPort.h"

enum class QSPI_InstrMode : uint32_t
{
    NoInstr     = 0,
    Instr1Line  = 1,
    Instr2Lines = 2,
    Instr4Lines = 3
};

enum class QSPI_DataMode : uint32_t
{
    NoData      = 0,
    Data1Line   = 1,
    Data2Lines  = 2,
    Data4Lines  = 3
};

enum class QSPI_AltBytesMode : uint32_t
{
    NoData      = 0,
    Alt1Line    = 1,
    Alt2Lines   = 2,
    Alt4Lines   = 3
};

enum class QSPI_AddressMode : uint32_t
{
    NoAddr = 0,
    Addr1Line = 1,
    Addr2Lines = 2,
    Addr4Lines = 3
};

enum class QSPI_FunctionalMode : uint32_t
{
    IndirectWrite       = 0,
    IndirectRead        = 1,
    AutomaticPolling    = 2,
    MemoryMapped        = 3
};

enum class QSPI_AltBytesLength : uint32_t
{
    AB8  = 0,
    AB16 = 1,
    AB24 = 2,
    AB32 = 3
};

enum class QSPI_AddressLength : uint32_t
{
    AL8 = 0,
    AL16 = 1,
    AL24 = 2,
    AL32 = 3
};

static constexpr uint32_t QSPI_PAGE_SIZE = 256;
static constexpr uint32_t QSPI_SECTOR_SIZE = 4096;
static constexpr uint32_t QSPI_BLOCK32_SIZE = 32 * 1024;
static constexpr uint32_t QSPI_BLOCK64_SIZE = 64 * 1024;

namespace kernel
{

class QSPI_STM32
{
public:
    virtual bool Setup(uint32_t spiFrequency, uint32_t addressBits, PinMuxTarget pinD0, PinMuxTarget pinD1, PinMuxTarget pinD2, PinMuxTarget pinD3, PinMuxTarget pinCLK, PinMuxTarget pinNCS);

    bool SetSPIFrequency(uint32_t spiFrequency);

    void SetDDRMode(bool enableDDR);
    void SetDDRHold(bool hold);
    void SetSendInstrOnlyOnce(bool onlyOnce);
    void SetAddressLen(QSPI_AddressLength addressLen);

    virtual void EnableMemoryMapping(bool useContinousRead) = 0;

    void SendCommand(
        uint8_t             cmd,
        QSPI_FunctionalMode functionalMode,
        QSPI_InstrMode      instrMode,
        QSPI_AddressMode    addrMode = QSPI_AddressMode::NoAddr,
        QSPI_DataMode       dataMode = QSPI_DataMode::NoData,
        QSPI_AltBytesMode   altBytesMode = QSPI_AltBytesMode::NoData,
        QSPI_AltBytesLength altBytesLen = QSPI_AltBytesLength::AB8,
        uint32_t            dummyCycles = 0
    ) const;
    void SetDataLength(uint32_t length) const;

    void Write8(uint8_t data);
    void Write16(uint16_t data);
    void Write32(uint32_t data);

    uint8_t     Read8() const;
    uint16_t    Read16() const;
    uint32_t    Read32() const;

    void WaitBusy() const;
    void WaitFIFOThreshold() const;
    void WaitTransferComplete() const;

private:
    void SendIO3Reset(DigitalPinID pinIO3);
    void SendJEDECReset(DigitalPinID pinD0, DigitalPinID pinCLK, DigitalPinID pinNCS);
};

} // namespace kernel
