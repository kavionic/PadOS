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


enum class KCapability : int32_t
{
    CAP_KILL = 0
};

bool kcheck_capability(KCapability capability);


} // namespace kernel
