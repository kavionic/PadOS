// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererCheckBox.h>
#include <GUI/Styles/Skinned/ViewRendererButtonSkinnedBase.h>

class PViewRendererCheckBoxSkinned : public PViewRendererCheckBox, protected PViewRendererButtonSkinnedBase
{
public:
    explicit PViewRendererCheckBoxSkinned(const PCheckBoxSkin& skin);
    virtual Ptr<PtrTarget> Setup(PView& target) const override { return PViewRendererButtonSkinnedBase::Setup(target); }
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context) const override
    {
        PViewRendererButtonSkinnedBase::PropertyChanged(target, properties, context);
    }

private:
    const PCheckBoxSkin m_CheckBoxSkin;
};
