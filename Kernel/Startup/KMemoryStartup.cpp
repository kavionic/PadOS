// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <span>
#include <System/AppDefinition.h>
#include <Kernel/HAL/MemoryProtection.h>
#include <Kernel/Startup/KMemoryStartup.h>

namespace kernel
{

static const PMemoryRegionTable* gk_MemoryRegions;

const PMemoryRegionTable& kget_memory_regions()
{
    return *gk_MemoryRegions;
}

const PMemoryRegionDefinition* kfind_memory_region(const char* name)
{
    return p_find_memory_region(kget_memory_regions(), name);
}

const PMemoryRegionDefinition* kfind_memory_region(uintptr_t address, size_t size)
{
    return p_find_memory_region(kget_memory_regions(), address, size);
}

static bool initialization_ranges_overlap(uintptr_t lhsAddress, size_t lhsSize, uintptr_t rhsAddress, size_t rhsSize)
{
    return lhsSize != 0 && rhsSize != 0 && uint64_t(lhsAddress) < uint64_t(rhsAddress) + rhsSize
        && uint64_t(rhsAddress) < uint64_t(lhsAddress) + lhsSize;
}

static KMemorySetupResult validate_memory_initialization(
    const PMemoryRegionTable& regions,
    std::span<const PMemoryInitializationTable* const> tables)
{
    for (const PMemoryInitializationTable* table : tables)
    {
        if (table == nullptr || (table->Count != 0 && table->Operations == nullptr)) {
            return KMemorySetupResult_InvalidInitialization;
        }
    }
    for (const PMemoryInitializationTable* table : tables)
    {
        for (size_t i = 0; i < table->Count; ++i)
        {
            const PMemoryInitialization& operation = table->Operations[i];
            if (operation.Size == 0) {
                continue;
            }
            const uintptr_t destination = uintptr_t(operation.Destination);
            const PMemoryRegionDefinition* region = p_find_memory_region(regions, destination, operation.Size);
            if (region == nullptr || region->Attributes.Type != PMemoryType::Normal
                || region->Attributes.KernelAccess == PMemoryAccess::None) {
                return KMemorySetupResult_InvalidInitialization;
            }
            if (operation.Phase != PMemoryInitializationPhase::BeforeBoardSetup
                && operation.Phase != PMemoryInitializationPhase::AfterBoardSetup) {
                return KMemorySetupResult_InvalidInitialization;
            }
            if (operation.Operation == PMemoryInitializationOperation::Copy)
            {
                const PMemoryRegionDefinition* sourceRegion = p_find_memory_region(
                    regions,
                    uintptr_t(operation.Source),
                    operation.Size);
                if (sourceRegion == nullptr || sourceRegion->Attributes.Type != PMemoryType::Normal
                    || sourceRegion->Attributes.KernelAccess == PMemoryAccess::None) {
                    return KMemorySetupResult_InvalidInitialization;
                }
            }
            else if (operation.Operation != PMemoryInitializationOperation::Zero || operation.Source != nullptr)
            {
                return KMemorySetupResult_InvalidInitialization;
            }
            for (const PMemoryInitializationTable* otherTable : tables)
            {
                for (size_t j = 0; j < otherTable->Count; ++j)
                {
                    const PMemoryInitialization& other = otherTable->Operations[j];
                    if (&operation != &other && initialization_ranges_overlap(
                        destination,
                        operation.Size,
                        uintptr_t(other.Destination),
                        other.Size)) {
                        return KMemorySetupResult_InvalidInitialization;
                    }
                    // Copy sources must survive every initialization operation, independently of operation ordering.
                    if (operation.Operation == PMemoryInitializationOperation::Copy && operation.Source != operation.Destination
                        && initialization_ranges_overlap(
                            uintptr_t(operation.Source),
                            operation.Size,
                            uintptr_t(other.Destination),
                            other.Size)) {
                        return KMemorySetupResult_InvalidInitialization;
                    }
                }
            }
        }
    }
    return KMemorySetupResult_Success;
}

static void initialize_memory_phase(
    std::span<const PMemoryInitializationTable* const> tables,
    PMemoryInitializationPhase phase)
{
    for (const PMemoryInitializationTable* table : tables)
    {
        for (size_t i = 0; i < table->Count; ++i)
        {
            const PMemoryInitialization& operation = table->Operations[i];
            if (operation.Phase != phase || operation.Size == 0
                || (operation.Operation == PMemoryInitializationOperation::Copy && operation.Source == operation.Destination)) {
                continue;
            }
            // Volatile accesses keep bootstrap loops independent of libc and support arbitrary section lengths.
            volatile uint8_t* destination = static_cast<volatile uint8_t*>(operation.Destination);
            if (operation.Operation == PMemoryInitializationOperation::Copy)
            {
                const uint8_t* source = static_cast<const uint8_t*>(operation.Source);
                for (size_t j = 0; j < operation.Size; ++j) {
                    destination[j] = source[j];
                }
            }
            else
            {
                for (size_t j = 0; j < operation.Size; ++j) {
                    destination[j] = 0;
                }
            }
        }
    }
}

static void prepare_initialization_permissions(
    PMemoryRegionTable& regions,
    std::span<PMemoryRegionDefinition> writableRegions,
    std::span<const PMemoryInitializationTable* const> tables)
{
    for (const PMemoryInitializationTable* table : tables)
    {
        for (size_t i = 0; i < table->Count; ++i)
        {
            const PMemoryInitialization& operation = table->Operations[i];
            if (operation.Size == 0
                || (operation.Operation == PMemoryInitializationOperation::Copy && operation.Source == operation.Destination)) {
                continue;
            }
            const PMemoryRegionDefinition* region = p_find_memory_region(
                regions,
                uintptr_t(operation.Destination),
                operation.Size);
            PMemoryRegionAttributes& attributes = writableRegions[size_t(region - regions.Regions)].Attributes;
            attributes.KernelAccess = PMemoryAccess::ReadWrite;
            attributes.UserAccess = PMemoryAccess::None;
            attributes.Executable = false;
        }
    }
}

} // namespace kernel

