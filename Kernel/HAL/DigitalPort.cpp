// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18/08/26 23:04:19

#include "Kernel/HAL/DigitalPort.h"

DigitalPort::IntMaskAcc DigitalPort::s_IntMaskAccumulators[e_DigitalPortID_Count];
