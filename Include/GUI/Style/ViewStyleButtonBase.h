// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewStyle.h>
#include <GUI/Style/ViewRendererButtonBase.h>

// Property references remain owned by the widget; renderer caches belong
// to the context.
class PViewStyleButtonBase : public PViewStyle<PViewRendererButtonBase>
{
public:
    using PViewStyle::PViewStyle;

    virtual const PButtonStyleData& GetButtonStyleData() const = 0;
    virtual bool IsButtonEnabled() const = 0;
};
