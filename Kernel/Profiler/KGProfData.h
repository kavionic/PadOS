// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 06.09.2026 00:00

#pragma once

#include <array>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <utility>

#include <Kernel/Profiler/KGProf.h>
#include <Kernel/Profiler/KGProfSampler.h>
#include <System/AppDefinition.h>
#include <System/GProf.h>

namespace kernel
{

enum class KGProfState : uint8_t
{
    Stopped,
    Preparing,
    Running,
    Stopping,
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

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
struct KGProfArcDeleter
{
    bool Userspace = false;
    void operator()(PGProfArc* arcs) const noexcept;
};
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

struct KGProfImageData
{
    std::array<KGProfRegionData, PFIRMWARE_PROFILE_REGION_COUNT> Regions;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    std::unique_ptr<PGProfArc[], KGProfArcDeleter> Arcs;
    PGProfCallGraph* CallGraph = nullptr;
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
};

inline constexpr size_t KGPROF_IMAGE_COUNT = std::to_underlying(KGProfImage::COUNT);

struct KGProfData
{
    std::array<KGProfImageData, KGPROF_IMAGE_COUNT> Images;
    volatile KGProfState State = KGProfState::Stopped;
    bool HasCapture = false;
    uint32_t SampleRateHz = KGPROF_SAMPLE_RATE_HZ;
    uint32_t SamplePhase = 0;
    uint32_t TotalSamples = 0;
    uint32_t KernelSamples = 0;
    uint32_t ApplicationSamples = 0;
    uint32_t UnmappedSamples = 0;
    uint32_t SaturatedSamples = 0;
    size_t CounterBytes = 0;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    size_t ArcCount = 0;
    uint64_t RecordedCalls = 0;
    uint64_t UnmappedCalls = 0;
    uint64_t DroppedCalls = 0;
    uint64_t SaturatedCalls = 0;
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
};

extern KGProfData g_KGProfData;

} // namespace kernel
