// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <span>
#include <Kernel/Startup/KMemoryStartup.h>

namespace kernel
{

struct KARMv7MMPURegion
{
    uint32_t Address;
    uint32_t Attributes;
    uint8_t SizeExponent;
    uint8_t DisabledSubregions;
};

// Produces exact, non-overlapping enabled ranges. Disabled subregions may overlap other MPU region envelopes.
// On supported cores, regions matching the enabled privileged default map do not require hardware entries.
KMemorySetupResult kbuild_armv7m_mpu_regions(
    const PMemoryRegionTable& table,
    std::span<KARMv7MMPURegion> output,
    size_t& regionCount);

} // namespace kernel