KMemorySetupResult kinitialize_firmware_memory(void (*setupBoardMemory)(void))
{
    using namespace kernel;

    const PMemoryRegionTable* regions = __app_definition.MemoryRegions;
    if (regions == nullptr || setupBoardMemory == nullptr) {
        return KMemorySetupResult_InvalidRegion;
    }
    KMemorySetupResult result = kconfigure_memory_protection(*regions, false);
    if (result != KMemorySetupResult_Success) {
        return result;
    }

    const PMemoryInitializationTable* const tables[] =
    {
        __kernel_definition.MemoryInitialization,
#ifdef PADOS_MODULE_USER_SPACE
        __app_definition.MemoryInitialization
#endif
    };
    result = validate_memory_initialization(*regions, tables);
    if (result != KMemorySetupResult_Success) {
        return result;
    }

    std::array<PMemoryRegionDefinition, MEMORY_REGION_LIMIT> writableRegions;
    for (size_t i = 0; i < regions->Count; ++i) {
        writableRegions[i] = regions->Regions[i];
    }
    PMemoryRegionTable temporaryRegions = {writableRegions.data(), regions->Count, regions->UsePrivilegedDefaultMap};
    prepare_initialization_permissions(temporaryRegions, writableRegions, tables);
    result = kconfigure_memory_protection(temporaryRegions, true);
    if (result != KMemorySetupResult_Success) {
        return result;
    }

    initialize_memory_phase(tables, PMemoryInitializationPhase::BeforeBoardSetup);
    gk_MemoryRegions = regions;
    setupBoardMemory();
    initialize_memory_phase(tables, PMemoryInitializationPhase::AfterBoardSetup);

    // This also synchronizes the data and instruction caches before any relocated function or application callback runs.
    return kconfigure_memory_protection(*regions, true);
}
