// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/GUIDefines.h>
#include <Math/Point.h>
#include <Utils/String.h>

struct PButtonStyleData
{
    PString Label;

    // Cached measurements are maintained by the selected style.
    PPoint LabelSize;
    PAlignment LabelAlignment = PAlignment::Center;
    bool IsCheckable = false;
    bool IsPressed = false;
    bool IsChecked = false;
    bool IsPointerOver = false;
    bool IsLabelSizeValid = false;
};
