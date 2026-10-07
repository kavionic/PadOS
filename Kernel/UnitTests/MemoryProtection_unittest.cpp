// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <gtest/gtest.h>
#include <System/AppDefinition.h>
#include <System/Platform.h>
#include <Kernel/HAL/MPU_ARMv7M.h>

#if defined(MPU_RASR_ENABLE_Msk)

namespace kernel
{

static constexpr PMemoryRegionAttributes TEST_READ_ONLY_ATTRIBUTES =
{
    PMemoryAccess::ReadOnly, PMemoryAccess::ReadOnly, PMemoryType::Normal, PMemoryCachePolicy::WriteBack, true, false
};

static bool mpu_region_contains_address(const KARMv7MMPURegion& region, uint64_t address)
{
    const uint64_t size = uint64_t(1) << region.SizeExponent;
    if (address < region.Address || address >= region.Address + size) {
        return false;
    }
    if (region.SizeExponent >= 8) {
        return (region.DisabledSubregions & (1U << ((address - region.Address) / (size / 8)))) == 0;
    }
    return true;
}

TEST(MemoryProtection, CombinesDisjointCodeWithoutExposingTheGap)
{
    const PMemoryRegionDefinition regions[] =
    {
        {"first", 0x30000000, 64 * 1024, PMemoryRegionPurpose::Code, TEST_READ_ONLY_ATTRIBUTES},
        {"second", 0x30020000, 64 * 1024, PMemoryRegionPurpose::Code, TEST_READ_ONLY_ATTRIBUTES}
    };
    std::array<KARMv7MMPURegion, 1> output;
    size_t count;
    ASSERT_EQ(kbuild_armv7m_mpu_regions({regions, std::size(regions), false}, output, count), KMemorySetupResult_Success);
    ASSERT_EQ(count, 1U);
    EXPECT_TRUE(mpu_region_contains_address(output[0], 0x30000000));
    EXPECT_TRUE(mpu_region_contains_address(output[0], 0x3000ffff));
    EXPECT_FALSE(mpu_region_contains_address(output[0], 0x30010000));
    EXPECT_FALSE(mpu_region_contains_address(output[0], 0x3001ffff));
    EXPECT_TRUE(mpu_region_contains_address(output[0], 0x30020000));
    EXPECT_FALSE(mpu_region_contains_address(output[0], 0x30030000));
}

TEST(MemoryProtection, CombinesUnalignedFlashPartitionsWithoutIncludingReservedFlash)
{
    const PMemoryRegionDefinition regions[] =
    {
        {"kernel", 0x90800000, 7 * 1024 * 1024, PMemoryRegionPurpose::ImageStorage, TEST_READ_ONLY_ATTRIBUTES},
        {"application", 0x90f00000, 49 * 1024 * 1024, PMemoryRegionPurpose::ImageStorage, TEST_READ_ONLY_ATTRIBUTES}
    };
    std::array<KARMv7MMPURegion, 1> output;
    size_t count;
    ASSERT_EQ(kbuild_armv7m_mpu_regions({regions, std::size(regions), false}, output, count), KMemorySetupResult_Success);
    ASSERT_EQ(count, 1U);
    EXPECT_FALSE(mpu_region_contains_address(output[0], 0x90000000));
    EXPECT_FALSE(mpu_region_contains_address(output[0], 0x907fffff));
    EXPECT_TRUE(mpu_region_contains_address(output[0], 0x90800000));
    EXPECT_TRUE(mpu_region_contains_address(output[0], 0x93ffffff));
    EXPECT_FALSE(mpu_region_contains_address(output[0], 0x94000000));
}

TEST(MemoryProtection, RejectsOverlapMisalignmentAndInsufficientSlots)
{
    PMemoryRegionDefinition regions[] =
    {
        {"first", 0x30000000, 64, PMemoryRegionPurpose::Code, TEST_READ_ONLY_ATTRIBUTES},
        {"second", 0x30000020, 64, PMemoryRegionPurpose::Code, TEST_READ_ONLY_ATTRIBUTES}
    };
    std::array<KARMv7MMPURegion, 16> output;
    size_t count;
    EXPECT_EQ(kbuild_armv7m_mpu_regions({regions, std::size(regions), false}, output, count), KMemorySetupResult_InvalidRegion);
    regions[1].Address = 0x30000101;
    EXPECT_EQ(
        kbuild_armv7m_mpu_regions({regions, std::size(regions), false}, output, count),
        KMemorySetupResult_UnsupportedAlignment);
    regions[1].Address = 0x30000100;
    regions[1].Attributes.KernelAccess = PMemoryAccess::ReadWrite;
    EXPECT_EQ(
        kbuild_armv7m_mpu_regions({regions, std::size(regions), false}, std::span(output).first(1), count),
        KMemorySetupResult_TooManyRegions);
    regions[1].Attributes.Type = PMemoryType::Device;
    EXPECT_EQ(
        kbuild_armv7m_mpu_regions({regions, std::size(regions), false}, output, count),
        KMemorySetupResult_UnsupportedAttributes);
}

#if defined(__CORTEX_M) && (__CORTEX_M == 7U)

TEST(MemoryProtection, OmitsDefaultRegionsOnlyWhenTheBackgroundMapIsEnabled)
{
    const PMemoryRegionDefinition regions[] =
    {
        {"peripheral", 0x40000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}},
        {"external.shared", 0xa0000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, true}},
        {"external.private", 0xc0000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}},
        {"ppb", 0xe0000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::StronglyOrdered,
                PMemoryCachePolicy::Uncached, false, true}},
        {"vendor", 0xffffffe0, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}}
    };
    std::array<KARMv7MMPURegion, 16> output;
    size_t count;
    ASSERT_EQ(kbuild_armv7m_mpu_regions({regions, std::size(regions), false}, output, count), KMemorySetupResult_Success);
    EXPECT_GT(count, 0U);
    ASSERT_EQ(
        kbuild_armv7m_mpu_regions({regions, std::size(regions), true}, std::span(output).first(0), count),
        KMemorySetupResult_Success);
    EXPECT_EQ(count, 0U);
}

