// This file is part of PadOS.
//
// Copyright (c) 2017-2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once


void SetupSDRAM(uint32_t configReg, uint32_t mode, uint32_t block1Bit, uint32_t refreshNS, uint32_t clkFrequency);
void ShutdownSDRAM();