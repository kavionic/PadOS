// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 30.04.2022 23:30

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <SerialConsole/SerialProtocol.h>

namespace SerialProtocol
{

namespace Commands
{
    // Common messages:

    // Bootloader message:
    static constexpr uint32_t BeginFirmwareUpdate       = SysCmdCount + 10;
    static constexpr uint32_t BeginFirmwareUpdateReply  = SysCmdCount + 20;
    static constexpr uint32_t EndFirmwareUpdate         = SysCmdCount + 30;
    static constexpr uint32_t EraseFlashSection         = SysCmdCount + 40;
    static constexpr uint32_t EraseFlashSectionProgress = SysCmdCount + 50;
    static constexpr uint32_t WriteFlashSection         = SysCmdCount + 60;
    static constexpr uint32_t WriteFlashSectionReply    = SysCmdCount + 70;
    static constexpr uint32_t GetFlashChecksum          = SysCmdCount + 80;
    static constexpr uint32_t GetFlashChecksumReply     = SysCmdCount + 90;

    static constexpr uint32_t LastBLCmd = SysCmdCount + 1000;

    // System message:
    static constexpr uint32_t InitiateFirmwareUpdate = LastBLCmd + 9;

}

struct InitiateFirmwareUpdate : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::InitiateFirmwareUpdate;
    static void InitMsg(InitiateFirmwareUpdate& msg) { InitHeader(msg, FLAG_NO_REPLY); }
};

struct BeginFirmwareUpdate : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::BeginFirmwareUpdate;
    static void InitMsg(BeginFirmwareUpdate& msg) { InitHeader(msg); }
};

struct BeginFirmwareUpdateReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::BeginFirmwareUpdateReply;
    static void InitMsg(BeginFirmwareUpdateReply& msg) { InitHeader(msg); }
};

struct EndFirmwareUpdate : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::EndFirmwareUpdate;
    static void InitMsg(EndFirmwareUpdate& msg) { InitHeader(msg); }
};

struct EraseFlashSection : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::EraseFlashSection;
    static void InitMsg(EraseFlashSection& msg, uint32_t startAddress, uint32_t length) { InitHeader(msg); msg.StartAddress = startAddress; msg.Length = length; }

    uint32_t    StartAddress = 0;
    uint32_t    Length = 0;
};

struct EraseFlashSectionProgress : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::EraseFlashSectionProgress;
    static void InitMsg(EraseFlashSectionProgress& msg, uint32_t bytesErased) { InitHeader(msg); msg.BytesErased = bytesErased; }

    uint32_t BytesErased = 0;
};

struct WriteFlashSection : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::WriteFlashSection;
    static constexpr uint32_t SegmentSize = 1024*16;

    static void InitMsg(WriteFlashSection& msg, const void* data, uint32_t startAddress, uint32_t length)
    {
        InitHeader(msg);
        msg.StartAddress = startAddress;
        msg.Length = length;
        assert(length <= SegmentSize);
        if (length <= SegmentSize)
        {
            memcpy(msg.Data, data, length);
        }
    }

    uint32_t    StartAddress = 0;
    uint32_t    Length = 0;
    uint8_t     Data[SegmentSize];
};

struct WriteFlashSectionReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::WriteFlashSectionReply;
    static void InitMsg(WriteFlashSectionReply& msg, uint32_t startAddress, uint32_t length) { InitHeader(msg); msg.StartAddress = startAddress; msg.Length = length; }

    uint32_t    StartAddress = 0;
    uint32_t    Length = 0;
};

struct GetFlashChecksum : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::GetFlashChecksum;
    static void InitMsg(GetFlashChecksum& msg, uint32_t startAddress, uint32_t length) { InitHeader(msg); msg.StartAddress = startAddress; msg.Length = length; }

    uint32_t    StartAddress = 0;
    uint32_t    Length = 0;
};

struct GetFlashChecksumReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::GetFlashChecksumReply;
    static void InitMsg(GetFlashChecksumReply& msg, uint32_t crc32) { InitHeader(msg); msg.CRC32 = crc32; }

    uint32_t CRC32;
};

} // namespace SerialProtocol
