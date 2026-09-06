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

namespace kernel
{

static constexpr uint32_t KGPROF_GMON_VERSION = 1;
static constexpr uint8_t KGPROF_GMON_HISTOGRAM_TAG = 0;
static constexpr uint32_t KGPROF_MAX_WIRE_COUNT = std::numeric_limits<uint16_t>::max();
static constexpr size_t KGPROF_COUNTERS_PER_WRITE = 256;

KGProfData g_KGProfData;

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

static PErrorCode KGProfInitializeImage(const PFirmwareProfileInfo& definition, KGProfImageData& image, size_t& counterBytes)
{
    if (definition.Magic != PFIRMWARE_PROFILE_INFO_MAGIC ||
        definition.Version != PFIRMWARE_PROFILE_INFO_VERSION ||
        definition.RegionCount != PFIRMWARE_PROFILE_REGION_COUNT) {
        return PErrorCode::INVAL;
    }

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
    KGProfPutLE32(&recordHeader[13], KGPROF_SAMPLE_RATE_HZ);
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
    return PErrorCode::Success;
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
        profilerData.State = KGProfState::Preparing;
        reuseCapture = profilerData.HasCapture;
    }

    if (reuseCapture)
    {
        KGProfClearImages(profilerData);
        {
            CRITICAL_SCOPE(CRITICAL_IRQ);
            KGProfResetStatistics(profilerData);
            profilerData.State = KGProfState::Running;
        }
        return PErrorCode::Success;
    }

    std::array<KGProfImageData, KGPROF_IMAGE_COUNT> images;
    size_t counterBytes = 0;
    PErrorCode result = KGProfInitializeImage(
        __kernel_definition.ProfileInfo,
        images[std::to_underlying(KGProfImage::Kernel)],
        counterBytes);
    if (result == PErrorCode::Success)
    {
        result = KGProfInitializeImage(
            __app_definition.ProfileInfo,
            images[std::to_underlying(KGProfImage::Application)],
            counterBytes);
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
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kgprof_stop() noexcept
{
    KGProfData& profilerData = g_KGProfData;
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (profilerData.State == KGProfState::Preparing || profilerData.State == KGProfState::Writing) {
        return PErrorCode::BUSY;
    }
    profilerData.State = KGProfState::Stopped;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KGProfStatus kgprof_get_status() noexcept
{
    KGProfData& profilerData = g_KGProfData;
    CRITICAL_SCOPE(CRITICAL_IRQ);
    return
    {
        .Running = profilerData.State == KGProfState::Running,
        .Busy = profilerData.State == KGProfState::Preparing || profilerData.State == KGProfState::Writing,
        .HasCapture = profilerData.HasCapture,
        .SampleRateHz = KGPROF_SAMPLE_RATE_HZ,
        .BinSizeBytes = KGPROF_BIN_SIZE_BYTES,
        .TotalSamples = profilerData.TotalSamples,
        .KernelSamples = profilerData.KernelSamples,
        .ApplicationSamples = profilerData.ApplicationSamples,
        .UnmappedSamples = profilerData.UnmappedSamples,
        .SaturatedSamples = profilerData.SaturatedSamples,
        .CounterBytes = profilerData.CounterBytes
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
        if (profilerData.State == KGProfState::Preparing || profilerData.State == KGProfState::Writing) {
            return PErrorCode::BUSY;
        }
        if (!profilerData.HasCapture) {
            return PErrorCode::NOENT;
        }
        profilerData.State = KGProfState::Writing;
    }

    PErrorCode result = KGProfWriteImage(
        callback,
        context,
        KGProfImage::Kernel,
        profilerData.Images[std::to_underlying(KGProfImage::Kernel)]);

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

} // namespace kernel
