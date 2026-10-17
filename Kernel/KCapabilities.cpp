// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 28.02.2026 23:00

#include <Kernel/KCapabilities.h>
#include <Kernel/KProcess.h>

namespace kernel
{


bool kcheck_capability(KCapability capability)
{
    return kcheck_capability(capability, kget_current_process());
}

bool kcheck_capability(KCapability capability, const KProcess& process, bool useEffectiveIDs)
{
    // Placeholder policy until per-process capabilities are implemented.
    const uid_t userID = useEffectiveIDs ? process.GetEUID() : process.GetRUID();
    return userID == 0;
}


} // namespace kernel
