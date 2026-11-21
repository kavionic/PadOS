// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererCheckBox.h>
#include <GUI/Styles/Default/ViewRendererButtonDefaultBase.h>

class PViewRendererCheckBoxDefault : public PViewRendererCheckBox, protected PViewRendererButtonDefaultBase
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override { return PViewRendererButtonDefaultBase::Setup(target); }
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context) const override
    {
        PViewRendererButtonDefaultBase::PropertyChanged(target, properties, context);
    }

private:
    static constexpr float CHECK_BOX_SIZE = 32.0f;
    static constexpr float CHECK_BOX_LABEL_SPACING = 8.0f;
    static constexpr float CHECK_BOX_CHECK_BORDER = 7.0f;
};
