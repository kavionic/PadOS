// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewStyle.h>
#include <GUI/Style/ViewRendererGroupView.h>
#include <GUI/Style/GroupViewStyleData.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleGroupView : public PViewStyle<PViewRendererGroupView>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PGroupViewStyleData& GetGroupViewStyleData() const = 0;
};
