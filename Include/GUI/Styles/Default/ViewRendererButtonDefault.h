// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererButton.h>
#include <GUI/Styles/Default/ViewRendererButtonDefaultBase.h>

class PViewRendererButtonDefault : public PViewRendererButton, protected PViewRendererButtonDefaultBase
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override { return PViewRendererButtonDefaultBase::Setup(target); }
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context) const override
    {
        PViewRendererButtonDefaultBase::PropertyChanged(target, properties, context);
    }
};
