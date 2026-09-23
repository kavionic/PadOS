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
// Created: 03.09.2026 00:00

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <new>
#include <utility>

#include <string.h>

#include "KGProfData.h"
#include <Kernel/Profiler/KGProf.h>
#include <Kernel/Profiler/KGProfSampler.h>
#include <Kernel/Scheduler.h>
#include <System/AppDefinition.h>
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
#include <Kernel/KConditionVariable.h>
#include <Kernel/KPIDNode.h>
#include <Kernel/KUserspaceService.h>
#include <Ptr/NoPtr.h>
#include <Threads/ThreadUserspaceState.h>
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

namespace kernel
{

static constexpr uint32_t KGPROF_GMON_VERSION = 1;
static constexpr uint8_t KGPROF_GMON_HISTOGRAM_TAG = 0;
static constexpr uint32_t KGPROF_MAX_WIRE_COUNT = std::numeric_limits<uint16_t>::max();
static constexpr size_t KGPROF_COUNTERS_PER_WRITE = 256;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
static constexpr uint8_t KGPROF_GMON_CALL_GRAPH_TAG = 1;
static constexpr size_t KGPROF_ARC_RECORD_SIZE = 13;
static constexpr size_t KGPROF_ARCS_PER_WRITE = 32;
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

KGProfData g_KGProfData;

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
static NoPtr<KConditionVariable> gk_KGProfDrainCondition("gprof_drain");
static handle_id gk_KGProfDrainHandle = INVALID_HANDLE;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KGProfArcDeleter::operator()(PGProfArc* arcs) const noexcept
{
    if (Userspace)
    {
        PUserspaceServiceRequest request
        {
            .Command = PUserspaceServiceCommand::FreeMemory,
            .Memory = arcs
        };
        if (kuserspace_service_request(request) != PErrorCode::Success) {
            panic("User-space service failed to release a call-graph table.\n");
        }
    }
    else
    {
        delete[] arcs;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool KGProfThreadIsRecording(KThreadCB& thread)
{
    if (std::atomic_ref<uint32_t>(thread.m_GProfState.Active).load() != 0) {
        return true;
    }
    return thread.m_ThreadUserData != nullptr &&
        std::atomic_ref<uint32_t>(thread.m_ThreadUserData->GProfState.Active).load() != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool KGProfHasActiveRecorders()
{
    // The PID-map mutex and scheduler lock keep thread records alive here.
    for (const auto& entry : g_PIDMap)
    {
        const Ptr<KThreadCB>& thread = entry.second->Thread;
        if (thread != nullptr && !thread->IsZombie() && KGProfThreadIsRecording(*thread)) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfSetCallGraphRunning(KGProfData& profilerData, bool running)
{
    for (KGProfImageData& image : profilerData.Images)
    {
        PGProfCallGraph& graph = *image.CallGraph;
        if (running) {
            graph.Arcs = image.Arcs.get();
        }
        graph.DrainRequested.store(!running, std::memory_order_release);
        graph.Enabled->store(running ? 1 : 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode KGProfWaitForRecorders(KGProfData& profilerData)
{
    for (;;)
    {
        KMutexGuardRaw threadLock(g_PIDMapMutex, true);
        CRITICAL_SCOPE(CRITICAL_IRQ);
        const bool active = KGProfHasActiveRecorders();
        threadLock.Unlock();
        if (!active)
        {
            for (KGProfImageData& image : profilerData.Images) {
                image.CallGraph->DrainRequested.store(false, std::memory_order_release);
            }
            return PErrorCode::Success;
        }
        // IRQWait links the waiter before allowing a recorder to run and
        // signal completion. There is no polling or lost-wakeup window.
        const PErrorCode result = gk_KGProfDrainCondition.IRQWait();
        if (result != PErrorCode::Success) {
            return result;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfClearCallGraph(KGProfImageData& image)
{
    PGProfCallGraph& graph = *image.CallGraph;
    std::fill_n(image.Arcs.get(), PGPROF_ARC_CAPACITY, PGProfArc{});
    graph.DroppedCalls = 0;
    graph.SaturatedCalls = 0;
    graph.UnmappedCalls = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool KGProfArcMatchesImage(const PGProfArc& arc, const KGProfImageData& image)
{
    bool callerMapped = false;
    bool calleeMapped = false;
    for (const KGProfRegionData& region : image.Regions)
    {
        callerMapped |= arc.CallerPC >= region.ActualLowPC && arc.CallerPC < region.ActualHighPC;
        calleeMapped |= arc.CalleePC >= region.ActualLowPC && arc.CalleePC < region.ActualHighPC;
    }
    return callerMapped && calleeMapped;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfSummarizeCallGraphs(KGProfData& profilerData)
{
    size_t arcCount = 0;
    uint64_t recordedCalls = 0;
    uint64_t unmappedCalls = 0;
    uint64_t droppedCalls = 0;
    uint64_t saturatedCalls = 0;
    for (const KGProfImageData& image : profilerData.Images)
    {
        const PGProfCallGraph& graph = *image.CallGraph;
        unmappedCalls += graph.UnmappedCalls;
        droppedCalls += graph.DroppedCalls;
        saturatedCalls += graph.SaturatedCalls;
        for (size_t i = 0; i < PGPROF_ARC_CAPACITY; ++i)
        {
            const PGProfArc& arc = graph.Arcs[i];
            if (arc.CalleePC == 0) {
                continue;
            }
            if (KGProfArcMatchesImage(arc, image))
            {
                ++arcCount;
                recordedCalls += arc.Count;
            }
            else
            {
                unmappedCalls += arc.Count;
            }
        }
    }
    CRITICAL_SCOPE(CRITICAL_IRQ);
    profilerData.ArcCount = arcCount;
    profilerData.RecordedCalls = recordedCalls;
    profilerData.UnmappedCalls = unmappedCalls;
    profilerData.DroppedCalls = droppedCalls;
    profilerData.SaturatedCalls = saturatedCalls;
}
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfPutLE16(uint8_t* destination, uint16_t value)
{
    destination[0] = uint8_t(value);
    destination[1] = uint8_t(value >> 8);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfPutLE32(uint8_t* destination, uint32_t value)
{
    destination[0] = uint8_t(value);
    destination[1] = uint8_t(value >> 8);
    destination[2] = uint8_t(value >> 16);
    destination[3] = uint8_t(value >> 24);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode KGProfInitializeRegion(const PFirmwareExecutableRegion& definition, KGProfRegionData& region)
{
    if (definition.Start == nullptr || definition.End == nullptr) {
        return PErrorCode::INVAL;
    }

    const uintptr_t actualLowPC = reinterpret_cast<uintptr_t>(definition.Start);
    const uintptr_t actualHighPC = reinterpret_cast<uintptr_t>(definition.End);
    const uintptr_t binMask = KGPROF_BIN_SIZE_BYTES - 1;

    if (actualLowPC >= actualHighPC || actualHighPC > std::numeric_limits<uint32_t>::max() - binMask) {
        return PErrorCode::INVAL;
    }

    const uintptr_t histogramLowPC = actualLowPC & ~binMask;
    const uintptr_t histogramHighPC = (actualHighPC + binMask) & ~binMask;
    const size_t binCount = size_t((histogramHighPC - histogramLowPC) / KGPROF_BIN_SIZE_BYTES);

    std::unique_ptr<uint32_t[]> counters(new(std::nothrow) uint32_t[binCount]());
    if (counters.get() == nullptr) {
        return PErrorCode::NOMEM;
    }

    region.ActualLowPC      = uint32_t(actualLowPC);
    region.ActualHighPC     = uint32_t(actualHighPC);
    region.HistogramLowPC   = uint32_t(histogramLowPC);
    region.HistogramHighPC  = uint32_t(histogramHighPC);
    region.BinCount         = binCount;
    region.Counters         = std::move(counters);

    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode KGProfInitializeImage(
    const PFirmwareProfileInfo& definition,
    KGProfImageData& image,
    size_t& counterBytes,
    [[maybe_unused]] KGProfImage imageID)
{
    if (definition.Magic != PFIRMWARE_PROFILE_INFO_MAGIC ||
        definition.Version != PFIRMWARE_PROFILE_INFO_VERSION ||
        definition.RegionCount != PFIRMWARE_PROFILE_REGION_COUNT) {
        return PErrorCode::INVAL;
    }

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    if (definition.CallGraph == nullptr || definition.CallGraph->Enabled == nullptr) {
        return PErrorCode::INVAL;
    }
    image.CallGraph = definition.CallGraph;
    image.CallGraph->DrainCondition = gk_KGProfDrainHandle;
    if (imageID == KGProfImage::Application)
    {
        PUserspaceServiceRequest request
        {
            .Command = PUserspaceServiceCommand::AllocateMemory,
            .Size = PGPROF_ARC_CAPACITY * sizeof(PGProfArc)
        };
        const PErrorCode result = kuserspace_service_request(request);
        if (result != PErrorCode::Success) {
            return result;
        }
        image.Arcs = {static_cast<PGProfArc*>(request.Memory), KGProfArcDeleter{.Userspace = true}};
    }
    else
    {
        image.Arcs.reset(new(std::nothrow) PGProfArc[PGPROF_ARC_CAPACITY]);
        if (image.Arcs == nullptr) {
            return PErrorCode::NOMEM;
        }
    }
    KGProfClearCallGraph(image);
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

    for (size_t i = 0; i < image.Regions.size(); ++i)
    {
        const PErrorCode result = KGProfInitializeRegion(definition.Regions[i], image.Regions[i]);
        if (result != PErrorCode::Success) {
            return result;
        }
        const size_t regionBytes = image.Regions[i].BinCount * sizeof(uint32_t);
        if (regionBytes > std::numeric_limits<size_t>::max() - counterBytes) {
            return PErrorCode::OVERFLOW;
        }
        counterBytes += regionBytes;
    }

    for (size_t lhsIndex = 0; lhsIndex < image.Regions.size(); ++lhsIndex)
    {
        const KGProfRegionData& lhs = image.Regions[lhsIndex];
        for (size_t rhsIndex = lhsIndex + 1; rhsIndex < image.Regions.size(); ++rhsIndex)
        {
            const KGProfRegionData& rhs = image.Regions[rhsIndex];
            if (lhs.HistogramLowPC < rhs.HistogramHighPC && rhs.HistogramLowPC < lhs.HistogramHighPC) {
                return PErrorCode::INVAL;
            }
        }
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfClearImages(KGProfData& profilerData)
{
    for (KGProfImageData& image : profilerData.Images)
    {
        for (KGProfRegionData& region : image.Regions) {
            std::fill_n(region.Counters.get(), region.BinCount, uint32_t(0));
        }
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        KGProfClearCallGraph(image);
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfResetStatistics(KGProfData& profilerData)
{
    profilerData.SamplePhase = 0;
    profilerData.TotalSamples = 0;
    profilerData.KernelSamples = 0;
    profilerData.ApplicationSamples = 0;
    profilerData.UnmappedSamples = 0;
    profilerData.SaturatedSamples = 0;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    profilerData.ArcCount = 0;
    profilerData.RecordedCalls = 0;
    profilerData.UnmappedCalls = 0;
    profilerData.DroppedCalls = 0;
    profilerData.SaturatedCalls = 0;
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode KGProfWriteHistogramRecord(
    KGProfWriteCallback     callback,
    void*                   context,
    KGProfImage             image,
    const KGProfRegionData& region,
    uint32_t                layer)
{
    std::array<uint8_t, 33> recordHeader = {};
    recordHeader[0] = KGPROF_GMON_HISTOGRAM_TAG;
    KGProfPutLE32(&recordHeader[1], region.HistogramLowPC);
    KGProfPutLE32(&recordHeader[5], region.HistogramHighPC);
    KGProfPutLE32(&recordHeader[9], uint32_t(region.BinCount));
    KGProfPutLE32(&recordHeader[13], g_KGProfData.SampleRateHz);
    memcpy(&recordHeader[17], "seconds", 7);
    recordHeader[32] = 's';

    PErrorCode result = callback(context, image, recordHeader.data(), recordHeader.size());
    if (result != PErrorCode::Success) {
        return result;
    }

    const uint64_t layerBase = uint64_t(layer) * KGPROF_MAX_WIRE_COUNT;
    std::array<uint8_t, KGPROF_COUNTERS_PER_WRITE * sizeof(uint16_t)> outputBuffer;

    for (size_t offset = 0; offset < region.BinCount; offset += KGPROF_COUNTERS_PER_WRITE)
    {
        const size_t count = std::min(KGPROF_COUNTERS_PER_WRITE, region.BinCount - offset);
        for (size_t i = 0; i < count; ++i)
        {
            const uint64_t liveCount = region.Counters[offset + i];
            const uint64_t remainingCount = (liveCount > layerBase) ? liveCount - layerBase : 0;
            const uint16_t wireCount = uint16_t(std::min<uint64_t>(remainingCount, KGPROF_MAX_WIRE_COUNT));
            KGProfPutLE16(&outputBuffer[i * sizeof(uint16_t)], wireCount);
        }
        result = callback(context, image, outputBuffer.data(), count * sizeof(uint16_t));
        if (result != PErrorCode::Success) {
            return result;
        }
    }
    return PErrorCode::Success;
}

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode KGProfWriteCallGraph(
    KGProfWriteCallback callback,
    void* context,
    KGProfImage imageID,
    const KGProfImageData& image)
{
    std::array<uint8_t, KGPROF_ARCS_PER_WRITE * KGPROF_ARC_RECORD_SIZE> outputBuffer;
    size_t bytesUsed = 0;
    for (size_t i = 0; i < PGPROF_ARC_CAPACITY; ++i)
    {
        const PGProfArc& arc = image.CallGraph->Arcs[i];
        if (arc.CalleePC == 0 || !KGProfArcMatchesImage(arc, image)) {
            continue;
        }
        uint8_t* record = outputBuffer.data() + bytesUsed;
        record[0] = KGPROF_GMON_CALL_GRAPH_TAG;
        KGProfPutLE32(record + 1, arc.CallerPC);
        KGProfPutLE32(record + 5, arc.CalleePC);
        KGProfPutLE32(record + 9, arc.Count);
        bytesUsed += KGPROF_ARC_RECORD_SIZE;
        if (bytesUsed == outputBuffer.size())
        {
            const PErrorCode result = callback(context, imageID, outputBuffer.data(), bytesUsed);
            if (result != PErrorCode::Success) {
                return result;
            }
            bytesUsed = 0;
        }
    }
    return (bytesUsed != 0) ? callback(context, imageID, outputBuffer.data(), bytesUsed) : PErrorCode::Success;
}
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode KGProfWriteImage(
    KGProfWriteCallback     callback,
    void*                   context,
    KGProfImage             imageID,
    const KGProfImageData&  image)
{
    std::array<uint8_t, 20> fileHeader = {'g', 'm', 'o', 'n'};
    KGProfPutLE32(&fileHeader[4], KGPROF_GMON_VERSION);

    PErrorCode result = callback(context, imageID, fileHeader.data(), fileHeader.size());
    if (result != PErrorCode::Success) {
        return result;
    }

    for (const KGProfRegionData& region : image.Regions)
    {
        uint32_t maxCount = 0;
        for (size_t i = 0; i < region.BinCount; ++i) {
            maxCount = std::max(maxCount, region.Counters[i]);
        }

        const uint32_t layerCount = (maxCount == 0) ? 1 : uint32_t((uint64_t(maxCount) + KGPROF_MAX_WIRE_COUNT - 1) / KGPROF_MAX_WIRE_COUNT);
        for (uint32_t layer = 0; layer < layerCount; ++layer)
        {
            result = KGProfWriteHistogramRecord(callback, context, imageID, region, layer);
            if (result != PErrorCode::Success) {
                return result;
            }
        }
    }
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    return KGProfWriteCallGraph(callback, context, imageID, image);
#else
    return PErrorCode::Success;
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kgprof_start()
{
    KGProfData& profilerData = g_KGProfData;
    bool reuseCapture = false;
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        if (profilerData.State != KGProfState::Stopped) {
            return PErrorCode::BUSY;
        }
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        if (KGProfThreadIsRecording(*gk_CurrentThread)) {
            return PErrorCode::BUSY;
        }
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
        profilerData.State = KGProfState::Preparing;
        reuseCapture = profilerData.HasCapture;
    }

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    if (gk_KGProfDrainHandle == INVALID_HANDLE)
    {
        const PErrorCode result = KNamedObject::RegisterObject(gk_KGProfDrainHandle, ptr_tmp_cast(&gk_KGProfDrainCondition));
        if (result != PErrorCode::Success)
        {
            CRITICAL_SCOPE(CRITICAL_IRQ);
            profilerData.State = KGProfState::Stopped;
            return result;
        }
    }
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

#ifdef PADOS_GPROF_HW_TIMER
    const PErrorCode samplerResult = kgprof_initialize_sampler(profilerData.SampleRateHz);
    if (samplerResult != PErrorCode::Success)
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        profilerData.State = KGProfState::Stopped;
        return samplerResult;
    }
#endif // PADOS_GPROF_HW_TIMER

    if (reuseCapture)
    {
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        // Also drain a previous stop attempt that returned an error.
        const PErrorCode result = KGProfWaitForRecorders(profilerData);
        if (result != PErrorCode::Success)
        {
            CRITICAL_SCOPE(CRITICAL_IRQ);
            profilerData.State = KGProfState::Stopped;
            return result;
        }
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
        KGProfClearImages(profilerData);
        {
            CRITICAL_SCOPE(CRITICAL_IRQ);
            KGProfResetStatistics(profilerData);
            profilerData.State = KGProfState::Running;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
            KGProfSetCallGraphRunning(profilerData, true);
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
#ifdef PADOS_GPROF_HW_TIMER
            kgprof_start_sampler();
#endif // PADOS_GPROF_HW_TIMER
        }
        return PErrorCode::Success;
    }

    std::array<KGProfImageData, KGPROF_IMAGE_COUNT> images;
    size_t counterBytes = 0;
    PErrorCode result = KGProfInitializeImage(
        __kernel_definition.ProfileInfo,
        images[std::to_underlying(KGProfImage::Kernel)],
        counterBytes,
        KGProfImage::Kernel);
    if (result == PErrorCode::Success)
    {
        result = KGProfInitializeImage(
            __app_definition.ProfileInfo,
            images[std::to_underlying(KGProfImage::Application)],
            counterBytes,
            KGProfImage::Application);
    }

    if (result != PErrorCode::Success)
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        profilerData.State = KGProfState::Stopped;
        return result;
    }

    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        profilerData.Images.swap(images);
        KGProfResetStatistics(profilerData);
        profilerData.CounterBytes = counterBytes;
        profilerData.HasCapture = true;
        profilerData.State = KGProfState::Running;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        KGProfSetCallGraphRunning(profilerData, true);
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
#ifdef PADOS_GPROF_HW_TIMER
        kgprof_start_sampler();
#endif // PADOS_GPROF_HW_TIMER
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kgprof_stop() noexcept
{
    KGProfData& profilerData = g_KGProfData;
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        if (profilerData.State != KGProfState::Stopped && profilerData.State != KGProfState::Running) {
            return PErrorCode::BUSY;
        }
        if (!profilerData.HasCapture) {
            return PErrorCode::Success;
        }
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        if (KGProfThreadIsRecording(*gk_CurrentThread)) {
            return PErrorCode::BUSY;
        }
        KGProfSetCallGraphRunning(profilerData, false);
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
#ifdef PADOS_GPROF_HW_TIMER
        kgprof_stop_sampler();
#endif // PADOS_GPROF_HW_TIMER
        profilerData.State = KGProfState::Stopping;
    }
    PErrorCode result = PErrorCode::Success;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    result = KGProfWaitForRecorders(profilerData);
    if (result == PErrorCode::Success) {
        KGProfSummarizeCallGraphs(profilerData);
    }
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        profilerData.State = KGProfState::Stopped;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KGProfStatus kgprof_get_status() noexcept
{
    KGProfData& profilerData = g_KGProfData;
    CRITICAL_SCOPE(CRITICAL_IRQ);
#ifdef PADOS_GPROF_HW_TIMER
    const KGProfSamplerIRQGuard samplerIRQGuard;
#endif // PADOS_GPROF_HW_TIMER
    return
    {
        .Running = profilerData.State == KGProfState::Running,
        .Busy = profilerData.State != KGProfState::Stopped && profilerData.State != KGProfState::Running,
        .HasCapture = profilerData.HasCapture,
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        .CallGraphEnabled = true,
#else
        .CallGraphEnabled = false,
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
        .SampleRateHz = profilerData.SampleRateHz,
        .BinSizeBytes = KGPROF_BIN_SIZE_BYTES,
        .TotalSamples = profilerData.TotalSamples,
        .KernelSamples = profilerData.KernelSamples,
        .ApplicationSamples = profilerData.ApplicationSamples,
        .UnmappedSamples = profilerData.UnmappedSamples,
        .SaturatedSamples = profilerData.SaturatedSamples,
        .CounterBytes = profilerData.CounterBytes,
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        .ArcCount = profilerData.ArcCount,
        .ArcCapacity = KGPROF_IMAGE_COUNT * PGPROF_ARC_CAPACITY,
        .ArcBytes = profilerData.HasCapture ? KGPROF_IMAGE_COUNT * PGPROF_ARC_CAPACITY * sizeof(PGProfArc) : 0,
        .RecordedCalls = profilerData.RecordedCalls,
        .UnmappedCalls = profilerData.UnmappedCalls,
        .DroppedCalls = profilerData.DroppedCalls,
        .SaturatedCalls = profilerData.SaturatedCalls
#else
        .ArcCount = 0,
        .ArcCapacity = 0,
        .ArcBytes = 0,
        .RecordedCalls = 0,
        .UnmappedCalls = 0,
        .DroppedCalls = 0,
        .SaturatedCalls = 0
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
    };
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kgprof_write_gmon(KGProfWriteCallback callback, void* context) noexcept
{
    if (callback == nullptr) {
        return PErrorCode::INVAL;
    }

    KGProfData& profilerData = g_KGProfData;
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        if (profilerData.State != KGProfState::Stopped && profilerData.State != KGProfState::Running) {
            return PErrorCode::BUSY;
        }
        if (!profilerData.HasCapture) {
            return PErrorCode::NOENT;
        }
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
        if (KGProfThreadIsRecording(*gk_CurrentThread)) {
            return PErrorCode::BUSY;
        }
        KGProfSetCallGraphRunning(profilerData, false);
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
#ifdef PADOS_GPROF_HW_TIMER
        kgprof_stop_sampler();
#endif // PADOS_GPROF_HW_TIMER
        profilerData.State = KGProfState::Writing;
    }

    PErrorCode result = PErrorCode::Success;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
    result = KGProfWaitForRecorders(profilerData);
    if (result == PErrorCode::Success) {
        KGProfSummarizeCallGraphs(profilerData);
    }
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
    if (result == PErrorCode::Success) {
        result = KGProfWriteImage(
            callback,
            context,
            KGProfImage::Kernel,
            profilerData.Images[std::to_underlying(KGProfImage::Kernel)]);
    }

    if (result == PErrorCode::Success)
    {
        result = KGProfWriteImage(
            callback,
            context,
            KGProfImage::Application,
            profilerData.Images[std::to_underlying(KGProfImage::Application)]);
    }

    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        profilerData.State = KGProfState::Stopped;
    }
    return result;
}

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kgprof_thread_exited(KThreadCB& thread) noexcept
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    std::atomic_ref<uint32_t>(thread.m_GProfState.Active).store(0);
    if (thread.m_ThreadUserData != nullptr) {
        std::atomic_ref<uint32_t>(thread.m_ThreadUserData->GProfState.Active).store(0);
    }
    if (g_KGProfData.HasCapture && g_KGProfData.State != KGProfState::Running && g_KGProfData.State != KGProfState::Stopped) {
        gk_KGProfDrainCondition.WakeupAll();
    }
}
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

} // namespace kernel
