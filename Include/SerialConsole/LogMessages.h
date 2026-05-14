// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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

#include <stdint.h>
#include <SerialConsole/SerialProtocol.h>

namespace SerialProtocol
{

struct LogMessage : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::LogMessage;

    static void InitMsg(LogMessage& msg, int64_t timestamp, uint32_t categoryHash, uint8_t severity)
    {
        InitHeader(msg, FLAG_NO_REPLY);
        msg.Timestamp    = timestamp;
        msg.CategoryHash = categoryHash;
        msg.Severity     = severity;
        msg.Padding[0]   = 0;
        msg.Padding[1]   = 0;
        msg.Padding[2]   = 0;
    }

    int64_t     Timestamp;      // UTC nanoseconds
    uint32_t    CategoryHash;
    uint8_t     Severity;       // uint8_t cast of PLogSeverity
    uint8_t     Padding[3];
    // Variable-length raw message text follows
};

struct RequestLogCategories : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::RequestLogCategories;
    static void InitMsg(RequestLogCategories& msg) { InitHeader(msg); }
};

struct LogCategoryEntry
{
    uint32_t    CategoryHash;
    uint8_t     MinSeverity;    // uint8_t cast of PLogSeverity
    uint8_t     Padding[3];
    char        CategoryName[64];
    char        DisplayName[64];
};

struct LogCategoriesReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::LogCategoriesReply;
    static void InitMsg(LogCategoriesReply& msg, uint32_t categoryCount)
    {
        InitHeader(msg, FLAG_NO_REPLY);
        msg.CategoryCount = categoryCount;
    }

    uint32_t CategoryCount;
    // Array of LogCategoryEntry follows
};

struct RequestLogSeverities : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::RequestLogSeverities;
    static void InitMsg(RequestLogSeverities& msg) { InitHeader(msg); }
};

struct LogSeverityEntry
{
    uint8_t     SeverityID;     // uint8_t cast of PLogSeverity
    uint8_t     Padding[3];
    char        Name[32];
};

struct LogSeveritiesReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::LogSeveritiesReply;
    static void InitMsg(LogSeveritiesReply& msg, uint32_t severityCount)
    {
        InitHeader(msg, FLAG_NO_REPLY);
        msg.SeverityCount = severityCount;
    }

    uint32_t SeverityCount;
    // Array of LogSeverityEntry follows
};

} // namespace SerialProtocol
