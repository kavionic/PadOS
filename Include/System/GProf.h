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

#pragma once

#include <atomic>
#include <stddef.h>
#include <stdint.h>

#ifdef PADOS_MODULE_GPROF_CALL_GRAPH
inline constexpr size_t PGPROF_ARC_CAPACITY = 16384;
inline constexpr size_t PGPROF_ARC_PROBE_LIMIT = 32;

struct PGProfArc
{
    uint32_t CallerPC;
    uint32_t CalleePC;
    uint32_t Count;
};

struct PGProfThreadState
{
    uint32_t Active;
};

struct PGProfCallGraph
{
    PGProfArc* Arcs = nullptr;
    std::atomic<uint32_t>* Enabled;
    std::atomic<bool> DrainRequested = false;
    int32_t DrainCondition = 0;
    uint32_t DroppedCalls = 0;
    uint32_t SaturatedCalls = 0;
    uint32_t UnmappedCalls = 0;
};

extern PGProfCallGraph g_PGProfCallGraph;
extern "C" std::atomic<uint32_t> g_PGProfCallGraphEnabled;

extern "C" PGProfThreadState* p_gprof_get_thread_state() __attribute__((no_instrument_function, target("general-regs-only")));
extern "C" bool p_gprof_record_arc(uint32_t callerReturnPC, uint32_t calleeReturnPC);
#endif // PADOS_MODULE_GPROF_CALL_GRAPH
