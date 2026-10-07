// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

enum class PMemoryAccess : uint8_t
{
    None,
    ReadOnly,
    ReadWrite
};

enum class PMemoryType : uint8_t
{
    Normal,
    Device,
    StronglyOrdered
};

enum class PMemoryCachePolicy : uint8_t
{
    Uncached,
    WriteThrough,
    WriteBack,
    WriteBackAllocate
};

enum class PMemoryRegionPurpose : uint8_t
{
    Reserved,
    Code,
    Data,
    Stack,
    Heap,
    ImageStorage,
    Device,
    Retained
};

struct PMemoryRegionAttributes
{
    PMemoryAccess KernelAccess;
    PMemoryAccess UserAccess;
    PMemoryType Type;
    PMemoryCachePolicy CachePolicy;
    bool Executable;
    bool Shareable;
};

struct PMemoryRegionDefinition
{
    const char* Name;
    uintptr_t Address;
    size_t Size;
    PMemoryRegionPurpose Purpose;
    PMemoryRegionAttributes Attributes;
};

struct PMemoryRegionTable
{
    // Regions must be sorted by address and may not overlap. The table and names must reside in bootstrap-readable ROM.
    const PMemoryRegionDefinition* Regions;
    size_t Count;
    // Unlisted addresses use the architecture's default map for privileged access only.
    bool UsePrivilegedDefaultMap;
};

enum class PMemoryInitializationOperation : uint8_t
{
    Copy,
    Zero
};

enum class PMemoryInitializationPhase : uint8_t
{
    BeforeBoardSetup,
    AfterBoardSetup
};

struct PMemoryInitialization
{
    void* Destination;
    const void* Source;
    size_t Size;
    PMemoryInitializationOperation Operation;
    PMemoryInitializationPhase Phase;
};

struct PMemoryInitializationTable
{
    const PMemoryInitialization* Operations;
    size_t Count;
};

// Each linked image supplies its own table. This symbol is resolved independently in the kernel and application images.
extern const PMemoryInitializationTable FIRMWARE_MEMORY_INITIALIZATION;

const PMemoryRegionDefinition* p_find_memory_region(const PMemoryRegionTable& table, const char* name);
const PMemoryRegionDefinition* p_find_memory_region(const PMemoryRegionTable& table, uintptr_t address, size_t size);
