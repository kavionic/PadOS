// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewStyleButtonBase.h>
#include <GUI/Style/ViewRendererCheckBox.h>

class PViewStyleCheckBox : public PViewStyle<PViewRendererCheckBox, PViewStyleButtonBase>
{
public:
    using PViewStyle::PViewStyle;
};
