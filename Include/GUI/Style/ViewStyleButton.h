// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererButton.h>
#include <GUI/Style/ViewStyleButtonBase.h>

class PViewStyleButton : public PViewStyle<PViewRendererButton, PViewStyleButtonBase>
{
public:
    using PViewStyle::PViewStyle;
};
