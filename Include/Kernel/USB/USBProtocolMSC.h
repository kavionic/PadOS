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

#include <System/Platform.h>
#include <Kernel/USB/USBProtocol.h>

enum class USB_MSC_SubclassCode : uint8_t
{
    SCSI_TRANSPARENT_COMMAND_SET = 0x06
};

enum class USB_MSC_ProtocolCode : uint8_t
{
    BULK_ONLY_TRANSPORT = 0x50
};

enum class USB_MSC_Request : uint8_t
{
    GET_MAX_LUN     = 0xfe,
    BULK_ONLY_RESET = 0xff
};

enum class USB_MSC_DataDirection : uint8_t
{
    DATA_OUT = 0x00,
    DATA_IN  = 0x80
};

enum class USB_MSC_CommandStatus : uint8_t
{
    COMMAND_PASSED = 0x00,
    COMMAND_FAILED = 0x01,
    PHASE_ERROR    = 0x02
};

enum class USB_MSC_SCSI_OperationCode : uint8_t
{
    TEST_UNIT_READY      = 0x00,
    REQUEST_SENSE        = 0x03,
    INQUIRY              = 0x12,
    READ_CAPACITY_10     = 0x25,
    READ_10              = 0x28,
    WRITE_10             = 0x2a,
    SYNCHRONIZE_CACHE_10 = 0x35,
    SERVICE_ACTION_IN_16 = 0x9e
};

enum class USB_MSC_SCSI_ServiceAction : uint8_t
{
    READ_CAPACITY_16 = 0x10
};

enum class USB_MSC_SCSI_SenseKey : uint8_t
{
    NO_SENSE        = 0x00,
    RECOVERED_ERROR = 0x01,
    NOT_READY       = 0x02,
    MEDIUM_ERROR    = 0x03,
    HARDWARE_ERROR  = 0x04,
    ILLEGAL_REQUEST = 0x05,
    UNIT_ATTENTION  = 0x06,
    DATA_PROTECT    = 0x07,
    BLANK_CHECK     = 0x08,
    VENDOR_SPECIFIC = 0x09,
    COPY_ABORTED    = 0x0a,
    ABORTED_COMMAND = 0x0b,
    EQUAL           = 0x0c,
    VOLUME_OVERFLOW = 0x0d,
    MISCOMPARE      = 0x0e,
    COMPLETED       = 0x0f
};

enum class USB_MSC_SCSI_SenseResponseCode : uint8_t
{
    CURRENT_ERRORS  = 0x70,
    DEFERRED_ERRORS = 0x71
};

enum class USB_MSC_SCSI_AdditionalSenseCode : uint8_t
{
    LOGICAL_UNIT_NOT_READY = 0x04,
    MEDIUM_NOT_PRESENT     = 0x3a
};

struct USB_MSC_CommandBlockWrapper
{
    static constexpr uint32_t SIGNATURE = 0x43425355;
    static constexpr uint8_t LOGICAL_UNIT_NUMBER_MASK = 0x0f;
    static constexpr uint8_t COMMAND_BLOCK_LENGTH_MASK = 0x1f;
    static constexpr uint8_t MAX_COMMAND_BLOCK_LENGTH = 16;

    uint32_t                 Signature = SIGNATURE; // Little-endian on the wire.
    uint32_t                 Tag = 0; // Little-endian on the wire.
    uint32_t                 DataTransferLength = 0; // Little-endian on the wire.
    USB_MSC_DataDirection    Flags = USB_MSC_DataDirection::DATA_OUT;
    uint8_t                  LogicalUnitNumber = 0;
    uint8_t                  CommandBlockLength = 0;
    uint8_t                  CommandBlock[MAX_COMMAND_BLOCK_LENGTH] = {};
} ATTR_PACKED;

static_assert(sizeof(USB_MSC_CommandBlockWrapper) == 31);

struct USB_MSC_CommandStatusWrapper
{
    static constexpr uint32_t SIGNATURE = 0x53425355;

    uint32_t                Signature = SIGNATURE; // Little-endian on the wire.
    uint32_t                Tag = 0; // Little-endian on the wire.
    uint32_t                DataResidue = 0; // Little-endian on the wire.
    USB_MSC_CommandStatus   Status = USB_MSC_CommandStatus::COMMAND_PASSED;
} ATTR_PACKED;

static_assert(sizeof(USB_MSC_CommandStatusWrapper) == 13);
