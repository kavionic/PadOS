// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __cplusplus
#include <System/MemoryRegions.h>

namespace kernel
{

inline constexpr size_t MEMORY_REGION_LIMIT = 64;

// Available after early BSS initialization, including from the board setup hook. The registry is immutable after startup.
const PMemoryRegionTable& kget_memory_regions();
const PMemoryRegionDefinition* kfind_memory_region(const char* name);
const PMemoryRegionDefinition* kfind_memory_region(uintptr_t address, size_t size);

} // namespace kernel

extern "C"
{
#endif

typedef enum KMemorySetupResult
{
    KMemorySetupResult_Success,
    KMemorySetupResult_InvalidRegion,
    KMemorySetupResult_InvalidInitialization,
    KMemorySetupResult_UnsupportedAttributes,
    KMemorySetupResult_TooManyRegions,
    KMemorySetupResult_UnsupportedAlignment
} KMemorySetupResult;

// The caller has disabled interrupts and made the bootstrap stack, ROM descriptors and early RAM accessible.
// The hook brings up external memory. No relocated code or data may be used before it returns.
KMemorySetupResult kinitialize_firmware_memory(void (*setupBoardMemory)(void));

#ifdef __cplusplus
} // extern "C"
#endif
