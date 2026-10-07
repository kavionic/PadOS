// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Kernel/Startup/KMemoryStartup.h>

namespace kernel
{

// A dry run validates the complete map and hardware region budget without changing hardware state.
KMemorySetupResult kconfigure_memory_protection(const PMemoryRegionTable& table, bool apply);

} // namespace kernel
