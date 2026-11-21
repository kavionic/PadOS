// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleButtonBase.h>

class PViewRendererButtonDefaultBase
{
protected:
    static Ptr<PtrTarget> Setup(PView& target);
    static void PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context);

    struct Context : PViewRendererContext<PViewStyleButtonBase>
    {
        using PViewRendererContext<PViewStyleButtonBase>::PViewRendererContext;

        PPoint LabelSize;
        bool IsLabelSizeValid = false;
    };

    static void UpdateMetrics(const PView& target, const PButtonStyleData& data, Context& state);
};
