// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewStyle.h>
#include <GUI/Style/ViewRendererDropdownMenu.h>
#include <GUI/Style/DropdownMenuStyleData.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleDropdownMenu : public PViewStyle<PViewRendererDropdownMenu>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PDropdownMenuStyleData& GetDropdownMenuStyleData() const = 0;
};
