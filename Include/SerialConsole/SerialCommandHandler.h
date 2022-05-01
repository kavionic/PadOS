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

#include <Signals/SignalTarget.h>
#include <Threads/Mutex.h>
#include <Kernel/KMutex.h>
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
    PacketHandlerBase(bool autoReply) : m_AutoReply(autoReply) {}
    virtual void HandleMessage(const SerialProtocol::PacketHeader* packet) const = 0;
    bool    m_AutoReply = false;
};

template<typename PacketType, typename CallbackType>
class PacketHandler : public PacketHandlerBase
{
public:
    PacketHandler(CallbackType&& callback, bool autoReply) : PacketHandlerBase(autoReply),  m_Callback(std::move(callback)) {}

    virtual void HandleMessage(const SerialProtocol::PacketHeader* packet) const override { m_Callback(*static_cast<const PacketType*>(packet)); }

private:
    CallbackType m_Callback;
};



class SerialCommandHandler : public SignalTarget
{
public:
	SerialCommandHandler();
	~SerialCommandHandler();

    static SerialCommandHandler& GetInstance();

	virtual void Setup(SerialProtocol::ProbeDeviceType deviceType, int file);

	void Execute();

    template<typename PacketType, typename CallbackType>
    void RegisterPacketHandler(SerialProtocol::Commands::Value commandID, CallbackType&& callback, bool autoReply)
    {
        PacketHandler<PacketType, CallbackType>* handler = new PacketHandler<PacketType, CallbackType>(std::move(callback), autoReply);
        m_CommandHandlerMap[commandID] = handler;

        m_LargestCommandPacket = std::max(m_LargestCommandPacket, sizeof(PacketType));
    }
    template<typename PacketType, typename CallbackType>
    void RegisterPacketHandler(CallbackType&& callback, bool autoReply)
    {
        RegisterPacketHandler<PacketType>(PacketType::COMMAND, std::move(callback), autoReply);
    }

    template<typename PacketType, typename ObjectType, typename CallbackType>
    void RegisterPacketHandler(ObjectType* object, CallbackType callback, bool autoReply = true)
    {
        RegisterPacketHandler<PacketType>(std::bind(callback, object, std::placeholders::_1), autoReply);
    }

    template<typename PacketType, typename ObjectType, typename CallbackType>
    void RegisterPacketHandler(SerialProtocol::Commands::Value commandID, ObjectType* object, CallbackType callback, bool autoReply = true)
    {
        RegisterPacketHandler<PacketType>(commandID, std::bind(callback, object, std::placeholders::_1), autoReply);
    }

    void RegisterRetransmitHandler(SerialProtocol::ReplyTokens::Value replyToken, std::function<void(SerialProtocol::ReplyTokens::Value)>&& callback);

    template<typename ObjectType, typename CallbackType>
    void RegisterRetransmitHandler(SerialProtocol::ReplyTokens::Value replyToken, ObjectType* object, CallbackType callback)
    {
        RegisterRetransmitHandler(replyToken, std::bind(callback, object, std::placeholders::_1));
    }

    bool GetRetransmitFlag(SerialProtocol::ReplyTokens::Value replyToken) const;

    int  SendSerialData(SerialProtocol::PacketHeader* header, size_t headerSize, const void* data, size_t dataSize);
	void SendSerialPacket(SerialProtocol::PacketHeader* msg);


	template<typename MSG_TYPE, typename... ARGS>
	void SendMessage(ARGS&&... args)
	{
		MSG_TYPE msg;
		MSG_TYPE::InitMsg(msg, std::forward<ARGS>(args)...);
		SendSerialPacket(&msg);
	}

	int WriteLogMessage(const char* buffer, int length);

private:
    void SetRetransmitFlag(SerialProtocol::ReplyTokens::Value replyToken, bool value);

    void HandleProbeDevice(const SerialProtocol::ProbeDevice& packet);
    void HandleSetSystemTime(const SerialProtocol::SetSystemTime& packet);

    static SerialCommandHandler* s_Instance;

	mutable kernel::KMutex	m_SendMutex;

	int               m_SerialPort = -1;

    SerialProtocol::ProbeDeviceType m_DeviceType = SerialProtocol::ProbeDeviceType::Bootloader;

    std::map<SerialProtocol::Commands::Value, const PacketHandlerBase*> m_CommandHandlerMap;
    size_t                                                       m_LargestCommandPacket = 0;

    std::function<void(SerialProtocol::ReplyTokens::Value)> m_RetransmitHandlers[SerialProtocol::ReplyTokens::StaticTokenCount];
    uint32_t                                         m_RetransmitFlags;
    TimeValMicros                                    m_RetransmitTimes[SerialProtocol::ReplyTokens::StaticTokenCount];

	SerialCommandHandler(const SerialCommandHandler &other) = delete;
	SerialCommandHandler& operator=(const SerialCommandHandler &other) = delete;
};
