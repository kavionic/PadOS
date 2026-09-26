// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 03.09.2026 00:00

#pragma once

#include <stdint.h>

#ifdef PADOS_GPROF_HW_TIMER
#include <System/ErrorCodes.h>
#include <Kernel/HAL/PeripheralMapping.h>

#ifndef STM32H7
#error Hardware profiler sampling requires STM32H7.
#endif

extern "C" void KGProfTimer_Handler() __attribute__((naked, no_instrument_function));
#endif // PADOS_GPROF_HW_TIMER

struct KExceptionStackFrame;

namespace kernel
{

#ifdef PADOS_MODULE_GPROF_SAMPLING
// SysTick samples thread execution; a dedicated timer can also sample lower-priority interrupts.
inline constexpr uint32_t KGPROF_SAMPLE_RATE_HZ = PADOS_GPROF_SAMPLE_RATE_HZ;
inline constexpr uint32_t KGPROF_BIN_SIZE_BYTES = 8;

// Called only by the selected sampling interrupt.
void kgprof_record_sample(const KExceptionStackFrame* exceptionFrame) noexcept __attribute__((no_instrument_function));

#ifdef PADOS_GPROF_HW_TIMER
PErrorCode kgprof_initialize_sampler(uint32_t& sampleRateHz) noexcept;
void kgprof_start_sampler() noexcept;
void kgprof_stop_sampler() noexcept;

// Masks only the sampling IRQ while taking a consistent status snapshot.
class KGProfSamplerIRQGuard
{
public:
    KGProfSamplerIRQGuard() noexcept;
    ~KGProfSamplerIRQGuard();

    KGProfSamplerIRQGuard(const KGProfSamplerIRQGuard&) = delete;
    KGProfSamplerIRQGuard& operator=(const KGProfSamplerIRQGuard&) = delete;

private:
    bool m_WasEnabled = false;
};
#endif // PADOS_GPROF_HW_TIMER
#endif // PADOS_MODULE_GPROF_SAMPLING

} // namespace kernel
