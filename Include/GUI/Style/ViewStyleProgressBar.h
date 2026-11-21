// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ProgressBarStyleData.h>
#include <GUI/Style/ViewRendererProgressBar.h>
#include <GUI/Style/ViewStyle.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleProgressBar : public PViewStyle<PViewRendererProgressBar>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PProgressBarStyleData& GetProgressBarStyleData() const = 0;
};
