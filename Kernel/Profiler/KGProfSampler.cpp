// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

static_assert(KGPROF_SAMPLE_RATE_HZ > 0);
#ifndef PADOS_GPROF_HW_TIMER
static_assert(KGPROF_SAMPLE_RATE_HZ <= SYS_TICKS_PER_SEC);
#endif // !PADOS_GPROF_HW_TIMER
static_assert(KGPROF_BIN_SIZE_BYTES > 0 && (KGPROF_BIN_SIZE_BYTES & (KGPROF_BIN_SIZE_BYTES - 1)) == 0);

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" PGProfThreadState* p_gprof_get_thread_state()
{
    KThreadCB* const thread = gk_CurrentThread;
    return (thread != nullptr) ? &thread->m_GProfState : nullptr;
}
#endif // PADOS_MODULE_GPROF_CALL_GRAPH

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __attribute__((no_instrument_function)) kgprof_record_sample(const KExceptionStackFrame* exceptionFrame) noexcept
{
    KGProfData& profilerData = g_KGProfData;
    if (profilerData.State != KGProfState::Running || exceptionFrame == nullptr) {
        return;
    }

#ifndef PADOS_GPROF_HW_TIMER
    profilerData.SamplePhase += KGPROF_SAMPLE_RATE_HZ;
    if (profilerData.SamplePhase < SYS_TICKS_PER_SEC) {
        return;
    }
    profilerData.SamplePhase -= SYS_TICKS_PER_SEC;
#endif // !PADOS_GPROF_HW_TIMER
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
