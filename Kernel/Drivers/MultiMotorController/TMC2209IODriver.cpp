// This file is part of PadOS.
//
// Copyright (C) 2021-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 22.03.2021 22:30

#include <string.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>

#include <System/ExceptionHandling.h>
#include <Utils/HashCalculator.h>
#include <Kernel/KTime.h>
#include <Kernel/VFS/FileIO.h>

#include <Kernel/Drivers/MultiMotorController/TMC2209IODriver.h>

using namespace os;

namespace kernel
{

static constexpr uint8_t TMC2209_SYNC = 0x05; // Sync pattern used by the chip to detect the baud rate.
static constexpr uint8_t TMC2209_REG_ADDR_WRITE = 0x80; // Set in register address to indicate write, clear for read.


struct TMC2209WriteDatagram
{
    uint8_t Sync;
    uint8_t SlaveAddress;
    uint8_t RegisterAddress;
    uint8_t Data[4];
    uint8_t Checksum;
};

struct TMC2209ReadRequestDatagram
{
    uint8_t Sync;
    uint8_t SlaveAddress;
    uint8_t RegisterAddress;
    uint8_t Checksum;
};

struct TMC2209ReadReplyDatagram
{
    uint8_t Sync;
    uint8_t MasterAddress;
    uint8_t RegisterAddress;
    uint8_t Data[4];
    uint8_t Checksum;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename DATAGRAM>
void TMC2209UpdateCRC(DATAGRAM* datagram)
{
    os::HashCalculator<os::HashAlgorithm::CRC8> crcCalc;
    crcCalc.Start();
    crcCalc.AddData(datagram, sizeof(DATAGRAM) - 1);
    datagram->Checksum = crcCalc.Finalize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename DATAGRAM>
bool TMC2209ValidateCRC(const DATAGRAM& datagram)
{
    os::HashCalculator<os::HashAlgorithm::CRC8> crcCalc;
    crcCalc.Start();
    crcCalc.AddData(&datagram, sizeof(DATAGRAM) - 1);
    return datagram.Checksum == crcCalc.Finalize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209IODriver::Setup(const char* controlPortPath, uint32_t baudrate)
{
    m_Baudrate = baudrate;
    m_ControlPort = kopen_trw(controlPortPath, O_RDWR);

    USARTIOCTL_SetBaudrate(m_ControlPort, m_Baudrate);
    USARTIOCTL_SetReadTimeout(m_ControlPort, TimeValNanos::FromMilliseconds(10));
    USARTIOCTL_SetPinMode(m_ControlPort, USARTPin::RX, USARTPinMode::High);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t TMC2209IODriver::ReadRegister(uint8_t chipAddress, uint8_t registerAddress)
{
    CRITICAL_SCOPE(m_Mutex);

    TMC2209ReadRequestDatagram request;

    request.Sync = TMC2209_SYNC;
    request.SlaveAddress = chipAddress & 0x03;
    request.RegisterAddress = uint8_t(registerAddress & ~TMC2209_REG_ADDR_WRITE);
    TMC2209UpdateCRC(&request);

    USARTPin pin = (chipAddress < 4) ? USARTPin::TX : USARTPin::RX;

    WaitForIdle();

    if (pin != m_ActivUARTPin)
    {
        USARTIOCTL_SetPinMode(m_ControlPort, m_ActivUARTPin, USARTPinMode::High);
        m_ActivUARTPin = pin;
        USARTIOCTL_SetSwapRXTX(m_ControlPort, m_ActivUARTPin == USARTPin::RX); // Swap for write if chip connected to RX pin.
        USARTIOCTL_SetPinMode(m_ControlPort, m_ActivUARTPin, USARTPinMode::Normal);
    }

    SerialWrite(&request, sizeof(request));

    USARTIOCTL_SetSwapRXTX(m_ControlPort, m_ActivUARTPin == USARTPin::TX); // Swap for read if chip connected to TX pin.

    TMC2209ReadReplyDatagram reply;

    try
    {
        SerialRead(&reply, sizeof(reply));
    }
    catch(...)
    {
        USARTIOCTL_SetSwapRXTX(m_ControlPort, m_ActivUARTPin == USARTPin::RX);
        throw;
    }

    USARTIOCTL_SetSwapRXTX(m_ControlPort, m_ActivUARTPin == USARTPin::RX); // Swap for write if chip connected to RX pin.
    m_LastActiveTime = kget_monotonic_time_hires();

    if (!TMC2209ValidateCRC(reply)) {
        printf("ERROR: TMC2209IODriver::ReadRegister(%d, %d) invalid checksum\n", chipAddress, registerAddress);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    return (uint32_t(reply.Data[0]) << 24) | (uint32_t(reply.Data[1]) << 16) | (uint32_t(reply.Data[2]) << 8) | uint32_t(reply.Data[3]);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209IODriver::WriteRegister(uint8_t chipAddress, uint8_t registerAddress, uint32_t data)
{
    CRITICAL_SCOPE(m_Mutex);

    USARTPin pin = (chipAddress < 4) ? USARTPin::TX : USARTPin::RX;

    TMC2209WriteDatagram request;

    request.Sync = TMC2209_SYNC;
    request.SlaveAddress = chipAddress & 0x03;
    request.RegisterAddress = uint8_t(registerAddress | TMC2209_REG_ADDR_WRITE);
    request.Data[0] = uint8_t(data >> 24);
    request.Data[1] = uint8_t(data >> 16);
    request.Data[2] = uint8_t(data >> 8);
    request.Data[3] = uint8_t(data);

    TMC2209UpdateCRC(&request);

    WaitForIdle();

    if (pin != m_ActivUARTPin)
    {
        USARTIOCTL_SetPinMode(m_ControlPort, m_ActivUARTPin, USARTPinMode::High);
        m_ActivUARTPin = pin;
        USARTIOCTL_SetSwapRXTX(m_ControlPort, m_ActivUARTPin == USARTPin::RX); // Swap for write if chip connected to RX pin.
        USARTIOCTL_SetPinMode(m_ControlPort, m_ActivUARTPin, USARTPinMode::Normal);
    }



    SerialWrite(&request, sizeof(request));
    m_LastActiveTime = kget_monotonic_time_hires();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209IODriver::SerialRead(void* buffer, size_t length)
{
    uint8_t* dst = static_cast<uint8_t*>(buffer);
    ssize_t totalBytesRead = 0;

    while (totalBytesRead < length)
    {
        const ssize_t bytesRead = kread_trw(m_ControlPort, dst, length - totalBytesRead);
        totalBytesRead += bytesRead;
        dst += bytesRead;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209IODriver::SerialWrite(const void* buffer, size_t length)
{
    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    ssize_t totalBytesWritten = 0;
    while (totalBytesWritten < length)
    {
        ssize_t bytesWritten = kwrite_trw(m_ControlPort, src, length - totalBytesWritten);
        totalBytesWritten += bytesWritten;
        src += bytesWritten;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209IODriver::WaitForIdle()
{
    TimeValNanos curTime = kget_monotonic_time_hires();

    while ((curTime.AsNative() - m_LastActiveTime.AsNative()) < (12LL * 1000000000LL / m_Baudrate)) {
        curTime = kget_monotonic_time_hires();
    }
}

} // namespace kernel
