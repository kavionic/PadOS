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

#include <SerialConsole/SerialCommandHandler.h>
#include <SerialConsole/SerialProtocol.h>
#include <SerialConsole/CommandHandlerFilesystem.h>

#include <Kernel/VFS/FileIO.h>
#include <Utils/HashCalculator.h>



using namespace os;

CommandHandlerFilesystem        g_FilesystemHandler;


SerialCommandHandler* SerialCommandHandler::s_Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialCommandHandler::SerialCommandHandler() : m_SendMutex("SerHandlerSend"/*, false*/)
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

void SerialCommandHandler::Setup(SerialProtocol::ProbeDeviceType deviceType, int file)
{
    m_SerialPort = file;
    m_DeviceType = deviceType;

    REGISTER_KERNEL_LOG_CATEGORY(LogCategorySerialHandler, kernel::KLogSeverity::WARNING);

    RegisterPacketHandler<SerialProtocol::ProbeDevice>(SerialProtocol::Commands::ProbeDevice, this, &SerialCommandHandler::HandleProbeDevice);
    RegisterPacketHandler<SerialProtocol::SetSystemTime>(SerialProtocol::Commands::SetSystemTime, this, &SerialCommandHandler::HandleSetSystemTime);

    g_FilesystemHandler.Setup(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::Execute()
{
    kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Starting serial command handler\n");

    SendMessage<SerialProtocol::RequestSystemTime>();

    SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(new uint8_t[m_LargestCommandPacket]);

    printf("Largest packet size: %u\n", m_LargestCommandPacket);

    for (;;)
    {
        memset(packetBuffer, 0x11, m_LargestCommandPacket);
        size_t size = sizeof(SerialProtocol::PacketHeader::Magic);

        uint8_t magicNumber;
        // Read and validate magic number.
        if (read(m_SerialPort, &magicNumber, 1) != 1)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Failed to read serial packet magic number 1\n");
            continue;
        }
        else if (magicNumber != SerialProtocol::PacketHeader::MAGIC1)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_HIGH_VOL, "Recevied packet with invalid magic number 1 %02x\n", magicNumber);
            continue;
        }
        if (read(m_SerialPort, &magicNumber, 1) != 1)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Failed to read serial packet magic number 2\n");
            continue;
        }
        else if (magicNumber != SerialProtocol::PacketHeader::MAGIC2)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Received packet with invalid magic number 2 %02x\n", magicNumber);
            continue;
        }
        packetBuffer->Magic = SerialProtocol::PacketHeader::MAGIC;
        // Read rest of header.
        size = sizeof(SerialProtocol::PacketHeader) - sizeof(SerialProtocol::PacketHeader::Magic);

        if (read(m_SerialPort, reinterpret_cast<uint8_t*>(packetBuffer) + sizeof(SerialProtocol::PacketHeader::Magic), size) != size)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Failed to read serial packet header\n");
            continue;
        }

        // Validate message size.
        if (packetBuffer->PackageLength > m_LargestCommandPacket) {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Packet to large. Cmd=%d, Size=%d\n", int(packetBuffer->Command), int(int(packetBuffer->PackageLength)));
            continue;
        }

        // Read message body.
        size_t bodyLength = packetBuffer->PackageLength - sizeof(SerialProtocol::PacketHeader);
        if (read(m_SerialPort, packetBuffer + 1, bodyLength) != bodyLength)
        {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Failed to read serial packet body\n");
            continue;
        }

        HashCalculator<HashAlgorithm::CRC32> crcCalc;

        uint32_t receivedCRC = packetBuffer->Checksum;
        packetBuffer->Checksum = 0;
        crcCalc.AddData(packetBuffer, packetBuffer->PackageLength);
        uint32_t calculatedCRC = crcCalc.Finalize();

        if (calculatedCRC != receivedCRC) {
            kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "Invalid CRC32. Got %08x, expected %08x\n", receivedCRC, calculatedCRC);
            continue;
        }

        kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_HIGH_VOL, "Received command %d\n", int(packetBuffer->Command));

        if (packetBuffer->Command == SerialProtocol::Commands::MessageReply)
        {
            SetRetransmitFlag(packetBuffer->Token, false);
            continue;
        }

        if (m_RetransmitFlags != 0)
        {
            TimeValMicros curTime = get_system_time();
            for (uint32_t i = 0; i < SerialProtocol::ReplyTokens::StaticTokenCount; ++i)
            {
                if ((m_RetransmitFlags & (1 << i)) != 0 && curTime >= m_RetransmitTimes[i] && m_RetransmitHandlers[i])
                {
                    SerialProtocol::ReplyTokens::Value replyToken = SerialProtocol::ReplyTokens::Value(i + 1);
                    SetRetransmitFlag(replyToken, false);
                    printf("Retransmit %d\n", int(replyToken));
                    m_RetransmitHandlers[i](replyToken);
                }
            }
        }

        // Decode message.

        auto handler = m_CommandHandlerMap.find(packetBuffer->Command);

        if (handler != m_CommandHandlerMap.end())
        {
            handler->second->HandleMessage(packetBuffer);
            if (handler->second->m_AutoReply) {
                SendMessage<SerialProtocol::MessageReply>(packetBuffer->Token);
            }
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

void SerialCommandHandler::RegisterRetransmitHandler(SerialProtocol::ReplyTokens::Value replyToken, std::function<void(SerialProtocol::ReplyTokens::Value)>&& callback)
{
    CRITICAL_SCOPE(m_SendMutex);
    uint32_t index = uint32_t(uint32_t(replyToken) - 1);
    if (index < SerialProtocol::ReplyTokens::StaticTokenCount)
    {
        m_RetransmitHandlers[index] = std::move(callback);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::SetRetransmitFlag(SerialProtocol::ReplyTokens::Value replyToken, bool value)
{
    uint32_t index = uint32_t(uint32_t(replyToken) - 1);
    if (index < SerialProtocol::ReplyTokens::StaticTokenCount)
    {
        if (value)
        {
            if ((m_RetransmitFlags & (1 << index)) == 0)
            {
                m_RetransmitFlags |= 1 << index;
                m_RetransmitTimes[index] = get_system_time() + TimeValMicros(1.0);
            }
        }
        else
        {
            m_RetransmitFlags &= ~(1 << index);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::GetRetransmitFlag(SerialProtocol::ReplyTokens::Value replyToken) const
{
    CRITICAL_SCOPE(m_SendMutex);
    return m_RetransmitFlags & (1 << (uint16_t(replyToken) - 1));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::HandleProbeDevice(const SerialProtocol::ProbeDevice& packet)
{
    SendMessage<SerialProtocol::ProbeDeviceReply>(m_DeviceType);
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

int SerialCommandHandler::SendSerialData(SerialProtocol::PacketHeader* header, size_t headerSize, const void* data, size_t dataSize)
{
    HashCalculator<HashAlgorithm::CRC32> crcCalc;

    header->Checksum = 0;
    crcCalc.AddData(header, headerSize);
    crcCalc.AddData(data, dataSize);
    header->Checksum = crcCalc.Finalize();

    CRITICAL_SCOPE(m_SendMutex);

    int result = os::FileIO::Write(m_SerialPort, header, headerSize);
    if (result != headerSize) return result;
    if (dataSize > 0) {
        result = os::FileIO::Write(m_SerialPort, data, dataSize);
    }
    if (SerialProtocol::IsStaticReplyToken(header->Token)) {
        SetRetransmitFlag(header->Token, true);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::SendSerialPacket(SerialProtocol::PacketHeader* msg)
{
    HashCalculator<HashAlgorithm::CRC32> crcCalc;

    msg->Checksum = 0;
    crcCalc.AddData(msg, msg->PackageLength);
    msg->Checksum = crcCalc.Finalize();

    CRITICAL_SCOPE(m_SendMutex);
    os::FileIO::Write(m_SerialPort, msg, msg->PackageLength);
    if (SerialProtocol::IsStaticReplyToken(msg->Token)) {
        SetRetransmitFlag(msg->Token, true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int SerialCommandHandler::WriteLogMessage(const char* buffer, int length)
{
    SerialProtocol::LogMessage header;

    header.InitMsg(header);

    header.PackageLength = sizeof(header) + length;

    CRITICAL_SCOPE(m_SendMutex);

    write(m_SerialPort, &header, sizeof(header));
    write(m_SerialPort, buffer, length);

    return length;
}
