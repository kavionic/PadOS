// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 17.03.2018 20:30:19
 
#pragma once


enum class PMessageID : uint32_t
{
    NONE = 0,
    FIRST_SIGNAL_ID = std::numeric_limits<int32_t>::max() - 200000,
    FIRST_SYSTEM_ID = std::numeric_limits<int32_t>::max() - 100000,
    QUIT
};
