// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.12.2025 17:30

#pragma once

#include <sys/pados_threads.h>

struct PThreadUserData;

void __process_entry_trampoline(PThreadUserData* threadData, ThreadEntryPoint_t threadEntry, void* arguments);