TEST(MemoryProtection, RetainsPermissionsAndAttributesDifferentFromTheBackgroundMap)
{
    const PMemoryRegionDefinition regions[] =
    {
        {"no.access", 0x40000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::None, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}},
        {"read.only", 0x40000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadOnly, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}},
        {"user.read", 0x40000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::ReadOnly, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}},
        {"user.write", 0x40000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::ReadWrite, PMemoryType::Device,
                PMemoryCachePolicy::Uncached, false, false}},
        {"shareable", 0x40000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, true}},
        {"uncached", 0x24000000, 32, PMemoryRegionPurpose::Data,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Normal, PMemoryCachePolicy::Uncached, true, false}},
        {"execute.never", 0x24000000, 32, PMemoryRegionPurpose::Data,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Normal,
                PMemoryCachePolicy::WriteBackAllocate, false, false}},
        {"outer.uncached", 0x24000000, 32, PMemoryRegionPurpose::Data,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Normal,
                PMemoryCachePolicy::WriteBackAllocate, true, false}}
    };
    std::array<KARMv7MMPURegion, 1> output;
    size_t count;
    for (const PMemoryRegionDefinition& region : regions)
    {
        ASSERT_EQ(kbuild_armv7m_mpu_regions({&region, 1, true}, output, count), KMemorySetupResult_Success);
        EXPECT_EQ(count, 1U);
    }
}

TEST(MemoryProtection, DoesNotOmitPartialMatchesOrBypassValidation)
{
    PMemoryRegionDefinition region =
    {
        "crosses.ppb", 0xdfffffe0, 0x00100040, PMemoryRegionPurpose::Device,
        {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}
    };
    std::array<KARMv7MMPURegion, 16> output;
    size_t count;
    // Both ends match the default Device attributes, but the PPB between them is Strongly-ordered.
    ASSERT_EQ(kbuild_armv7m_mpu_regions({&region, 1, true}, output, count), KMemorySetupResult_Success);
    EXPECT_GT(count, 0U);
    region.Address = 0x40000001;
    region.Size = 32;
    EXPECT_EQ(kbuild_armv7m_mpu_regions({&region, 1, true}, output, count), KMemorySetupResult_UnsupportedAlignment);
    region.Address = 0x40000000;
    region.Attributes.CachePolicy = PMemoryCachePolicy::WriteBack;
    EXPECT_EQ(kbuild_armv7m_mpu_regions({&region, 1, true}, output, count), KMemorySetupResult_UnsupportedAttributes);
    region.Attributes.CachePolicy = PMemoryCachePolicy::Uncached;
    const PMemoryRegionDefinition overlapping[] = {region, region};
    EXPECT_EQ(kbuild_armv7m_mpu_regions({overlapping, 2, true}, output, count), KMemorySetupResult_InvalidRegion);
}

