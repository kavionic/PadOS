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
#include <map>

#include <Signals/SignalTarget.h>
#include <Utils/String.h>
#include <Utils/MessagePort.h>
#include <Kernel/KLogging.h>
#include <Kernel/KThread.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/Kernel.h>
#include <SerialConsole/SerialProtocol.h>

namespace SerialProtocol
{
struct PacketHeader;

}

PDEFINE_LOG_CATEGORY(LogCategorySerialHandler, "SCMDH", PLogSeverity::WARNING, PLogChannel::DebugPort);

class PacketHandlerBase
{
public:
    virtual void HandleMessage(const SerialProtocol::PacketHeader* packet) const = 0;
};

template<typename PacketType, typename CallbackType>
class PacketHandler : public PacketHandlerBase
{
public:
    PacketHandler(CallbackType&& callback) : m_Callback(std::move(callback)) {}

    virtual void HandleMessage(const SerialProtocol::PacketHeader* packet) const override { m_Callback(*static_cast<const PacketType*>(packet)); }

private:
    CallbackType m_Callback;
};

class PPacketHandlerDispatcher
{
public:
    bool DispatchPacket(const SerialProtocol::PacketHeader* packetBuffer)
    {
        auto handler = m_CommandHandlerMap.find(packetBuffer->Command);
        if (handler == m_CommandHandlerMap.end()) {
            return false;
        }
        handler->second->HandleMessage(packetBuffer);
        return true;
    }

    template<typename PacketType, typename CallbackType>
    void RegisterPacketHandler(SerialProtocol::Commands::Value commandID, CallbackType&& callback)
    {
        PacketHandler<PacketType, CallbackType>* handler = new PacketHandler<PacketType, CallbackType>(std::move(callback));
        m_CommandHandlerMap[commandID] = handler;

        m_LargestCommandPacket = std::max(m_LargestCommandPacket, sizeof(PacketType));
    }

    template<typename PacketType, typename CallbackType>
    void RegisterPacketHandler(CallbackType&& callback)
    {
        RegisterPacketHandler<PacketType>(PacketType::COMMAND, std::move(callback));
    }

    template<typename PacketType, typename ObjectType, typename CallbackType>
    void RegisterPacketHandler(ObjectType* object, CallbackType callback)
    {
        RegisterPacketHandler<PacketType>(std::bind(callback, object, std::placeholders::_1));
    }

    template<typename PacketType, typename ObjectType, typename CallbackType>
    void RegisterPacketHandler(SerialProtocol::Commands::Value commandID, ObjectType* object, CallbackType callback)
    {
        RegisterPacketHandler<PacketType>(commandID, std::bind(callback, object, std::placeholders::_1));
    }

protected:
    size_t                                                              m_LargestCommandPacket = 0;

private:
    std::map<SerialProtocol::Commands::Value, const PacketHandlerBase*> m_CommandHandlerMap;
};

namespace kernel
{

class SerialCommandHandler : public PPacketHandlerDispatcher, public SignalTarget, public KThread
{
public:
    SerialCommandHandler();
    ~SerialCommandHandler();

    static SerialCommandHandler& Get();

    virtual void* Run() override;


    virtual void Setup(SerialProtocol::ProbeDeviceType deviceType, PString&& serialPortPath, int baudrate, int readThreadPriority, int procThreadPriority);
    virtual void ProbeRequestReceived(SerialProtocol::ProbeDeviceType expectedMode) {}

    bool SendSerialData(SerialProtocol::PacketHeader* header, size_t headerSize, const void* data, size_t dataSize);
    void SendSerialPacket(SerialProtocol::PacketHeader* msg);

    template<typename MSG_TYPE, typename... ARGS>
    void SendMessage(ARGS&&... args)
    {
        MSG_TYPE msg;
        MSG_TYPE::InitMsg(msg, std::forward<ARGS>(args)...);
        SendSerialPacket(&msg);
    }

    void SetHandlerMessagePort(port_id portID);

    KObjectWaitGroup& GetWaitGroup() { return m_WaitGroup; }
private:
    static void* InputHandlerThreadEntry(void* args) { return static_cast<SerialCommandHandler*>(args)->HandleIO(); }

    void* HandleIO();

    bool OpenSerialPort();
    void CloseSerialPort();
    bool IsSerialPortActive() const { return !m_ComFailure && m_SerialPortIn != -1; }
    ssize_t SerialWrite(const void* buffer, size_t length);
    bool ReadPacket();

    void HandleProbeDevice(const SerialProtocol::ProbeDevice& packet);
    void HandleSetSystemTime(const SerialProtocol::SetSystemTime& packet);

    static SerialCommandHandler* s_Instance;

    mutable KMutex      m_TransmitMutex;
    mutable KMutex      m_QueueMutex;
    mutable KMutex      m_LogMutex;
    KConditionVariable  m_EventCondition;
    KConditionVariable  m_ReplyCondition;
    KConditionVariable  m_QueueCondition;
    KObjectWaitGroup    m_WaitGroup;
    thread_id           m_InputHandlerThread = INVALID_HANDLE;
    PMessagePort        m_HandlerPort;

    volatile std::atomic_bool     m_ReplyReceived = false;

    PString             m_SerialPortPath;
    int                 m_Baudrate = 0;
    int                 m_SerialPortIn = -1;
    int                 m_SerialPortOut = -1;
    bool                m_ComFailure = false;

    SerialProtocol::ProbeDeviceType m_DeviceType = SerialProtocol::ProbeDeviceType::Bootloader;


    std::deque<std::vector<uint8_t>> m_MessageQueue;
    std::vector<uint8_t> m_InMessageBuffer;
    size_t               m_PackageBytesRead = 0;
    TimeValNanos    m_NextDeviceProbeTime;
    SerialCommandHandler(const SerialCommandHandler& other) = delete;
    SerialCommandHandler& operator=(const SerialCommandHandler& other) = delete;
};

} // namespace kernel
