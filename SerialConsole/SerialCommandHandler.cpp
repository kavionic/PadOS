// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 20.01.2020

#include <algorithm>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <cstring>

#include <DeviceControl/USART.h>
#include <SerialConsole/SerialCommandHandler.h>
#include <SerialConsole/SerialProtocol.h>
#include <SerialConsole/CommandHandlerFilesystem.h>

#include <Kernel/VFS/FileIO.h>
#include <Kernel/IRQDispatcher.h>
#include <Utils/HashCalculator.h>

using namespace os;
using namespace kernel;

static constexpr size_t MAX_MESSAGE_QUEUE_SIZE = 1024 * 64;

CommandHandlerFilesystem        g_FilesystemHandler;

SerialCommandHandler* SerialCommandHandler::s_Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialCommandHandler::SerialCommandHandler()
    : Thread("SerialHandler")
    , m_TransmitMutex("sch_xmt_mutex", EMutexRecursionMode::RaiseError)
    , m_QueueMutex("sch_queue_mutex", EMutexRecursionMode::RaiseError)
    , m_LogMutex("sch_log_mutes", EMutexRecursionMode::RaiseError)
    , m_ReplyCondition("sch_reply_cond")
    , m_QueueCondition("sch_queue_cond")
{
    s_Instance = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialCommandHandler::~SerialCommandHandler()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialCommandHandler& SerialCommandHandler::Get()
{
    return *s_Instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::OpenSerialPort()
{
    if (m_SerialPort == -1)
    {
        m_SerialPort = FileIO::Open(m_SerialPortPath.c_str(), O_RDWR | O_DIRECT);
        if (m_SerialPort != -1)
        {
            USARTIOCTL_SetWriteTimeout(m_SerialPort, TimeValMicros::FromMilliseconds(100));
            USARTIOCTL_SetBaudrate(m_SerialPort, m_Baudrate);
            return true;
        }
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::CloseSerialPort()
{
    if (m_SerialPort != -1)
    {
        FileIO::Close(m_SerialPort);
        m_SerialPort = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int SerialCommandHandler::Run()
{
    SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer);
    for (;;)
    {
        if (m_SerialPort == -1)
        {
            if (!OpenSerialPort())
            {
                snooze_ms(100);
                continue;
            }
        }
        if (!ReadPacket(packetBuffer, SerialProtocol::MAX_MESSAGE_SIZE)) {
            continue;
        }
        CRITICAL_BEGIN(m_QueueMutex)
        {
            if (packetBuffer->Command == SerialProtocol::Commands::MessageReply)
            {
                m_ReplyReceived = true;
                m_ReplyCondition.WakeupAll();
            }
            else
            {
                if (packetBuffer->PackageLength > m_MessageQueue.GetRemainingSpace()) {
                    continue;
                }
                m_MessageQueue.Write(packetBuffer, packetBuffer->PackageLength);
                m_QueueCondition.WakeupAll();
            }
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::Setup(SerialProtocol::ProbeDeviceType deviceType, String&& serialPortPath, int baudrate, int readThreadPriority)
{
    m_SerialPortPath = std::move(serialPortPath);
    m_Baudrate = baudrate;

    m_DeviceType = deviceType;

    REGISTER_KERNEL_LOG_CATEGORY(LogCategorySerialHandler, kernel::KLogSeverity::WARNING);

//    kernel::kernel_log_set_category_log_level(LogCategorySerialHandler, kernel::KLogSeverity::INFO_HIGH_VOL);

    RegisterPacketHandler<SerialProtocol::ProbeDevice>(SerialProtocol::Commands::ProbeDevice, this, &SerialCommandHandler::HandleProbeDevice);
    RegisterPacketHandler<SerialProtocol::SetSystemTime>(SerialProtocol::Commands::SetSystemTime, this, &SerialCommandHandler::HandleSetSystemTime);

    g_FilesystemHandler.Setup(this);

    Start(false, readThreadPriority);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::Execute()
{
    kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Starting serial command handler\n");
    kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Largest packet size: %u\n", m_LargestCommandPacket);

    for (;;)
    {
        const SerialProtocol::PacketHeader* packetBuffer = nullptr;

        CRITICAL_BEGIN(m_QueueMutex);
        {
            if (m_MessageQueue.IsEmpty()) {
                m_QueueCondition.WaitTimeout(m_QueueMutex, TimeValMicros::FromMilliseconds(SerialProtocol::PING_PERIOD_MS_DEVICE));
            }
            if (!m_MessageQueue.IsEmpty())
            {
                m_MessageQueue.Read(m_OutMessageBuffer, sizeof(SerialProtocol::PacketHeader));
                packetBuffer = reinterpret_cast<const SerialProtocol::PacketHeader*>(m_OutMessageBuffer);
                m_MessageQueue.Read(&m_OutMessageBuffer[sizeof(SerialProtocol::PacketHeader)], packetBuffer->PackageLength - sizeof(SerialProtocol::PacketHeader));
            }
        } CRITICAL_END;

        if (packetBuffer != nullptr)
        {
            auto handler = m_CommandHandlerMap.find(packetBuffer->Command);
            if (handler != m_CommandHandlerMap.end())
            {
                if ((packetBuffer->Flags & SerialProtocol::PacketHeader::FLAG_NO_REPLY) == 0) {
                    SendMessage<SerialProtocol::MessageReply>();
                }
                handler->second->HandleMessage(packetBuffer);
            }
            else
            {
                kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Unknown serial command %d\n", int(packetBuffer->Command));
            }
        }
        else
        {
            FlushLogBuffer();
            SendMessage<SerialProtocol::ProbeDeviceReply>(m_DeviceType);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SerialCommandHandler::SerialRead(void* buffer, size_t length)
{
    uint8_t* dst = static_cast<uint8_t*>(buffer);
    ssize_t bytesRead = 0;
    while (bytesRead < length)
    {
        const ssize_t result = FileIO::Read(m_SerialPort, dst, length - bytesRead);
        if (result <= 0 && get_last_error() != EINTR)
        {
            CloseSerialPort();
            return result;
        }
        bytesRead += result;
        dst += result;
    }
    return bytesRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SerialCommandHandler::SerialWrite(const void* buffer, size_t length)
{
    kassert(m_TransmitMutex.IsLocked());

    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    ssize_t bytesWritten = 0;
    while (bytesWritten < length)
    {
        const ssize_t result = FileIO::Write(m_SerialPort, src, length - bytesWritten);
        if (result <= 0)
        {
            if (get_last_error() == EINTR) {
                continue;
            }
            CloseSerialPort();
            return result;
        }
        bytesWritten += result;
        src += result;
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::ReadPacket(SerialProtocol::PacketHeader* packetBuffer, size_t maxLength)
{
    for (;;)
    {
        size_t size = sizeof(SerialProtocol::PacketHeader::Magic);

        uint8_t magicNumber;
        // Read and validate magic number.
        ssize_t length = FileIO::Read(m_SerialPort, &magicNumber, 1);
        if (length != 1)
        {
            if (length != 0) {
                CloseSerialPort();
                snooze_ms(100);
            }
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Failed to read serial packet magic number 1\n");
            return false;
        }
        else if (magicNumber != SerialProtocol::PacketHeader::MAGIC1)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_HIGH_VOL, "Received packet with invalid magic number 1 %02x\n", magicNumber);
            return false;
        }
        if (SerialRead(&magicNumber, 1) != 1)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Failed to read serial packet magic number 2\n");
            return false;
        }
        else if (magicNumber != SerialProtocol::PacketHeader::MAGIC2)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Received packet with invalid magic number 2 %02x\n", magicNumber);
            return false;
        }
        packetBuffer->Magic = SerialProtocol::PacketHeader::MAGIC;
        // Read rest of header.
        size = sizeof(SerialProtocol::PacketHeader) - sizeof(SerialProtocol::PacketHeader::Magic);

        if (SerialRead(reinterpret_cast<uint8_t*>(packetBuffer) + sizeof(SerialProtocol::PacketHeader::Magic), size) != size)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Failed to read serial packet header\n");
            return false;
        }

        if (packetBuffer->PackageLength > SerialProtocol::MAX_MESSAGE_SIZE)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Packet to large. Cmd=%d, Size=%d\n", int(packetBuffer->Command), int(int(packetBuffer->PackageLength)));
            return false;
        }

        // Validate message size.
        if (packetBuffer->PackageLength > maxLength)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Packet to large. Cmd=%d, Size=%d\n", int(packetBuffer->Command), int(int(packetBuffer->PackageLength)));
            return false;
        }

        // Read message body.
        size_t bodyLength = packetBuffer->PackageLength - sizeof(SerialProtocol::PacketHeader);
        if (SerialRead(packetBuffer + 1, bodyLength) != bodyLength)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Failed to read serial packet body\n");
            return false;
        }

        HashCalculator<HashAlgorithm::CRC32> crcCalc;

        uint32_t receivedCRC = packetBuffer->Checksum;
        packetBuffer->Checksum = 0;
        crcCalc.AddData(packetBuffer, packetBuffer->PackageLength);
        uint32_t calculatedCRC = crcCalc.Finalize();

        if (calculatedCRC != receivedCRC)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Invalid CRC32. Got %08x, expected %08x\n", receivedCRC, calculatedCRC);
            return false;
        }

        kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_HIGH_VOL, "Received command %d\n", int(packetBuffer->Command));

        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::HandleProbeDevice(const SerialProtocol::ProbeDevice& packet)
{
    SendMessage<SerialProtocol::ProbeDeviceReply>(m_DeviceType);
    ProbeRequestReceived(packet.DeviceType);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::HandleSetSystemTime(const SerialProtocol::SetSystemTime& packet)
{
    set_real_time(TimeValMicros::FromMicroseconds(packet.UnixTime), true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::SendSerialData(SerialProtocol::PacketHeader* header, size_t headerSize, const void* data, size_t dataSize)
{
    if (GetThreadID() == -1 || m_SerialPort == -1)
    {
        return false;
    }
    HashCalculator<HashAlgorithm::CRC32> crcCalc;

    header->Checksum = 0;
    crcCalc.AddData(header, headerSize);
    crcCalc.AddData(data, dataSize);
    header->Checksum = crcCalc.Finalize();

    kassert(!m_TransmitMutex.IsLocked());
    CRITICAL_SCOPE(m_TransmitMutex);

    for (int i = 0; i < 3; ++i)
    {
        m_ReplyReceived = false;

        int result = SerialWrite(header, headerSize);
        if (result != headerSize) {
            return false;
        }
        if (dataSize > 0)
        {
            result = SerialWrite(data, dataSize);
            if (result != dataSize) {
                return false;
            }
        }
        if (header->Flags & SerialProtocol::PacketHeader::FLAG_NO_REPLY) {
            return true;
        }
        m_ReplyCondition.WaitTimeout(TimeValMicros::FromMilliseconds(500));
        if (m_ReplyReceived) {
            return true;
        }
    }
    kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "ERROR: transmitting %ld failed.\n", header->Command);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::SendSerialPacket(SerialProtocol::PacketHeader* msg)
{
    SendSerialData(msg, msg->PackageLength, nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SerialCommandHandler::WriteLogMessage(const void* buffer, size_t length)
{
    kassert(!is_in_isr());

    kassert(!m_TransmitMutex.IsLocked());
    kassert(!m_LogMutex.IsLocked());
    CRITICAL_SCOPE(m_LogMutex);

    const uint8_t* src = static_cast<const uint8_t*>(buffer);

    size_t bytesWritten = 0;
    bytesWritten = m_LogBuffer.Write(src, length);

    if (m_LogBuffer.GetLength() > 0) {
        m_QueueCondition.WakeupAll();
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::FlushLogBuffer()
{
    kassert(!m_LogMutex.IsLocked());
    CRITICAL_SCOPE(m_LogMutex);
  
    SerialProtocol::LogMessage header;
    header.InitMsg(header);

    if (m_LogBuffer.GetLength() == 0) {
        return false;
    }
    if (m_SerialPort == -1) {
        return false;
    }

    size_t length = m_LogBuffer.GetLength();

    header.PackageLength = sizeof(header) + length;

    kassert(!m_TransmitMutex.IsLocked());
    CRITICAL_SCOPE(m_TransmitMutex);
    SerialWrite(&header, sizeof(header));
    while (length > 0)
    {
        uint8_t buffer[128];
        ssize_t curLength = 0;

        curLength = m_LogBuffer.Read(buffer, std::min(length, sizeof(buffer)));
        length -= curLength;

        SerialWrite(buffer, curLength);
    }
    return true;
}
