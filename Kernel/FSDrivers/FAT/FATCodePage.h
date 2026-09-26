// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 26/08/27

#pragma once

#include <stdint.h>

namespace kernel
{

uint32_t CP437ToUnicode(uint8_t character);
bool UnicodeToCP437(uint32_t unicode, uint8_t* result);
uint8_t CP437CharacterToLower(uint8_t character);
uint8_t CP437CharacterToUpper(uint8_t character);

} // namespace kernel

