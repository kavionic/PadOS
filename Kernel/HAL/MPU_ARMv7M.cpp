// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <bit>
#include <System/Platform.h>
#include <Kernel/HAL/MemoryProtection.h>
#include <Kernel/HAL/MPU_ARMv7M.h>

namespace kernel
{

#if defined(MPU_RASR_ENABLE_Msk)

static KMemorySetupResult encode_memory_attributes(const PMemoryRegionAttributes& attributes, uint32_t& encoded)
{
    uint32_t access;
    if (attributes.KernelAccess == PMemoryAccess::None && attributes.UserAccess == PMemoryAccess::None) {
        access = ARM_MPU_AP_NONE;
    } else if (attributes.KernelAccess == PMemoryAccess::ReadOnly && attributes.UserAccess == PMemoryAccess::None) {
        access = ARM_MPU_AP_PRO;
    } else if (attributes.KernelAccess == PMemoryAccess::ReadOnly && attributes.UserAccess == PMemoryAccess::ReadOnly) {
        access = ARM_MPU_AP_RO;
    } else if (attributes.KernelAccess == PMemoryAccess::ReadWrite && attributes.UserAccess == PMemoryAccess::None) {
        access = ARM_MPU_AP_PRIV;
    } else if (attributes.KernelAccess == PMemoryAccess::ReadWrite && attributes.UserAccess == PMemoryAccess::ReadOnly) {
        access = ARM_MPU_AP_URO;
    } else if (attributes.KernelAccess == PMemoryAccess::ReadWrite && attributes.UserAccess == PMemoryAccess::ReadWrite) {
        access = ARM_MPU_AP_FULL;
    } else {
        return KMemorySetupResult_UnsupportedAttributes;
    }

    if (attributes.Executable && attributes.KernelAccess == PMemoryAccess::None) {
        return KMemorySetupResult_UnsupportedAttributes;
    }

    uint32_t memoryAttributes;
    if (attributes.Type == PMemoryType::Normal)
    {
        uint32_t cachePolicy;
        switch (attributes.CachePolicy)
        {
            case PMemoryCachePolicy::Uncached:          cachePolicy = ARM_MPU_CACHEP_NOCACHE; break;
            case PMemoryCachePolicy::WriteThrough:      cachePolicy = ARM_MPU_CACHEP_WT_NWA; break;
            case PMemoryCachePolicy::WriteBack:         cachePolicy = ARM_MPU_CACHEP_WB_NWA; break;
            case PMemoryCachePolicy::WriteBackAllocate: cachePolicy = ARM_MPU_CACHEP_WB_WRA; break;
            default: return KMemorySetupResult_UnsupportedAttributes;
        }
#if defined(STM32H7)
        // STM32H7 has no hardware cache coherency; shareable normal memory is treated as non-cacheable.
        if (attributes.Shareable && attributes.CachePolicy != PMemoryCachePolicy::Uncached) {
            return KMemorySetupResult_UnsupportedAttributes;
        }
#endif
        memoryAttributes = ARM_MPU_ACCESS_NORMAL(ARM_MPU_CACHEP_NOCACHE, cachePolicy, attributes.Shareable);
    }
    else
    {
        if (attributes.CachePolicy != PMemoryCachePolicy::Uncached || attributes.Executable) {
            return KMemorySetupResult_UnsupportedAttributes;
        }
        if (attributes.Type == PMemoryType::Device) {
            memoryAttributes = ARM_MPU_ACCESS_DEVICE(attributes.Shareable);
        } else if (attributes.Type == PMemoryType::StronglyOrdered && attributes.Shareable) {
            memoryAttributes = ARM_MPU_ACCESS_ORDERED;
        } else {
            return KMemorySetupResult_UnsupportedAttributes;
        }
    }
    encoded = ARM_MPU_RASR_EX(!attributes.Executable, access, memoryAttributes, 0, 0);
    return KMemorySetupResult_Success;
}

static bool matches_default_memory_map(uint64_t address, uint64_t size, uint32_t attributes)
{
#if defined(__CORTEX_M) && (__CORTEX_M == 7U)
    struct KDefaultMemoryRegion
    {
        uint64_t End;
        uint32_t MemoryAttributes;
        bool Executable;
    };

    // Cortex-M7 Devices Generic User Guide (DUI 0646C), Tables 2-11 and 2-12.
    // Include both cache levels: the explicit Normal-memory encoder keeps the outer cache disabled.
    static constexpr KDefaultMemoryRegion DEFAULT_MEMORY_REGIONS[] =
    {
        {0x20000000, ARM_MPU_ACCESS_NORMAL(ARM_MPU_CACHEP_WT_NWA, ARM_MPU_CACHEP_WT_NWA, 0), true},
        {0x40000000, ARM_MPU_ACCESS_NORMAL(ARM_MPU_CACHEP_WB_WRA, ARM_MPU_CACHEP_WB_WRA, 0), true},
        {0x60000000, ARM_MPU_ACCESS_DEVICE(0), false},
        {0x80000000, ARM_MPU_ACCESS_NORMAL(ARM_MPU_CACHEP_WB_WRA, ARM_MPU_CACHEP_WB_WRA, 0), true},
        {0xa0000000, ARM_MPU_ACCESS_NORMAL(ARM_MPU_CACHEP_WT_NWA, ARM_MPU_CACHEP_WT_NWA, 0), true},
        {0xc0000000, ARM_MPU_ACCESS_DEVICE(1), false},
        {0xe0000000, ARM_MPU_ACCESS_DEVICE(0), false},
        {0xe0100000, ARM_MPU_ACCESS_ORDERED, false},
        {0x100000000ULL, ARM_MPU_ACCESS_DEVICE(0), false}
    };

    const uint64_t end = address + size;
    for (const KDefaultMemoryRegion& region : DEFAULT_MEMORY_REGIONS)
    {
        if (address < region.End)
        {
            const uint32_t defaultAttributes = ARM_MPU_RASR_EX(
                !region.Executable,
                ARM_MPU_AP_PRIV,
                region.MemoryAttributes,
                0,
                0);
            if (attributes != defaultAttributes) {
                return false;
            }
            if (end <= region.End) {
                return true;
            }
            address = region.End;
        }
    }
#endif
    return false;
}

static bool encode_memory_range(uint64_t address, uint64_t size, uint32_t attributes, KARMv7MMPURegion& output)
{
    for (uint8_t exponent = 5; exponent <= 32; ++exponent)
    {
        const uint64_t regionSize = uint64_t(1) << exponent;
        const uint64_t regionAddress = address & ~(regionSize - 1);
        if (address + size > regionAddress + regionSize) {
            continue;
        }
        uint8_t disabledSubregions = 0;
        if (exponent < 8)
        {
            if (address != regionAddress || size != regionSize) {
                continue;
            }
        }
        else
        {
            const uint64_t subregionSize = regionSize / 8;
            if ((address & (subregionSize - 1)) != 0 || (size & (subregionSize - 1)) != 0) {
                continue;
            }
            const uint32_t firstSubregion = uint32_t((address - regionAddress) / subregionSize);
            const uint32_t subregionCount = uint32_t(size / subregionSize);
            disabledSubregions = uint8_t(~(((1U << subregionCount) - 1) << firstSubregion));
        }
        output = {uint32_t(regionAddress), attributes, exponent, disabledSubregions};
        return true;
    }
    return false;
}

static uint64_t get_covered_memory_size(const KARMv7MMPURegion& region, uint64_t address, uint64_t end)
{
    const uint64_t regionSize = uint64_t(1) << region.SizeExponent;
    const size_t subregionCount = (region.SizeExponent >= 8) ? 8 : 1;
    const uint64_t subregionSize = regionSize / subregionCount;
    uint64_t coveredSize = 0;
    for (size_t i = 0; i < subregionCount; ++i)
    {
        if ((region.DisabledSubregions & (1U << i)) == 0)
        {
            const uint64_t subregionStart = region.Address + i * subregionSize;
            const uint64_t subregionEnd = subregionStart + subregionSize;
            const uint64_t start = (address > subregionStart) ? address : subregionStart;
            const uint64_t limit = (end < subregionEnd) ? end : subregionEnd;
            if (start < limit) {
                coveredSize += limit - start;
            }
        }
    }
    return coveredSize;
}

static bool merge_mpu_regions(const KARMv7MMPURegion& lhs, const KARMv7MMPURegion& rhs, KARMv7MMPURegion& output)
{
    if (lhs.Attributes != rhs.Attributes) {
        return false;
    }
    const uint64_t firstAddress = (lhs.Address < rhs.Address) ? lhs.Address : rhs.Address;
    const uint64_t lhsEnd = lhs.Address + (uint64_t(1) << lhs.SizeExponent);
    const uint64_t rhsEnd = rhs.Address + (uint64_t(1) << rhs.SizeExponent);
    const uint64_t lastAddress = (lhsEnd > rhsEnd) ? lhsEnd : rhsEnd;
    const uint8_t firstExponent = (lhs.SizeExponent > rhs.SizeExponent) ? lhs.SizeExponent : rhs.SizeExponent;
    for (uint8_t exponent = firstExponent; exponent <= 32; ++exponent)
    {
        const uint64_t regionSize = uint64_t(1) << exponent;
        const uint64_t regionAddress = firstAddress & ~(regionSize - 1);
        if (lastAddress > regionAddress + regionSize) {
            continue;
        }
        const size_t subregionCount = (exponent >= 8) ? 8 : 1;
        const uint64_t subregionSize = regionSize / subregionCount;
        uint8_t disabledSubregions = 0;
        bool representable = true;
        for (size_t i = 0; i < subregionCount; ++i)
        {
            const uint64_t address = regionAddress + i * subregionSize;
            const uint64_t end = address + subregionSize;
            const uint64_t coverage = get_covered_memory_size(lhs, address, end) + get_covered_memory_size(rhs, address, end);
            if (coverage == 0) {
                disabledSubregions |= uint8_t(1U << i);
            } else if (coverage != subregionSize) {
                representable = false;
            }
        }
        if (representable)
        {
            output = {uint32_t(regionAddress), lhs.Attributes, exponent, disabledSubregions};
            return true;
        }
    }
    return false;
}

static void compact_mpu_regions(std::span<KARMv7MMPURegion> regions, size_t& regionCount)
{
    bool merged;
    do
    {
        merged = false;
        for (size_t i = 0; i < regionCount && !merged; ++i)
        {
            for (size_t j = i + 1; j < regionCount; ++j)
            {
                KARMv7MMPURegion combined;
                if (merge_mpu_regions(regions[i], regions[j], combined))
                {
                    regions[i] = combined;
                    for (size_t k = j + 1; k < regionCount; ++k) {
                        regions[k - 1] = regions[k];
                    }
                    --regionCount;
                    merged = true;
                    break;
                }
            }
        }
    } while (merged);
}

KMemorySetupResult kbuild_armv7m_mpu_regions(
    const PMemoryRegionTable& table,
    std::span<KARMv7MMPURegion> output,
    size_t& regionCount)
{
    regionCount = 0;
    if (table.Regions == nullptr || table.Count == 0 || table.Count > MEMORY_REGION_LIMIT) {
        return KMemorySetupResult_InvalidRegion;
    }
    std::array<KARMv7MMPURegion, MEMORY_REGION_LIMIT> regions;
    size_t count = 0;
    uint64_t previousEnd = 0;
    for (size_t i = 0; i < table.Count; ++i)
    {
        const PMemoryRegionDefinition& region = table.Regions[i];
        const uint64_t end = uint64_t(region.Address) + region.Size;
        if (region.Name == nullptr || region.Size == 0 || region.Address < previousEnd || end > (uint64_t(1) << 32)) {
            return KMemorySetupResult_InvalidRegion;
        }
        if ((region.Address & 31) != 0 || (region.Size & 31) != 0) {
            return KMemorySetupResult_UnsupportedAlignment;
        }
        uint32_t attributes;
        const KMemorySetupResult result = encode_memory_attributes(region.Attributes, attributes);
        if (result != KMemorySetupResult_Success) {
            return result;
        }
        previousEnd = end;
        if (table.UsePrivilegedDefaultMap && matches_default_memory_map(region.Address, region.Size, attributes)) {
            continue;
        }
        uint64_t address = region.Address;
        uint64_t remainingSize = region.Size;
        while (remainingSize != 0)
        {
            if (count == regions.size())
            {
                compact_mpu_regions(regions, count);
                if (count == regions.size()) {
                    return KMemorySetupResult_TooManyRegions;
                }
            }
            uint64_t size = remainingSize;
            if (!encode_memory_range(address, size, attributes, regions[count]))
            {
                size = std::bit_floor(remainingSize);
                while ((address & (size - 1)) != 0) {
                    size /= 2;
                }
                encode_memory_range(address, size, attributes, regions[count]);
            }
            ++count;
            address += size;
            remainingSize -= size;
        }
    }
    compact_mpu_regions(regions, count);
    if (count > output.size()) {
        return KMemorySetupResult_TooManyRegions;
    }
    for (size_t i = 0; i < count; ++i) {
        output[i] = regions[i];
    }
    regionCount = count;
    return KMemorySetupResult_Success;
}

KMemorySetupResult kconfigure_memory_protection(const PMemoryRegionTable& table, bool apply)
{
    std::array<KARMv7MMPURegion, 16> regions;
    const size_t hardwareRegionCount = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    const size_t regionCapacity = (hardwareRegionCount < regions.size()) ? hardwareRegionCount : regions.size();
    size_t regionCount;
    const KMemorySetupResult result = kbuild_armv7m_mpu_regions(
        table,
        std::span(regions).first(regionCapacity),
        regionCount);
    if (result == KMemorySetupResult_Success && apply)
    {
        // Publish copied instructions and finish writes made using the previous memory attributes before replacing the map.
#if defined(__DCACHE_PRESENT) && __DCACHE_PRESENT == 1U
        SCB_CleanInvalidateDCache();
#endif
#if defined(__ICACHE_PRESENT) && __ICACHE_PRESENT == 1U
        SCB_InvalidateICache();
#endif
        __DSB();
        ARM_MPU_Disable();
        for (size_t i = 0; i < hardwareRegionCount; ++i) {
            ARM_MPU_ClrRegion(uint32_t(i));
        }
        for (size_t i = 0; i < regionCount; ++i)
        {
            const KARMv7MMPURegion& region = regions[i];
            const uint32_t attributes = region.Attributes | (uint32_t(region.SizeExponent - 1) << MPU_RASR_SIZE_Pos)
                | (uint32_t(region.DisabledSubregions) << MPU_RASR_SRD_Pos);
            ARM_MPU_SetRegion(ARM_MPU_RBAR(uint32_t(i), region.Address), attributes);
        }
        ARM_MPU_Enable(MPU_CTRL_HFNMIENA_Msk | (table.UsePrivilegedDefaultMap ? MPU_CTRL_PRIVDEFENA_Msk : 0));
#if defined(__DCACHE_PRESENT) && __DCACHE_PRESENT == 1U
        SCB_EnableDCache();
#endif
#if defined(__ICACHE_PRESENT) && __ICACHE_PRESENT == 1U
        SCB_EnableICache();
#endif
        __DSB();
        __ISB();
    }
    return result;
}

#else

KMemorySetupResult kbuild_armv7m_mpu_regions(
    const PMemoryRegionTable& table,
    std::span<KARMv7MMPURegion> output,
    size_t& regionCount)
{
    regionCount = 0;
    return KMemorySetupResult_UnsupportedAttributes;
}

KMemorySetupResult kconfigure_memory_protection(const PMemoryRegionTable& table, bool apply)
{
    return KMemorySetupResult_UnsupportedAttributes;
}

#endif

} // namespace kernel
