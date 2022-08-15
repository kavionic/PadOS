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

#pragma once

#include <algorithm>
#include <functional>
#include <queue>

#include <Signals/SignalTarget.h>
#include <Utils/String.h>
#include <Utils/CircularBuffer.h>
#include <Threads/Thread.h>
#include <Threads/Mutex.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/VFS/KINode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/Kernel.h>
#include <SerialConsole/SerialProtocol.h>

namespace SerialProtocol
{
struct PacketHeader;
struct SetMaterials;
struct SetMaterialsConfig;

struct BufferSensorUpdate;

struct GetDirectory;
struct CreateFile;
struct CreateDirectory;
struct OpenFile;
struct WriteFile;
struct ReadFile;
struct CloseFile;
struct DeleteFile;

struct FilamentDetectorUpdate;
struct MotorStatusUpdate;
struct SelectChannel;
struct ChannelInfoUpdate;

}

DEFINE_KERNEL_LOG_CATEGORY(LogCategorySerialHandler);

class PacketHandlerBase
{
public:
    virtual void HandleMessage(const SerialProtocol::PacketHeader* packet) const = 0;
};

template<typename PacketType, typename CallbackType>
class PacketHandler : public PacketHandlerBase
{
public:
    IFLASHC PacketHandler(CallbackType&& callback) : m_Callback(std::move(callback)) {}

    IFLASHC virtual void HandleMessage(const SerialProtocol::PacketHeader* packet) const override { m_Callback(*static_cast<const PacketType*>(packet)); }

private:
    CallbackType m_Callback;
};



class SerialCommandHandler : public SignalTarget, public os::Thread
{
public:
    IFLASHC SerialCommandHandler();
    IFLASHC ~SerialCommandHandler();

    static IFLASHC SerialCommandHandler& GetInstance();

    IFLASHC virtual int Run() override;


    IFLASHC virtual void Setup(SerialProtocol::ProbeDeviceType deviceType, std::vector<os::String>&& serialPortPaths, int baudrate, int readThreadPriority);
    virtual void ProbeRequestReceived(SerialProtocol::ProbeDeviceType expectedMode) {}

    IFLASHC void Execute();

    template<typename PacketType, typename CallbackType>
    IFLASHC void RegisterPacketHandler(SerialProtocol::Commands::Value commandID, CallbackType&& callback)
    {
        PacketHandler<PacketType, CallbackType>* handler = new PacketHandler<PacketType, CallbackType>(std::move(callback));
        m_CommandHandlerMap[commandID] = handler;

        m_LargestCommandPacket = std::max(m_LargestCommandPacket, sizeof(PacketType));
    }
    template<typename PacketType, typename CallbackType>
    IFLASHC void RegisterPacketHandler(CallbackType&& callback)
    {
        RegisterPacketHandler<PacketType>(PacketType::COMMAND, std::move(callback));
    }

    template<typename PacketType, typename ObjectType, typename CallbackType>
    IFLASHC void RegisterPacketHandler(ObjectType* object, CallbackType callback)
    {
        RegisterPacketHandler<PacketType>(std::bind(callback, object, std::placeholders::_1));
    }

    template<typename PacketType, typename ObjectType, typename CallbackType>
    IFLASHC void RegisterPacketHandler(SerialProtocol::Commands::Value commandID, ObjectType* object, CallbackType callback)
    {
        RegisterPacketHandler<PacketType>(commandID, std::bind(callback, object, std::placeholders::_1));
    }

    IFLASHC bool SendSerialData(SerialProtocol::PacketHeader* header, size_t headerSize, const void* data, size_t dataSize);
    IFLASHC void SendSerialPacket(SerialProtocol::PacketHeader* msg);

    template<typename MSG_TYPE, typename... ARGS>
    IFLASHC void SendMessage(ARGS&&... args)
    {
        MSG_TYPE msg;
        MSG_TYPE::InitMsg(msg, std::forward<ARGS>(args)...);
        SendSerialPacket(&msg);
    }

    IFLASHC ssize_t WriteLogMessage(const void* buffer, size_t length);

private:
    IFLASHC ssize_t SerialRead(void* buffer, size_t length);
    IFLASHC ssize_t SerialWrite(const void* buffer, size_t length);
    IFLASHC bool ReadPacket(SerialProtocol::PacketHeader* packetBuffer, size_t maxLength);

    IFLASHC bool FlushLogBuffer();
    IFLASHC void HandleProbeDevice(const SerialProtocol::ProbeDevice& packet);
    IFLASHC void HandleSetSystemTime(const SerialProtocol::SetSystemTime& packet);

    static SerialCommandHandler* s_Instance;

    mutable kernel::KMutex      m_Mutex;
    kernel::KConditionVariable  m_ReplyCondition;
    kernel::KConditionVariable  m_QueueCondition;
    volatile bool               m_WaitingForReply = false;
    volatile bool               m_ReplyReceived = false;

    std::vector<os::String>     m_SerialPortPaths;
    std::vector<int>            m_SerialPortFiles;
    int                         m_Baudrate = 0;
    int                         m_SerialPort = -1;
    ssize_t                     m_ActiveSerialPortIndex = -1;
    kernel::KObjectWaitGroup    m_SerialPortGroup;

    SerialProtocol::ProbeDeviceType m_DeviceType = SerialProtocol::ProbeDeviceType::Bootloader;

    std::map<SerialProtocol::Commands::Value, const PacketHandlerBase*> m_CommandHandlerMap;
    size_t                                                              m_LargestCommandPacket = 0;

    std::queue<std::vector<uint8_t>>    m_MessageQueue;
    size_t                              m_TotalMessageQueueSize = 0;

    CircularBuffer<uint8_t, 2048>   m_LogBuffer;

    SerialCommandHandler(const SerialCommandHandler &other) = delete;
    SerialCommandHandler& operator=(const SerialCommandHandler &other) = delete;
};
