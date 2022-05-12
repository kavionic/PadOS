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
// Created: 26.04.2022 21:00


#include <Kernel/HAL/STM32/QSPI_STM32_IS25LP512M.h>
#include <Kernel/SpinTimer.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool QSPI_STM32_IS25LP512M::Setup(uint32_t spiFrequency, uint32_t addressBits, PinMuxTarget pinD0, PinMuxTarget pinD1, PinMuxTarget pinD2, PinMuxTarget pinD3, PinMuxTarget pinCLK, PinMuxTarget pinNCS)
{
    if (!QSPI_STM32::Setup((spiFrequency > 50000000UL) ? 50000000UL : spiFrequency, addressBits, pinD0, pinD1, pinD2, pinD3, pinCLK, pinNCS)) {
        return false;
    }
    for (;;)
    {
        kernel::SpinTimer::SleepuS(500);
        SendCommand(QSPI_CMD_RSTEN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr1Line);
        SendCommand(QSPI_CMD_RST, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr1Line);

        kernel::SpinTimer::SleepuS(500);

        // Set output drive strength.
        SendCommand(QSPI_CMD_WREN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr1Line);
        SetDataLength(1);
        SendCommand(QSPI_CMD_SERPV, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr1Line, QSPI_AddressMode::NoAddr, QSPI_DataMode::Data1Line);
        Write8(QSPI_EXT_READR_ODS_100 << QSPI_EXT_READR_ODS_Pos);

        volatile uint8_t functionReg = ReadFunctionRegister(false);
        (void)functionReg;

        uint32_t id = ReadProductID(false);
        if (id != 0) {
            break;
        }
        QUADSPI->ABR = 0;

        for (int i = 0; i < 100; ++i)
        {
            SendCommand(
                QSPI_CMD_4FRQIO,
                QSPI_FunctionalMode::IndirectRead,
                QSPI_InstrMode::NoInstr,
                QSPI_AddressMode::Addr4Lines,
                QSPI_DataMode::NoData,
                QSPI_AltBytesMode::Alt4Lines,
                QSPI_AltBytesLength::AB8,
                QSPI_READ_DUMMY_CYCLES - 2
            );
        }
        SendCommand(QSPI_CMD_WREN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines);
        SendCommand(QSPI_CMD_QPIDI, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines);
    }

    // Enable quad mode.
    SendCommand(QSPI_CMD_WREN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr1Line);
    SendCommand(QSPI_CMD_QPIEN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr1Line);

    SetSPIFrequency(spiFrequency);

    // Configure read mode.
    SendCommand(QSPI_CMD_WREN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines);
    const uint8_t readMode = (QSPI_READ_BURST_LEN << QSPI_READR_BurstLength_Pos) | (QSPI_READ_DUMMY_CYCLES << QSPI_READR_DummyCycles_Pos);
    SetDataLength(1);
    SendCommand(QSPI_CMD_SRPV, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines, QSPI_AddressMode::NoAddr, QSPI_DataMode::Data4Lines);
    Write8(readMode);

    SetAddressLen(QSPI_AddressLength::AL32);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32_IS25LP512M::EnableMemoryMapping(bool useContinousRead)
{
    if (useContinousRead)
    {
        SetSendInstrOnlyOnce(true);
        QUADSPI->ABR = 0xa0a0a0a0;
    }
    else
    {
        QUADSPI->ABR = 0;
    }

    char data1[5];
    char data2[5];
    Read(data1, 0x1000, 4);
    Read(data2, 0x1000, 4);

    SendCommand(
        QSPI_CMD_4FRQIO,
        QSPI_FunctionalMode::MemoryMapped,
        QSPI_InstrMode::Instr4Lines,
        QSPI_AddressMode::Addr4Lines,
        QSPI_DataMode::Data4Lines,
        QSPI_AltBytesMode::Alt4Lines,
        QSPI_AltBytesLength::AB8,
        QSPI_READ_DUMMY_CYCLES - 2
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32_IS25LP512M::Erase(uint32_t address, uint32_t length)
{
    while (length >= QSPI_SECTOR_SIZE)
    {
        SendCommand(QSPI_CMD_WREN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines);
        SendCommand(QSPI_CMD_4SER, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines, QSPI_AddressMode::Addr4Lines);
        QUADSPI->AR = address;
        WaitWriteInProgress(QSPI_STATUS_QE | QSPI_STATUS_WEL | QSPI_STATUS_WIP, QSPI_STATUS_QE);
        length -= QSPI_SECTOR_SIZE;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32_IS25LP512M::Read(void* data, uint32_t address, uint32_t length)
{
    uint8_t* ptr = reinterpret_cast<uint8_t*>(data);

    SetDataLength(length);
    SendCommand(QSPI_CMD_4FRQIO, QSPI_FunctionalMode::IndirectRead, QSPI_InstrMode::Instr4Lines, QSPI_AddressMode::Addr4Lines, QSPI_DataMode::Data4Lines, QSPI_AltBytesMode::Alt4Lines, QSPI_AltBytesLength::AB8, QSPI_READ_DUMMY_CYCLES - 2);
    QUADSPI->AR = address;

    for (int i = 0; i < length; ++i)
    {
        WaitFIFOThreshold();
        *ptr++ = Read8();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32_IS25LP512M::Write(const void* data, uint32_t address, uint32_t length)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);

    WaitTransferComplete();

    while (length > 0)
    {
        uint32_t pageLength = std::min(QSPI_PAGE_SIZE, length);
        length -= pageLength;

        SendCommand(QSPI_CMD_WREN, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines);
        SetDataLength(pageLength);
        SendCommand(QSPI_CMD_4PP, QSPI_FunctionalMode::IndirectWrite, QSPI_InstrMode::Instr4Lines, QSPI_AddressMode::Addr4Lines, QSPI_DataMode::Data4Lines);
        QUADSPI->AR = address;
        address += pageLength;

        while (pageLength-- > 0)
        {
            WaitFIFOThreshold();
            Write8(*ptr++);
        }
        WaitTransferComplete();
        WaitWriteInProgress(QSPI_STATUS_QE | QSPI_STATUS_WEL | QSPI_STATUS_WIP, QSPI_STATUS_QE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32_IS25LP512M::WaitWriteInProgress(uint8_t mask, uint8_t match)
{
    WaitBusy();

    QUADSPI->PSMKR = mask;
    QUADSPI->PSMAR = match;
    QUADSPI->PIR = 10;

    SetDataLength(1);
    // Not sure why, but it must insert 2 dummy cycles (to ignore the first byte read) in order to make it work. Without that, it trigger a false match instantly.
    SendCommand(QSPI_CMD_RDSR, QSPI_FunctionalMode::AutomaticPolling, QSPI_InstrMode::Instr4Lines, QSPI_AddressMode::NoAddr, QSPI_DataMode::Data4Lines, QSPI_AltBytesMode::NoData, QSPI_AltBytesLength::AB8, 2);
    WaitBusy();
    QUADSPI->FCR = QUADSPI_FCR_CSMF;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t QSPI_STM32_IS25LP512M::ReadFunctionRegister(bool quadMode) const
{
    SetDataLength(1);
    if (quadMode) {
        SendCommand(QSPI_CMD_RDFR, QSPI_FunctionalMode::IndirectRead, QSPI_InstrMode::Instr4Lines, QSPI_AddressMode::NoAddr, QSPI_DataMode::Data4Lines);
    } else {
        SendCommand(QSPI_CMD_RDFR, QSPI_FunctionalMode::IndirectRead, QSPI_InstrMode::Instr1Line, QSPI_AddressMode::NoAddr, QSPI_DataMode::Data1Line);
    }
    WaitFIFOThreshold();
    return Read8();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void QSPI_STM32_IS25LP512M::ReadProductID(uint8_t& manufacturerID, uint8_t& memoryType, uint8_t& capacity, bool quadMode)
{
    SetDataLength(3);
    if (quadMode) {
        SendCommand(QSPI_CMD_RDJDIDQ, QSPI_FunctionalMode::IndirectRead, QSPI_InstrMode::Instr4Lines, QSPI_AddressMode::NoAddr, QSPI_DataMode::Data4Lines);
    } else {
        SendCommand(QSPI_CMD_RDJDID, QSPI_FunctionalMode::IndirectRead, QSPI_InstrMode::Instr1Line, QSPI_AddressMode::NoAddr, QSPI_DataMode::Data1Line);
    }
    uint8_t id[3];
    for (int i = 0; i < 3; ++i)
    {
        WaitFIFOThreshold();
        id[i] = Read8();
    }
    manufacturerID  = id[0];
    memoryType      = id[1];
    capacity        = id[2];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t QSPI_STM32_IS25LP512M::ReadProductID(bool quadMode)
{
    uint8_t manufacturerID;
    uint8_t memoryType;
    uint8_t capacity;

    ReadProductID(manufacturerID, memoryType, capacity, quadMode);

    return (manufacturerID << 16) | (memoryType << 8) | capacity;
}
