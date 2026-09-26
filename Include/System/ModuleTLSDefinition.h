// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 25.11.2025 01:00

#pragma once

#include <stdint.h>

struct PModuleTLSDefinition
{
	void* TLSData;
	void* TLSBSS;

	uint32_t TLSDataSize;
	uint32_t TLSBSSSize;
	uint32_t TLSAlign;
};
