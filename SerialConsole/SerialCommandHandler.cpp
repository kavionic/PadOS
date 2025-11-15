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

#include <utility>
#include <algorithm>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <PadOS/Time.h>

#include <DeviceControl/USART.h>
#include <SerialConsole/SerialCommandHandler.h>
#include <SerialConsole/SerialProtocol.h>
#include <SerialConsole/CommandHandlerFilesystem.h>

#include <Kernel/VFS/FileIO.h>
#include <Kernel/IRQDispatcher.h>
#include <Utils/HashCalculator.h>

using namespace os;

static constexpr size_t MAX_MESSAGE_QUEUE_SIZE = 1024 * 64;

CommandHandlerFilesystem        g_FilesystemHandler;

SerialCommandHandler* SerialCommandHandler::s_Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialCommandHandler::SerialCommandHandler()
    : Thread("SerialHandler")
    , m_TransmitMutex("sch_xmt_mutex", PEMutexRecursionMode_RaiseError)
    , m_QueueMutex("sch_queue_mutex", PEMutexRecursionMode_RaiseError)
    , m_LogMutex("sch_log_mutes", PEMutexRecursionMode_RaiseError)
    , m_EventCondition("sch_event_cond")
    , m_ReplyCondition("sch_reply_cond")
    , m_QueueCondition("sch_queue_cond")
    , m_WaitGroup("sch_wait_group")
    , m_MessageQueue(*new CircularBuffer<uint8_t, SerialProtocol::MAX_MESSAGE_SIZE * 16, void>)
{
    s_Instance = this;

    m_WaitGroup.AddObject(m_EventCondition);
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
    if (m_SerialPortIn == -1)
    {
        m_SerialPortIn = open(m_SerialPortPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (m_SerialPortIn != -1)
        {
            USARTIOCTL_SetBaudrate(m_SerialPortIn, m_Baudrate);

            m_WaitGroup.AddFile(m_SerialPortIn);
            m_SerialPortOut = reopen_file(m_SerialPortIn, O_WRONLY | O_DIRECT);
            USARTIOCTL_SetWriteTimeout(m_SerialPortOut, TimeValNanos::FromMilliseconds(100));
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
    m_ComFailure = false;
    if (m_SerialPortIn != -1)
    {
        m_WaitGroup.RemoveFile(m_SerialPortIn);
        close(m_SerialPortIn);
        close(m_SerialPortOut);
        m_SerialPortIn = -1;
        m_SerialPortOut = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* SerialCommandHandler::Run()
{
    SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer);
    for (;;)
    {
        if (m_ComFailure) {
            CloseSerialPort();
        }
        if (m_SerialPortIn == -1)
        {
            if (!OpenSerialPort())
            {
                snooze_ms(100);
                continue;
            }
        }
        m_WaitGroup.Wait();
        Tick();
        try
        {
            if (!ReadPacket()) {
                continue;
            }
        }
        PERROR_CATCH([this](PErrorCode error) { m_InMessageBytes = 0; CloseSerialPort(); });

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
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::Setup(SerialProtocol::ProbeDeviceType deviceType, String&& serialPortPath, int baudrate, int readThreadPriority)
{
    m_SerialPortPath = std::move(serialPortPath);
    m_Baudrate = baudrate;

    m_DeviceType = deviceType;

//    kernel::kernel_log_set_category_log_level(LogCategorySerialHandler, PLogSeverity::INFO_HIGH_VOL);

    RegisterPacketHandler<SerialProtocol::ProbeDevice>(SerialProtocol::Commands::ProbeDevice, this, &SerialCommandHandler::HandleProbeDevice);
    RegisterPacketHandler<SerialProtocol::SetSystemTime>(SerialProtocol::Commands::SetSystemTime, this, &SerialCommandHandler::HandleSetSystemTime);

    g_FilesystemHandler.Setup(this);

    Start(PThreadDetachState_Detached, readThreadPriority);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::Execute()
{
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandler, "Starting serial command handler.");
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandler, "Largest packet size: {}", m_LargestCommandPacket);

    for (;;)
    {
        const SerialProtocol::PacketHeader* packetBuffer = nullptr;

        CRITICAL_BEGIN(m_QueueMutex);
        {
            if (m_MessageQueue.IsEmpty()) {
                m_QueueCondition.WaitTimeout(m_QueueMutex, TimeValNanos::FromMilliseconds(SerialProtocol::PING_PERIOD_MS_DEVICE));
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
                p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Unknown serial command {}", int(packetBuffer->Command));
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

ssize_t SerialCommandHandler::SerialWrite(const void* buffer, size_t length)
{
    kassert(m_TransmitMutex.IsLocked());

    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    ssize_t bytesWritten = 0;
    while (bytesWritten < length)
    {
        const ssize_t result = write(m_SerialPortOut, src, length - bytesWritten);
        if (result <= 0)
        {
            if (get_last_error() == EINTR) {
                continue;
            }
            m_ComFailure = true;
            m_EventCondition.WakeupAll();
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

bool SerialCommandHandler::ReadPacket()
{
    for (;;)
    {
        if (m_InMessageBytes == 0)
        {
            const ssize_t length = read_trw(m_SerialPortIn, &m_InMessageBuffer[0], 1);
            if (length == 0 || m_InMessageBuffer[0] != SerialProtocol::PacketHeader::MAGIC1) {
                return false;
            }
            m_InMessageBytes++;
        }
        if (m_InMessageBytes == 1)
        {
            const ssize_t length = read_trw(m_SerialPortIn, &m_InMessageBuffer[1], 1);

            if (length == 0) {
                return false;
            }
            if (m_InMessageBuffer[1] != SerialProtocol::PacketHeader::MAGIC2)
            {
                m_InMessageBytes = 0;
                return false;
            }
            m_InMessageBytes++;
        }
        if (m_InMessageBytes < sizeof(SerialProtocol::PacketHeader))
        {
            const ssize_t length = read_trw(m_SerialPortIn, &m_InMessageBuffer[m_InMessageBytes], sizeof(SerialProtocol::PacketHeader) - m_InMessageBytes);
            if (length == 0) {
                return false;
            }
            m_InMessageBytes += length;
            if (m_InMessageBytes < sizeof(SerialProtocol::PacketHeader)) {
                continue;
            }
        }
        // *** PACKET HEADER READ ***

        SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer);

        if (packetBuffer->PackageLength > SerialProtocol::MAX_MESSAGE_SIZE)
        {
            p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Packet to large. Cmd={}, Size={}", packetBuffer->Command, packetBuffer->PackageLength);
            m_InMessageBytes = 0;
            continue;
        }

        if (m_InMessageBytes < packetBuffer->PackageLength)
        {
            const ssize_t length = read_trw(m_SerialPortIn, &m_InMessageBuffer[m_InMessageBytes], packetBuffer->PackageLength - m_InMessageBytes);
            if (length == 0) {
                return false;
            }
            m_InMessageBytes += length;
            if (m_InMessageBytes < packetBuffer->PackageLength) {
                continue;
            }
        }
        // *** FULL PACKET READ ***

        m_InMessageBytes = 0;

        HashCalculator<HashAlgorithm::CRC32> crcCalc;

        uint32_t receivedCRC = packetBuffer->Checksum;
        packetBuffer->Checksum = 0;
        crcCalc.AddData(packetBuffer, packetBuffer->PackageLength);
        uint32_t calculatedCRC = crcCalc.Finalize();

        if (calculatedCRC != receivedCRC)
        {
            p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Invalid CRC32. Got {:08x}, expected {:08x}", receivedCRC, calculatedCRC);
            continue;
        }

        p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandler, "Received command {}", int(packetBuffer->Command));

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
    set_real_time(TimeValNanos::FromMicroseconds(packet.UnixTime), true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::SendSerialData(SerialProtocol::PacketHeader* header, size_t headerSize, const void* data, size_t dataSize)
{
    if (GetThreadID() == -1 || !IsSerialPortActive())
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
        m_ReplyCondition.WaitTimeout(m_TransmitMutex, TimeValNanos::FromMilliseconds(500));
        if (m_ReplyReceived) {
            return true;
        }
    }
//    p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Transmitting {} failed.", header->Command);
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
    if (!IsSerialPortActive()) {
        return false;
    }

    size_t length = m_LogBuffer.GetLength();

    header.PackageLength = sizeof(header) + length;

    kassert(!m_TransmitMutex.IsLocked());
    CRITICAL_SCOPE(m_TransmitMutex);
    SerialWrite(&header, sizeof(header));
    while (length > 0 && IsSerialPortActive())
    {
        uint8_t buffer[128];
        ssize_t curLength = 0;

        curLength = m_LogBuffer.Read(buffer, std::min(length, sizeof(buffer)));
        length -= curLength;

        SerialWrite(buffer, curLength);
    }
    return true;
}
