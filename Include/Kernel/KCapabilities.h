// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 28.02.2026 23:00

#pragma once

#include <stdint.h>

namespace kernel
{


class KProcess;

enum class KCapability : int32_t
{
    CAP_KILL = 0,
    CAP_DAC_OVERRIDE,
    CAP_SETUID,
    CAP_SETGID
};

// Callers hold g_PIDMapMutex, as when reading the process credentials directly.
bool kcheck_capability(KCapability capability);
bool kcheck_capability(KCapability capability, const KProcess& process, bool useEffectiveIDs = true);


} // namespace kernel
