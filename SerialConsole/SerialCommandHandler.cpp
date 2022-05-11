// This file is part of PadOS.
//
// Copyright (C) 2020-2022 Kurt Skauen <http://kavionic.com/>
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
#include <Utils/HashCalculator.h>

using namespace os;

static constexpr size_t MAX_MESSAGE_QUEUE_SIZE = 1024 * 64;

CommandHandlerFilesystem        g_FilesystemHandler;

SerialCommandHandler* SerialCommandHandler::s_Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialCommandHandler::SerialCommandHandler() : Thread("SerialHandler"), m_Mutex("SerialHandler", false), m_ReplyCondition("sch_reply_cond"), m_QueueCondition("sch_queue_cond")
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

SerialCommandHandler& SerialCommandHandler::GetInstance()
{
    return *s_Instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int SerialCommandHandler::Run()
{
    SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(new uint8_t[m_LargestCommandPacket]);
    for (;;)
    {
        if (!ReadPacket(packetBuffer, m_LargestCommandPacket, false)) {
            continue;
        }
        {
            CRITICAL_SCOPE(m_Mutex);

            if (packetBuffer->Command == SerialProtocol::Commands::MessageReply)
            {
                if (m_WaitingForReply)
                {
                    m_ReplyReceived = true;
                    m_ReplyCondition.WakeupAll();
                }
            }
            else
            {
                if ((m_TotalMessageQueueSize + packetBuffer->PackageLength) > MAX_MESSAGE_QUEUE_SIZE)
                {
                    continue;
                }
                if ((packetBuffer->Flags & SerialProtocol::PacketHeader::FLAG_NO_REPLY) == 0) {
                    SendMessage<SerialProtocol::MessageReply>();
                }

                std::vector<uint8_t> buffer;
                buffer.resize(packetBuffer->PackageLength);
                memcpy(buffer.data(), packetBuffer, packetBuffer->PackageLength);
                m_MessageQueue.push(std::move(buffer));
                m_TotalMessageQueueSize += packetBuffer->PackageLength;
                m_QueueCondition.WakeupAll();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::Setup(SerialProtocol::ProbeDeviceType deviceType, int file, int readThreadPriority)
{
    m_SerialPort = file;
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

    printf("Largest packet size: %u\n", m_LargestCommandPacket);

    for (;;)
    {
        std::vector<uint8_t> buffer;
        {
            CRITICAL_SCOPE(m_Mutex);

            while (m_MessageQueue.empty())
            {
                m_QueueCondition.Wait(m_Mutex);
            }
            buffer = std::move(m_MessageQueue.front());
            m_MessageQueue.pop();
        }

        const SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<const SerialProtocol::PacketHeader*>(buffer.data());

        m_TotalMessageQueueSize -= packetBuffer->PackageLength;

        auto handler = m_CommandHandlerMap.find(packetBuffer->Command);

        if (handler != m_CommandHandlerMap.end())
        {
            handler->second->HandleMessage(packetBuffer);
        }
        else
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Unknown serial command %d\n", int(packetBuffer->Command));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::ReadPacket(SerialProtocol::PacketHeader* packetBuffer, size_t maxLength, bool repliesOnly)
{
    for (;;)
    {
        memset(packetBuffer, 0x11, maxLength);
        size_t size = sizeof(SerialProtocol::PacketHeader::Magic);

        uint8_t magicNumber;
        // Read and validate magic number.
        if (read(m_SerialPort, &magicNumber, 1) != 1)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Failed to read serial packet magic number 1\n");
            return false;
        }
        else if (magicNumber != SerialProtocol::PacketHeader::MAGIC1)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_HIGH_VOL, "Recevied packet with invalid magic number 1 %02x\n", magicNumber);
            return false;
        }
        if (read(m_SerialPort, &magicNumber, 1) != 1)
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

        if (read(m_SerialPort, reinterpret_cast<uint8_t*>(packetBuffer) + sizeof(SerialProtocol::PacketHeader::Magic), size) != size)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Failed to read serial packet header\n");
            return false;
        }

        if (packetBuffer->PackageLength > m_LargestCommandPacket)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Packet to large. Cmd=%d, Size=%d\n", int(packetBuffer->Command), int(int(packetBuffer->PackageLength)));
            return false;
        }

        if (repliesOnly && packetBuffer->Command != SerialProtocol::Commands::MessageReply)
        {
            size_t remainingBytes = packetBuffer->PackageLength - sizeof(SerialProtocol::PacketHeader);
            while (remainingBytes != 0)
            {
                size_t curLength = std::min(remainingBytes, maxLength);
                remainingBytes -= curLength;
                if (read(m_SerialPort, packetBuffer, curLength) != curLength)
                {
                    kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Failed to purge non-reply packet\n");
                    return false;
                }
            }
            printf("Skipped non reply packet.\n");
            continue;
        }

        // Validate message size.
        if (packetBuffer->PackageLength > maxLength)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Packet to large. Cmd=%d, Size=%d\n", int(packetBuffer->Command), int(int(packetBuffer->PackageLength)));
            return false;
        }

        // Read message body.
        size_t bodyLength = packetBuffer->PackageLength - sizeof(SerialProtocol::PacketHeader);
        if (read(m_SerialPort, packetBuffer + 1, bodyLength) != bodyLength)
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
    if (GetThreadID() == -1)
    {
        return false;
    }
    HashCalculator<HashAlgorithm::CRC32> crcCalc;

    header->Checksum = 0;
    crcCalc.AddData(header, headerSize);
    crcCalc.AddData(data, dataSize);
    header->Checksum = crcCalc.Finalize();

    const bool recursing = get_thread_id() == GetThreadID();
    {
        CRITICAL_SCOPE(m_Mutex, !recursing);

        if (header->Command != SerialProtocol::Commands::MessageReply)
        {
            while (m_WaitingForReply) {
                m_ReplyCondition.Wait(m_Mutex);
            }
        }
        for (int i = 0; i < 10; ++i)
        {
            int result = os::FileIO::Write(m_SerialPort, header, headerSize);
            if (result != headerSize) return result;
            if (dataSize > 0) {
                result = os::FileIO::Write(m_SerialPort, data, dataSize);
            }
            if (header->Flags & SerialProtocol::PacketHeader::FLAG_NO_REPLY) {
                return true;
            }

            m_ReplyReceived = false;
            m_WaitingForReply = true;
            m_ReplyCondition.WaitTimeout(m_Mutex, TimeValMicros::FromMilliseconds(100));
            m_WaitingForReply = false;
            m_ReplyCondition.WakeupAll();
            if (m_ReplyReceived) {
                return true;
            }
        }
    }
    printf("ERROR: transmitting %ld failed.\n", header->Command);
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

int SerialCommandHandler::WriteLogMessage(const char* buffer, int length)
{
    SerialProtocol::LogMessage header;

    header.InitMsg(header);

    header.PackageLength = sizeof(header) + length;

    CRITICAL_SCOPE(m_Mutex, get_thread_id() != GetThreadID());

    write(m_SerialPort, &header, sizeof(header));
    write(m_SerialPort, buffer, length);

    return length;
}
