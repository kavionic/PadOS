// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Style/ViewRendererScrollBar.h>
#include <GUI/Style/ViewStyle.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleScrollBar : public PViewStyle<PViewRendererScrollBar>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PScrollBarStyleData& GetScrollBarStyleData() const = 0;
};