TEST(MemoryProtection, CompactionDoesNotCoverAnOmittedRegion)
{
    const PMemoryRegionDefinition regions[] =
    {
        {"first", 0x40000000, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::ReadOnly, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}},
        {"default", 0x40000020, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::None, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}},
        {"last", 0x40000040, 32, PMemoryRegionPurpose::Device,
            {PMemoryAccess::ReadWrite, PMemoryAccess::ReadOnly, PMemoryType::Device, PMemoryCachePolicy::Uncached, false, false}}
    };
    std::array<KARMv7MMPURegion, 1> output;
    size_t count;
    ASSERT_EQ(kbuild_armv7m_mpu_regions({regions, std::size(regions), true}, output, count), KMemorySetupResult_Success);
    ASSERT_EQ(count, 1U);
    EXPECT_TRUE(mpu_region_contains_address(output[0], 0x40000000));
    EXPECT_FALSE(mpu_region_contains_address(output[0], 0x40000020));
    EXPECT_TRUE(mpu_region_contains_address(output[0], 0x40000040));
}

#endif

TEST(MemoryProtection, FirmwareMapFitsAndPreservesLogicalRegionBoundaries)
{
    PMemoryRegionTable table = *__app_definition.MemoryRegions;
    table.UsePrivilegedDefaultMap = false;
    std::array<KARMv7MMPURegion, MEMORY_REGION_LIMIT> output;
    size_t count;
    ASSERT_EQ(kbuild_armv7m_mpu_regions(table, output, count), KMemorySetupResult_Success);
    for (size_t i = 0; i < table.Count; ++i)
    {
        const PMemoryRegionDefinition& region = table.Regions[i];
        const uint64_t addresses[] = {region.Address, uint64_t(region.Address) + region.Size - 1};
        for (uint64_t address : addresses)
        {
            size_t matches = 0;
            for (size_t j = 0; j < count; ++j)
            {
                if (mpu_region_contains_address(output[j], address))
                {
                    ++matches;
                    EXPECT_EQ((output[j].Attributes & MPU_RASR_XN_Msk) == 0, region.Attributes.Executable);
                    const uint32_t access = (output[j].Attributes & MPU_RASR_AP_Msk) >> MPU_RASR_AP_Pos;
                    if (region.Attributes.UserAccess == PMemoryAccess::ReadOnly) {
                        EXPECT_EQ(access, ARM_MPU_AP_RO);
                    } else if (region.Attributes.UserAccess == PMemoryAccess::ReadWrite) {
                        EXPECT_EQ(access, ARM_MPU_AP_FULL);
                    } else if (region.Attributes.KernelAccess == PMemoryAccess::None) {
                        EXPECT_EQ(access, ARM_MPU_AP_NONE);
                    } else {
                        EXPECT_EQ(access, ARM_MPU_AP_PRIV);
                    }
                }
            }
            EXPECT_EQ(matches, 1U);
        }
        if (i + 1 < table.Count && uint64_t(region.Address) + region.Size < table.Regions[i + 1].Address)
        {
            for (size_t j = 0; j < count; ++j) {
                EXPECT_FALSE(mpu_region_contains_address(output[j], uint64_t(region.Address) + region.Size));
            }
        }
    }
    ASSERT_EQ(
        kbuild_armv7m_mpu_regions(*__app_definition.MemoryRegions, std::span(output).first(16), count),
        KMemorySetupResult_Success);
}

} // namespace kernel

#endif
