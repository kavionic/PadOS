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

#include <Kernel/KGProfSampler.h>
#include <Kernel/KStackFrames.h>
#include <Kernel/Scheduler.h>
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

static constexpr size_t     KGPROF_IMAGE_COUNT          = std::to_underlying(KGProfImage::Application) + 1;
static constexpr uint32_t   KGPROF_GMON_VERSION         = 1;
static constexpr uint8_t    KGPROF_GMON_HISTOGRAM_TAG   = 0;
static constexpr uint32_t   KGPROF_MAX_WIRE_COUNT       = std::numeric_limits<uint16_t>::max();
static constexpr size_t     KGPROF_COUNTERS_PER_WRITE   = 256;

static std::array<KGProfImageData, KGPROF_IMAGE_COUNT> g_KGProfImages;
static volatile KGProfState g_KGProfState = KGProfState::Stopped;
static bool     g_KGProfHasCapture = false;
static uint32_t g_KGProfSamplePhase = 0;
static uint32_t g_KGProfTotalSamples = 0;
static uint32_t g_KGProfKernelSamples = 0;
static uint32_t g_KGProfApplicationSamples = 0;
static uint32_t g_KGProfUnmappedSamples = 0;
static uint32_t g_KGProfSaturatedSamples = 0;
static size_t g_KGProfCounterBytes = 0;

static_assert(KGPROF_SAMPLE_RATE_HZ > 0 && KGPROF_SAMPLE_RATE_HZ <= SYS_TICKS_PER_SEC);
static_assert(KGPROF_BIN_SIZE_BYTES > 0 && (KGPROF_BIN_SIZE_BYTES & (KGPROF_BIN_SIZE_BYTES - 1)) == 0);

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

static void KGProfClearImages()
{
    for (KGProfImageData& image : g_KGProfImages)
    {
        for (KGProfRegionData& region : image.Regions) {
            std::fill_n(region.Counters.get(), region.BinCount, uint32_t(0));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfResetStatistics()
{
    g_KGProfSamplePhase = 0;
    g_KGProfTotalSamples = 0;
    g_KGProfKernelSamples = 0;
    g_KGProfApplicationSamples = 0;
    g_KGProfUnmappedSamples = 0;
    g_KGProfSaturatedSamples = 0;
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
    bool reuseCapture = false;
    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        if (g_KGProfState != KGProfState::Stopped) {
            return PErrorCode::BUSY;
        }
        g_KGProfState = KGProfState::Preparing;
        reuseCapture = g_KGProfHasCapture;
    }

    if (reuseCapture)
    {
        KGProfClearImages();
        {
            CRITICAL_SCOPE(CRITICAL_IRQ);
            KGProfResetStatistics();
            g_KGProfState = KGProfState::Running;
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
        g_KGProfState = KGProfState::Stopped;
        return result;
    }

    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        g_KGProfImages.swap(images);
        KGProfResetStatistics();
        g_KGProfCounterBytes = counterBytes;
        g_KGProfHasCapture = true;
        g_KGProfState = KGProfState::Running;
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kgprof_stop() noexcept
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    if (g_KGProfState == KGProfState::Preparing || g_KGProfState == KGProfState::Writing) {
        return PErrorCode::BUSY;
    }
    g_KGProfState = KGProfState::Stopped;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KGProfStatus kgprof_get_status() noexcept
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    return
    {
        .Running = g_KGProfState == KGProfState::Running,
        .Busy = g_KGProfState == KGProfState::Preparing || g_KGProfState == KGProfState::Writing,
        .HasCapture = g_KGProfHasCapture,
        .SampleRateHz = KGPROF_SAMPLE_RATE_HZ,
        .BinSizeBytes = KGPROF_BIN_SIZE_BYTES,
        .TotalSamples = g_KGProfTotalSamples,
        .KernelSamples = g_KGProfKernelSamples,
        .ApplicationSamples = g_KGProfApplicationSamples,
        .UnmappedSamples = g_KGProfUnmappedSamples,
        .SaturatedSamples = g_KGProfSaturatedSamples,
        .CounterBytes = g_KGProfCounterBytes
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

    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        if (g_KGProfState == KGProfState::Preparing || g_KGProfState == KGProfState::Writing) {
            return PErrorCode::BUSY;
        }
        if (!g_KGProfHasCapture) {
            return PErrorCode::NOENT;
        }
        g_KGProfState = KGProfState::Writing;
    }

    PErrorCode result = KGProfWriteImage(
        callback,
        context,
        KGProfImage::Kernel,
        g_KGProfImages[std::to_underlying(KGProfImage::Kernel)]);
    
    if (result == PErrorCode::Success)
    {
        result = KGProfWriteImage(
            callback,
            context,
            KGProfImage::Application,
            g_KGProfImages[std::to_underlying(KGProfImage::Application)]);
    }

    {
        CRITICAL_SCOPE(CRITICAL_IRQ);
        g_KGProfState = KGProfState::Stopped;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __attribute__((no_instrument_function)) kgprof_record_sample(const KExceptionStackFrame* exceptionFrame) noexcept
{
    if (g_KGProfState != KGProfState::Running || exceptionFrame == nullptr) {
        return;
    }

    g_KGProfSamplePhase += KGPROF_SAMPLE_RATE_HZ;
    if (g_KGProfSamplePhase < SYS_TICKS_PER_SEC) {
        return;
    }
    g_KGProfSamplePhase -= SYS_TICKS_PER_SEC;
    ++g_KGProfTotalSamples;

    const uint32_t programCounter = exceptionFrame->PC;
    for (size_t imageIndex = 0; imageIndex < g_KGProfImages.size(); ++imageIndex)
    {
        KGProfImageData& image = g_KGProfImages[imageIndex];
        for (KGProfRegionData& region : image.Regions)
        {
            if (programCounter >= region.ActualLowPC && programCounter < region.ActualHighPC)
            {
                const size_t binIndex = size_t(programCounter - region.HistogramLowPC) / KGPROF_BIN_SIZE_BYTES;
                uint32_t& counter = region.Counters[binIndex];
                if (counter != std::numeric_limits<uint32_t>::max()) {
                    ++counter;
                } else {
                    ++g_KGProfSaturatedSamples;
                }
                if (imageIndex == std::to_underlying(KGProfImage::Kernel)) {
                    ++g_KGProfKernelSamples;
                } else {
                    ++g_KGProfApplicationSamples;
                }
                return;
            }
        }
    }
    ++g_KGProfUnmappedSamples;
}

} // namespace kernel
