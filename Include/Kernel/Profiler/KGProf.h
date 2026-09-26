// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 03.09.2026 00:00

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <System/ErrorCodes.h>

namespace kernel
{

#ifdef PADOS_MODULE_GPROF_SAMPLING
enum class KGProfImage : uint8_t
{
    Kernel,
    Application,
    COUNT
};

using KGProfWriteCallback = PErrorCode (*)(void* context, KGProfImage image, const void* data, size_t length) noexcept;

struct KGProfStatus
{
    bool Running;
    bool Busy;
    bool HasCapture;
    bool CallGraphEnabled;
    uint32_t SampleRateHz;
    uint32_t BinSizeBytes;
    uint32_t TotalSamples;
    uint32_t KernelSamples;
    uint32_t ApplicationSamples;
    uint32_t UnmappedSamples;
    uint32_t SaturatedSamples;
    size_t CounterBytes;
    size_t ArcCount;
    size_t ArcCapacity;
    size_t ArcBytes;
    uint64_t RecordedCalls;
    uint64_t UnmappedCalls;
    uint64_t DroppedCalls;
    uint64_t SaturatedCalls;
};

PErrorCode kgprof_start();
PErrorCode kgprof_stop() noexcept;
KGProfStatus kgprof_get_status() noexcept;
PErrorCode kgprof_write_gmon(KGProfWriteCallback callback, void* context) noexcept;
#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
class KThreadCB;
void kgprof_thread_exited(KThreadCB& thread) noexcept;
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
#endif // PADOS_MODULE_GPROF_SAMPLING

} // namespace kernel
