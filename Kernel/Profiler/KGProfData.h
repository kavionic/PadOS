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
// Created: 06.09.2026 00:00

#pragma once

#include <array>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <utility>

#include <Kernel/Profiler/KGProf.h>
#include <System/AppDefinition.h>

namespace kernel
{

enum class KGProfState : uint8_t
{
    Stopped,
    Preparing,
    Running,
    Writing
};

struct KGProfRegionData
{
    uint32_t ActualLowPC = 0;
    uint32_t ActualHighPC = 0;
    uint32_t HistogramLowPC = 0;
    uint32_t HistogramHighPC = 0;
    size_t BinCount = 0;
    std::unique_ptr<uint32_t[]> Counters;
};

struct KGProfImageData
{
    std::array<KGProfRegionData, PFIRMWARE_PROFILE_REGION_COUNT> Regions;
};

inline constexpr size_t KGPROF_IMAGE_COUNT = std::to_underlying(KGProfImage::Application) + 1;

struct KGProfData
{
    std::array<KGProfImageData, KGPROF_IMAGE_COUNT> Images;
    volatile KGProfState State = KGProfState::Stopped;
    bool HasCapture = false;
    uint32_t SamplePhase = 0;
    uint32_t TotalSamples = 0;
    uint32_t KernelSamples = 0;
    uint32_t ApplicationSamples = 0;
    uint32_t UnmappedSamples = 0;
    uint32_t SaturatedSamples = 0;
    size_t CounterBytes = 0;
};

extern KGProfData g_KGProfData;

} // namespace kernel
