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
// Created: 23.09.2026

#include <algorithm>
#include <limits>

#include <Kernel/Profiler/KGProfSampler.h>

namespace kernel
{

static_assert(PADOS_GPROF_IRQ_PRIORITY < (1u << __NVIC_PRIO_BITS));

static TIM_TypeDef* g_KGProfTimer = nullptr;
static IRQn_Type g_KGProfTimerIRQ = IRQn_Type(IRQ_COUNT);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void KGProfConfigureTimer(TIM_TypeDef* timer, uint64_t prescaler, uint64_t period)
{
    timer->CR1 = TIM_CR1_URS | TIM_CR1_ARPE;
    if (IS_TIM_MASTER_INSTANCE(timer) || IS_TIM_REPETITION_COUNTER_INSTANCE(timer)) {
        timer->CR2 = 0;
    }
    if (IS_TIM_SLAVE_INSTANCE(timer)) {
        timer->SMCR = 0;
    }
    if (IS_TIM_CC1_INSTANCE(timer)) {
        timer->CCER = 0;
    }
    if (IS_TIM_REPETITION_COUNTER_INSTANCE(timer)) {
        timer->RCR = 0;
    }
    timer->PSC = uint32_t(prescaler - 1);
    timer->ARR = uint32_t(period - 1);
    timer->EGR = TIM_EGR_UG;
    timer->SR = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kgprof_initialize_sampler(uint32_t& sampleRateHz) noexcept
{
    TIM_TypeDef* const timer = get_timer_from_id(PADOS_GPROF_TIMER_ID);
    const IRQn_Type timerIRQ = get_timer_irq(PADOS_GPROF_TIMER_ID, HWTimerIRQType::Update);
    const uint64_t clockFrequency = get_timer_int_clock_freq(PADOS_GPROF_TIMER_ID);
    const uint64_t maxPrescaler = uint64_t(TIM_PSC_PSC_Msk) + 1;
    const uint64_t maxPeriod = IS_TIM_32B_COUNTER_INSTANCE(timer) ?
        uint64_t(std::numeric_limits<uint32_t>::max()) + 1 : uint64_t(std::numeric_limits<uint16_t>::max()) + 1;
    const uint64_t requestedRate = KGPROF_SAMPLE_RATE_HZ;
    const uint64_t prescaler = std::max<uint64_t>(
        1,
        (clockFrequency + requestedRate * maxPeriod - 1) / (requestedRate * maxPeriod));
    const uint64_t period = (clockFrequency + prescaler * requestedRate / 2) / (prescaler * requestedRate);

    if (timer == nullptr || timerIRQ == IRQ_COUNT || clockFrequency == 0 ||
        prescaler > maxPrescaler || period == 0 || period > maxPeriod) {
        return PErrorCode::INVAL;
    }
    if (!enable_timer_clock(PADOS_GPROF_TIMER_ID)) {
        return PErrorCode::INVAL;
    }
    if ((timer->CR1 & TIM_CR1_CEN) != 0 || timer->DIER != 0) {
        return PErrorCode::BUSY;
    }

    NVIC_DisableIRQ(timerIRQ);
    KGProfConfigureTimer(timer, prescaler, period);

    uint32_t debugFlagMask = 0;
    volatile uint32_t* const debugRegister = get_timer_dbg_clk_flag(PADOS_GPROF_TIMER_ID, debugFlagMask);
    if (debugRegister != nullptr) {
        *debugRegister |= debugFlagMask;
    }
    NVIC_SetPriority(timerIRQ, PADOS_GPROF_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(timerIRQ);
    g_KGProfTimer = timer;
    g_KGProfTimerIRQ = timerIRQ;
    const uint64_t divisor = prescaler * period;
    sampleRateHz = uint32_t((clockFrequency + divisor / 2) / divisor);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kgprof_start_sampler() noexcept
{
    g_KGProfTimer->EGR = TIM_EGR_UG;
    g_KGProfTimer->SR = 0;
    NVIC_ClearPendingIRQ(g_KGProfTimerIRQ);
    g_KGProfTimer->DIER = TIM_DIER_UIE;
    NVIC_EnableIRQ(g_KGProfTimerIRQ);
    g_KGProfTimer->CR1 |= TIM_CR1_CEN;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kgprof_stop_sampler() noexcept
{
    if (g_KGProfTimer != nullptr)
    {
        NVIC_DisableIRQ(g_KGProfTimerIRQ);
        g_KGProfTimer->DIER = 0;
        g_KGProfTimer->CR1 &= ~TIM_CR1_CEN;
        g_KGProfTimer->SR = 0;
        NVIC_ClearPendingIRQ(g_KGProfTimerIRQ);
        __DSB();
        __ISB();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KGProfSamplerIRQGuard::KGProfSamplerIRQGuard() noexcept
{
    if (g_KGProfTimer != nullptr)
    {
        m_WasEnabled = NVIC_GetEnableIRQ(g_KGProfTimerIRQ) != 0;
        NVIC_DisableIRQ(g_KGProfTimerIRQ);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KGProfSamplerIRQGuard::~KGProfSamplerIRQGuard()
{
    if (m_WasEnabled) {
        NVIC_EnableIRQ(g_KGProfTimerIRQ);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((used, no_instrument_function)) void KGProfTimer_HandlerImpl(const KExceptionStackFrame* exceptionFrame)
{
    if ((g_KGProfTimer->SR & TIM_SR_UIF) != 0)
    {
        g_KGProfTimer->SR = ~TIM_SR_UIF;
        kgprof_record_sample(exceptionFrame);
    }
    __DSB();
}

} // namespace kernel

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((naked, no_instrument_function)) void KGProfTimer_Handler()
{
    // Capture MSP/PSP before a C++ prologue changes the handler stack.
    __asm volatile
    (
        "tst     lr, #4\n"
        "ite     eq\n"
        "mrseq   r0, msp\n"
        "mrsne   r0, psp\n"
        "b       KGProfTimer_HandlerImpl\n"
    );
}
