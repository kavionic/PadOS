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
// Created: 21.09.2026 00:00

#include <System/GProf.h>


static_assert(std::atomic_ref<uint32_t>::is_always_lock_free);
static_assert(std::atomic<uint32_t>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
static_assert((PGPROF_ARC_CAPACITY & (PGPROF_ARC_CAPACITY - 1)) == 0);
static_assert(PGPROF_ARC_PROBE_LIMIT <= PGPROF_ARC_CAPACITY);
static_assert(sizeof(PGProfArc) == 12);

extern "C"
{
std::atomic<uint32_t> g_PGProfCallGraphEnabled = 0;
}

PGProfCallGraph g_PGProfCallGraph =
{
    .Enabled = &g_PGProfCallGraphEnabled
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline __attribute__((always_inline)) bool PGProfIncrementCounter(uint32_t& value)
{
    std::atomic_ref<uint32_t> counter(value);
    uint32_t previous = counter.load(std::memory_order_relaxed);
    while (previous != UINT32_MAX) {
        if (counter.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed)) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void PGProfCountArc(uint32_t callerPC, uint32_t calleePC)
{
    uint32_t hash = (callerPC >> 1) * 0x9e3779b1u;
    hash ^= (calleePC >> 1) * 0x85ebca6bu;
    hash ^= hash >> 16;
    for (size_t probe = 0; probe < PGPROF_ARC_PROBE_LIMIT; ++probe)
    {
        PGProfArc& arc = g_PGProfCallGraph.Arcs[(hash + probe) & (PGPROF_ARC_CAPACITY - 1)];
        std::atomic_ref<uint32_t> caller(arc.CallerPC);
        std::atomic_ref<uint32_t> callee(arc.CalleePC);
        uint32_t storedCaller = caller.load(std::memory_order_relaxed);
        if (storedCaller == 0 && caller.compare_exchange_strong(storedCaller, callerPC, std::memory_order_relaxed))
        {
            std::atomic_ref<uint32_t>(arc.Count).store(1, std::memory_order_relaxed);
            callee.store(calleePC, std::memory_order_release);
            return;
        }
        // A reserved slot has CalleePC == 0 until its key and count are ready.
        // Skip it instead of waiting for a preempted thread. Duplicate records
        // produced by this race are summed by gprof when reading the dump.
        if (storedCaller == callerPC && callee.load(std::memory_order_acquire) == calleePC)
        {
            if (!PGProfIncrementCounter(arc.Count)) {
                PGProfIncrementCounter(g_PGProfCallGraph.SaturatedCalls);
            }
            return;
        }
    }
    PGProfIncrementCounter(g_PGProfCallGraph.DroppedCalls);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" bool p_gprof_record_arc(uint32_t callerReturnPC, uint32_t calleeReturnPC)
{
    PGProfThreadState* threadState = p_gprof_get_thread_state();
    if (threadState == nullptr) {
        return false;
    }
    std::atomic_ref<uint32_t> active(threadState->Active);
    // Only this thread writes its marker. ISR entries are rejected by mcount,
    // so sampling IRQs can interrupt this code without entering the recorder.
    if (active.load(std::memory_order_relaxed) != 0)
    {
        PGProfIncrementCounter(g_PGProfCallGraph.DroppedCalls);
        return false;
    }
    active.store(1, std::memory_order_relaxed);
    // Keep the store/load fence: stop must see the marker or we must see its
    // closed gate. Stop uses sequentially consistent gate stores and marker loads.
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (g_PGProfCallGraphEnabled.load(std::memory_order_acquire) != 0)
    {
        const uint32_t callerPC = (callerReturnPC & ~uint32_t(1)) - 2;
        const uint32_t calleePC = (calleeReturnPC & ~uint32_t(1)) - 2;
        if (callerPC != 0 && calleePC != 0) {
            PGProfCountArc(callerPC, calleePC);
        } else {
            PGProfIncrementCounter(g_PGProfCallGraph.UnmappedCalls);
        }
    }
    active.store(0, std::memory_order_release);
    return g_PGProfCallGraphEnabled.load(std::memory_order_relaxed) == 0 &&
        g_PGProfCallGraph.DrainRequested.load(std::memory_order_acquire);
}
