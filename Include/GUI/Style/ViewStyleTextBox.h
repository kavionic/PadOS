// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/TextBoxStyleData.h>
#include <GUI/Style/ViewRendererTextBox.h>
#include <GUI/Style/ViewStyle.h>

class PViewStyleTextBox : public PViewStyle<PViewRendererTextBox>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PTextBoxStyleData& GetTextBoxStyleData() const = 0;
};
