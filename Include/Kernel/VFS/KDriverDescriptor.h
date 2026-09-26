// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 04.11.2025 21:00

#pragma once

#include <Ptr/Ptr.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/Kernel.h>

namespace kernel
{

struct KDriverDescriptor
{
    const char* Name;

    bool (*Initialize)(const char* parameters);
};


} // namespace kernel
