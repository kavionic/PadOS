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

#include <limits>
#include <utility>

#include "KGProfData.h"
#include <Kernel/KStackFrames.h>
#include <Kernel/Profiler/KGProfSampler.h>
#include <Kernel/Scheduler.h>

namespace kernel
{

static_assert(KGPROF_SAMPLE_RATE_HZ > 0 && KGPROF_SAMPLE_RATE_HZ <= SYS_TICKS_PER_SEC);
static_assert(KGPROF_BIN_SIZE_BYTES > 0 && (KGPROF_BIN_SIZE_BYTES & (KGPROF_BIN_SIZE_BYTES - 1)) == 0);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __attribute__((no_instrument_function)) kgprof_record_sample(const KExceptionStackFrame* exceptionFrame) noexcept
{
    KGProfData& profilerData = g_KGProfData;
    if (profilerData.State != KGProfState::Running || exceptionFrame == nullptr) {
        return;
    }

    profilerData.SamplePhase += KGPROF_SAMPLE_RATE_HZ;
    if (profilerData.SamplePhase < SYS_TICKS_PER_SEC) {
        return;
    }
    profilerData.SamplePhase -= SYS_TICKS_PER_SEC;
    ++profilerData.TotalSamples;

    const uint32_t programCounter = exceptionFrame->PC;
    for (size_t imageIndex = 0; imageIndex < profilerData.Images.size(); ++imageIndex)
    {
        KGProfImageData& image = profilerData.Images[imageIndex];
        for (KGProfRegionData& region : image.Regions)
        {
            if (programCounter >= region.ActualLowPC && programCounter < region.ActualHighPC)
            {
                const size_t binIndex = size_t(programCounter - region.HistogramLowPC) / KGPROF_BIN_SIZE_BYTES;
                uint32_t& counter = region.Counters[binIndex];
                if (counter != std::numeric_limits<uint32_t>::max()) {
                    ++counter;
                } else {
                    ++profilerData.SaturatedSamples;
                }
                if (imageIndex == std::to_underlying(KGProfImage::Kernel)) {
                    ++profilerData.KernelSamples;
                } else {
                    ++profilerData.ApplicationSamples;
                }
                return;
            }
        }
    }
    ++profilerData.UnmappedSamples;
}

} // namespace kernel
