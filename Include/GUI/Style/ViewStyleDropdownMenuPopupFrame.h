// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererDropdownMenuPopupFrame.h>
#include <GUI/Style/ViewStyle.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleDropdownMenuPopupFrame : public PViewStyle<PViewRendererDropdownMenuPopupFrame>
{
public:
    using PViewStyle::PViewStyle;

    virtual PPoint GetPopupContentSize() const = 0;
};
