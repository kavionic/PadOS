// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 06.03.2026 20:00

#pragma once

#include <unistd.h>
#include <sys/types.h>

namespace kernel
{

int ksetsid_trw();
void ksetpgid_trw(pid_t inDest, pid_t inGroup);
pid_t kgetpgrp();


} // namespace kernel
