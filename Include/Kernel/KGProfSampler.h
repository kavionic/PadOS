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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <System/ErrorCodes.h>

struct KExceptionStackFrame;

namespace kernel
{

#ifdef PADOS_MODULE_GPROF_SAMPLING
// Samples thread-mode execution from the lowest-priority SysTick interrupt.
inline constexpr uint32_t KGPROF_SAMPLE_RATE_HZ = 100;
inline constexpr uint32_t KGPROF_BIN_SIZE_BYTES = 32;

enum class KGProfImage : uint8_t
{
    Kernel,
    Application
};

using KGProfWriteCallback = PErrorCode (*)(void* context, KGProfImage image, const void* data, size_t length) noexcept;

struct KGProfStatus
{
    bool Running;
    bool Busy;
    bool HasCapture;
    uint32_t SampleRateHz;
    uint32_t BinSizeBytes;
    uint32_t TotalSamples;
    uint32_t KernelSamples;
    uint32_t ApplicationSamples;
    uint32_t UnmappedSamples;
    uint32_t SaturatedSamples;
    size_t CounterBytes;
};

PErrorCode kgprof_start();
PErrorCode kgprof_stop() noexcept;
KGProfStatus kgprof_get_status() noexcept;
PErrorCode kgprof_write_gmon(KGProfWriteCallback callback, void* context) noexcept;

// Called by SysTick while normal-latency interrupts are disabled.
void kgprof_record_sample(const KExceptionStackFrame* exceptionFrame) noexcept __attribute__((no_instrument_function));
#endif // PADOS_MODULE_GPROF_SAMPLING

} // namespace kernel
