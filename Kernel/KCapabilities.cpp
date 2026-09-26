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
    KProcess& process = kget_current_process();

    return process.GetEUID() == 0;
}


} // namespace kernel
