// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/GUIStyleData.h>
#include <Utils/EnumBitmask.h>
#include <Utils/String.h>

enum class PGroupViewProperty : uint32_t
{
    Label = 0x01,
    Font  = 0x02,
    Frame = 0x04
};

using PGroupViewProperties = PEnumBitmask<PGroupViewProperty>;

struct PGroupViewStyleData : PGUIStyleData
{
    PString Label;
};
