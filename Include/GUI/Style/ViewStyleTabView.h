// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/TabViewStyleData.h>
#include <GUI/Style/ViewRendererTabView.h>
#include <GUI/Style/ViewStyle.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleTabView : public PViewStyle<PViewRendererTabView>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PTabViewStyleData& GetTabViewStyleData() const = 0;
    virtual PView& GetTabsView() const = 0;
    virtual void GetTabContentSizeLimits(PPoint* minSize, PPoint* maxSize) const = 0;
};
