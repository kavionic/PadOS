// This file is part of PadOS.
//
// Copyright (C) 2018-2022 Kurt Skauen <http://kavionic.com/>
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

#pragma once

namespace SerialProtocol
{

namespace Commands
{
    using Value = uint32_t;

    static constexpr uint32_t MessageReply      = 1;
    static constexpr uint32_t LogMessage        = 2;

    // Misc messages:
    static constexpr uint32_t SetSystemTime     = 3;
    static constexpr uint32_t RequestSystemTime = 4;

    static constexpr uint32_t SysCmdCount       = 100;
}


namespace ReplyTokens
{
    using Value = uint16_t;
    static constexpr uint16_t None = 0;
    static constexpr uint16_t StaticTokenCount = 32;
}


static constexpr bool IsStaticReplyToken(ReplyTokens::Value token) { return token != ReplyTokens::None && token <= ReplyTokens::StaticTokenCount; }

struct PacketHeader
{
    static const uint8_t MAGIC1 = 0x42;
    static const uint8_t MAGIC2 = 0x13;
    static const uint16_t MAGIC = MAGIC1 | (uint16_t(MAGIC2) << 8);

    template<typename T>
    static void InitHeader(T& msg, uint16_t replyToken = 0)
    {
        msg.Magic = MAGIC;
        msg.Token = replyToken;
        msg.Checksum = 0;
        msg.PackageLength = sizeof(msg);
        msg.Command = T::COMMAND;
    }

    uint16_t        Magic;
    uint16_t        Token;
    uint32_t        Checksum;
    uint32_t        PackageLength;
    Commands::Value Command;
};

struct MessageReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::MessageReply;

    static void InitMsg(MessageReply& msg, uint16_t token) { InitHeader(msg); msg.Token = token; }
};

struct LogMessage : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::LogMessage;

    static void InitMsg(LogMessage& msg) { InitHeader(msg); }
};

struct SetSystemTime : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::SetSystemTime;
    static void InitMsg(SetSystemTime& msg, int64_t unixTime) { InitHeader(msg); msg.UnixTime = unixTime; }

    int64_t UnixTime;
};

struct RequestSystemTime : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::RequestSystemTime;
    static void InitMsg(RequestSystemTime& msg) { InitHeader(msg); }
};

}
