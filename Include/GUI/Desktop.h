// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Math/Point.h>

class PDesktop
{
public:
    PPoint GetResolution() const { return PPoint(800.0f, 480.0f); }
};
