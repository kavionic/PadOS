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
//            USARTIOCTL_SetWriteTimeout(m_SerialPortOut, TimeValNanos::FromMilliseconds(1000));
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
        PERROR_CATCH([this](PErrorCode error) { m_InMessageBuffer.clear(); CloseSerialPort(); });
        if (m_InMessageBuffer.empty()) {
            continue;
        }
        SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer.data());
        if (packetBuffer->Command == SerialProtocol::Commands::MessageReply)
        {
            m_ReplyReceived = true;
            m_ReplyCondition.WakeupAll();
            m_InMessageBuffer.clear();
        }
        else
        {
            CRITICAL_BEGIN(m_QueueMutex)
            {
                if (m_MessageQueue.size() >= 1000) {
                    m_MessageQueue.pop_front();
                }
                m_MessageQueue.push_back(std::move(m_InMessageBuffer));
                m_InMessageBuffer.clear();
                m_QueueCondition.WakeupAll();
            } CRITICAL_END;
        }
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
        std::vector<uint8_t> packetData;

        CRITICAL_BEGIN(m_QueueMutex);
        {
            if (m_MessageQueue.empty()) {
                m_QueueCondition.WaitDeadline(m_QueueMutex, m_NextDeviceProbeTime);
            }
            if (!m_MessageQueue.empty())
            {
                packetData = std::move(m_MessageQueue.front());
                m_MessageQueue.pop_front();
            }
        } CRITICAL_END;

        const TimeValNanos curTime = get_monotonic_time();
        if (curTime > m_NextDeviceProbeTime) {
            m_NextDeviceProbeTime = curTime + TimeValNanos::FromMilliseconds(SerialProtocol::PING_PERIOD_MS_DEVICE);
            SendMessage<SerialProtocol::ProbeDeviceReply>(m_DeviceType);
        }
        if (!packetData.empty())
        {
            const SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<const SerialProtocol::PacketHeader*>(packetData.data());

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
        if (m_InMessageBuffer.size() < sizeof(SerialProtocol::PacketHeader))
        {
            uint8_t data[sizeof(SerialProtocol::PacketHeader)];
            const ssize_t length = read_trw(m_SerialPortIn, data, sizeof(SerialProtocol::PacketHeader) - m_InMessageBuffer.size());
            if (length == 0) {
                return false;
            }
            m_InMessageBuffer.insert(m_InMessageBuffer.end(), &data[0], &data[length]);

            if (m_InMessageBuffer.size() >= 1 && m_InMessageBuffer[0] != SerialProtocol::PacketHeader::MAGIC1)
            {
                auto end = m_InMessageBuffer.end();
                for (auto i = m_InMessageBuffer.begin(); i != end; ++i)
                {
                    if (*i == SerialProtocol::PacketHeader::MAGIC1)
                    {
                        end = i;
                        break;
                    }
                }
                m_InMessageBuffer.erase(m_InMessageBuffer.begin(), end);
            }

            if (m_InMessageBuffer.size() >= 2)
            {
                if (m_InMessageBuffer[1] != SerialProtocol::PacketHeader::MAGIC2)
                {
                    m_InMessageBuffer.erase(m_InMessageBuffer.begin(), m_InMessageBuffer.begin() + 2);
                    continue;
                }
            }
            if (m_InMessageBuffer.size() < sizeof(SerialProtocol::PacketHeader)) {
                continue;
            }
        }
        // *** PACKET HEADER READ ***

        const size_t packageLength = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer.data())->PackageLength;

        if (packageLength > SerialProtocol::MAX_MESSAGE_SIZE)
        {
            p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Packet to large. Cmd={}, Size={}", reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer.data())->Command, packageLength);
            m_InMessageBuffer.clear();
            continue;
        }

        if (m_InMessageBuffer.size() < packageLength)
        {
            size_t prevSize = m_InMessageBuffer.size();
            m_InMessageBuffer.resize(packageLength);

            const ssize_t length = read_trw(m_SerialPortIn, &m_InMessageBuffer[prevSize], packageLength - prevSize);
            if (length == 0) {
                m_InMessageBuffer.resize(prevSize);
                return false;
            }

            if (prevSize + length < packageLength) {
                m_InMessageBuffer.resize(prevSize + length);
                continue;
            }
        }
        // *** FULL PACKET READ ***

        SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer.data());

        HashCalculator<HashAlgorithm::CRC32> crcCalc;

        uint32_t receivedCRC = packetBuffer->Checksum;
        packetBuffer->Checksum = 0;
        crcCalc.AddData(packetBuffer, packetBuffer->PackageLength);
        uint32_t calculatedCRC = crcCalc.Finalize();

        if (calculatedCRC != receivedCRC)
        {
            m_InMessageBuffer.clear();
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
    p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Transmitting {} failed.", header->Command);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::SendSerialPacket(SerialProtocol::PacketHeader* msg)
{
    SendSerialData(msg, msg->PackageLength, nullptr, 0);
}
