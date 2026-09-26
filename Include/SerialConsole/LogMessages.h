// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

struct RequestLogHistory : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::RequestLogHistory;

    static void InitMsg(RequestLogHistory& msg, int64_t lastTimestamp, uint32_t pageSize)
    {
        InitHeader(msg);
        msg.LastTimestamp = lastTimestamp;
        msg.PageSize      = pageSize;
    }

    int64_t  LastTimestamp;  // Send entries older than this timestamp. 0 = start from newest.
    uint32_t PageSize;       // Number of bytes of log data to read per page.
};

struct LogHistoryComplete : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::LogHistoryComplete;

    static void InitMsg(LogHistoryComplete& msg, bool hasMorePages)
    {
        InitHeader(msg, FLAG_NO_REPLY);
        msg.HasMorePages = hasMorePages ? 1 : 0;
        msg.Padding[0]   = 0;
        msg.Padding[1]   = 0;
        msg.Padding[2]   = 0;
    }

    uint8_t HasMorePages;   // Non-zero means there are more older log files available.
    uint8_t Padding[3];
};

} // namespace SerialProtocol
