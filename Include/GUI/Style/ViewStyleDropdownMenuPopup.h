// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/DropdownMenuPopupStyleData.h>
#include <GUI/Style/ViewRendererDropdownMenuPopup.h>
#include <GUI/Style/ViewStyle.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleDropdownMenuPopup : public PViewStyle<PViewRendererDropdownMenuPopup>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PDropdownMenuPopupStyleData& GetDropdownMenuPopupStyleData() const = 0;
};
