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

namespace kernel
{

static constexpr size_t MAX_MESSAGE_QUEUE_SIZE = 1024 * 64;

CommandHandlerFilesystem        g_FilesystemHandler;

SerialCommandHandler* SerialCommandHandler::s_Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialCommandHandler::SerialCommandHandler()
    : KThread("SerialHandlerProc")
    , m_TransmitMutex("sch_xmt_mutex", PEMutexRecursionMode_RaiseError)
    , m_QueueMutex("sch_queue_mutex", PEMutexRecursionMode_RaiseError)
    , m_LogMutex("sch_log_mutes", PEMutexRecursionMode_RaiseError)
    , m_EventCondition("sch_event_cond")
    , m_ReplyCondition("sch_reply_cond")
    , m_QueueCondition("sch_queue_cond")
    , m_WaitGroup("sch_wait_group")
    , m_HandlerPort(INVALID_HANDLE)
{
    assert(s_Instance == nullptr);

    s_Instance = this;

    m_WaitGroup.AddObject_trw(&m_EventCondition);
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

void* SerialCommandHandler::HandleIO()
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
        m_WaitGroup.Wait_trw();

        try
        {
            if (!ReadPacket()) {
                continue;
            }
        }
        PERROR_CATCH([this](PErrorCode error)
            {
                m_InMessageBuffer.clear();
                m_PackageBytesRead = 0;
                CloseSerialPort();
            }
        );
        if (m_PackageBytesRead == 0) {
            continue;
        }
        SerialProtocol::PacketHeader* packetBuffer = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer.data());
        if (packetBuffer->Command == SerialProtocol::Commands::MessageReply)
        {
            m_ReplyReceived = true;
            m_ReplyCondition.WakeupAll();
            m_InMessageBuffer.clear();
            m_PackageBytesRead = 0;
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
                m_PackageBytesRead = 0;
                m_QueueCondition.WakeupAll();
            } CRITICAL_END;
        }
    }

    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SerialCommandHandler::OpenSerialPort()
{
    try
    {
        if (m_SerialPortIn == -1)
        {
            m_SerialPortIn = kopen_trw(m_SerialPortPath.c_str(), O_RDONLY | O_NONBLOCK);
            if (m_SerialPortIn != -1)
            {
                USARTIOCTL_SetBaudrate(m_SerialPortIn, m_Baudrate);

                m_WaitGroup.AddFile_trw(m_SerialPortIn);
                m_SerialPortOut = kreopen_file_trw(m_SerialPortIn, O_WRONLY | O_DIRECT);
                USARTIOCTL_SetWriteTimeout(m_SerialPortOut, TimeValNanos::FromMilliseconds(1000));
                return true;
            }
            return false;
        }
        return true;
    }
    PERROR_CATCH_RET([](const std::exception& exc, PErrorCode error) { return false; });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::CloseSerialPort()
{
    m_ComFailure = false;
    if (m_SerialPortIn != -1)
    {
        try {
            m_WaitGroup.RemoveFile_trw(m_SerialPortIn);
        }
        catch (const std::exception&) {}

        kclose(m_SerialPortIn);
        kclose(m_SerialPortOut);
        m_SerialPortIn = -1;
        m_SerialPortOut = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* SerialCommandHandler::Run()
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

            if ((packetBuffer->Flags & SerialProtocol::PacketHeader::FLAG_NO_REPLY) == 0) {
                SendMessage<SerialProtocol::MessageReply>();
            }

            if (!DispatchPacket(packetBuffer))
            {
                if (m_HandlerPort.GetHandle() != INVALID_HANDLE) {
                    m_HandlerPort.SendMessageTimeout(INVALID_HANDLE, packetBuffer->Command, packetData.data(), packetData.size(), TimeValNanos::FromSeconds(10.0));
                } else {
                    p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Unknown serial command {}", int(packetBuffer->Command));
                }
            }
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialCommandHandler::Setup(SerialProtocol::ProbeDeviceType deviceType, PString&& serialPortPath, int baudrate, int readThreadPriority, int procThreadPriority)
{
    m_SerialPortPath = std::move(serialPortPath);
    m_Baudrate = baudrate;

    m_DeviceType = deviceType;

    //    kernel::kernel_log_set_category_log_level(LogCategorySerialHandler, PLogSeverity::INFO_HIGH_VOL);

    RegisterPacketHandler<SerialProtocol::ProbeDevice>(SerialProtocol::Commands::ProbeDevice, this, &SerialCommandHandler::HandleProbeDevice);
    RegisterPacketHandler<SerialProtocol::SetSystemTime>(SerialProtocol::Commands::SetSystemTime, this, &SerialCommandHandler::HandleSetSystemTime);

    g_FilesystemHandler.Setup(this);

    Start_trw(PThreadDetachState_Detached, readThreadPriority);

    PThreadAttribs attrs("SerialHandlerIO", readThreadPriority, PThreadDetachState_Detached, 16384);
    m_InputHandlerThread = kthread_spawn_trw(&attrs, nullptr, true, InputHandlerThreadEntry, this);
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
        if (m_PackageBytesRead < sizeof(SerialProtocol::PacketHeader))
        {
            if (m_InMessageBuffer.empty()) {
                m_InMessageBuffer.resize(sizeof(SerialProtocol::PacketHeader));
            }
            const ssize_t length = read_trw(m_SerialPortIn, &m_InMessageBuffer[m_PackageBytesRead], sizeof(SerialProtocol::PacketHeader) - m_PackageBytesRead);
            if (length == 0) {
                return false;
            }
            m_PackageBytesRead += length;

            if (m_PackageBytesRead >= 1 && m_InMessageBuffer[0] != SerialProtocol::PacketHeader::MAGIC1)
            {
                size_t end = m_PackageBytesRead;
                for (size_t i = 0; i < m_PackageBytesRead; ++i)
                {
                    if (m_InMessageBuffer[i] == SerialProtocol::PacketHeader::MAGIC1)
                    {
                        end = i;
                        break;
                    }
                }
                memmove(&m_InMessageBuffer[0], &m_InMessageBuffer[end], m_PackageBytesRead - end);
                m_PackageBytesRead -= end;
            }

            if (m_PackageBytesRead >= 2)
            {
                if (m_InMessageBuffer[1] != SerialProtocol::PacketHeader::MAGIC2)
                {
                    memmove(&m_InMessageBuffer[0], &m_InMessageBuffer[2], m_PackageBytesRead - 2);
                    m_PackageBytesRead -= 2;
                    continue;
                }
            }
            if (m_PackageBytesRead < sizeof(SerialProtocol::PacketHeader)) {
                continue;
            }
        }
        // *** PACKET HEADER READ ***

        const size_t packageLength = reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer.data())->PackageLength;

        if (packageLength > SerialProtocol::MAX_MESSAGE_SIZE)
        {
            p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Packet to large. Cmd={}, Size={}", reinterpret_cast<SerialProtocol::PacketHeader*>(m_InMessageBuffer.data())->Command, packageLength);
            m_InMessageBuffer.clear();
            m_PackageBytesRead = 0;
            continue;
        }

        if (m_InMessageBuffer.size() < packageLength) {
            m_InMessageBuffer.resize(packageLength);
        }
        if (m_PackageBytesRead < packageLength)
        {

            const ssize_t length = read_trw(m_SerialPortIn, &m_InMessageBuffer[m_PackageBytesRead], packageLength - m_PackageBytesRead);
            if (length == 0) {
                return false;
            }
            m_PackageBytesRead += length;
            if (m_PackageBytesRead < packageLength) {
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
            m_PackageBytesRead = 0;
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
    kset_real_time(TimeValNanos::FromMicroseconds(packet.UnixTime), true);
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

void SerialCommandHandler::SetHandlerMessagePort(port_id portID)
{
    m_HandlerPort.SetHandle(portID);
}

} // namespace kernel
