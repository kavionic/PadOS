// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererButtonBase.h>

class PViewRendererCheckBox : public PViewRendererButtonBase
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::CheckBox;
};
