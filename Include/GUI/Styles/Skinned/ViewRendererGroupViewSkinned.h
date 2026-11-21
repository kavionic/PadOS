// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Style/ViewRendererGroupView.h>

class PViewRendererGroupViewSkinned : public PViewRendererGroupView
{
public:
    explicit PViewRendererGroupViewSkinned(const PGroupViewSkin& skin);
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& componentFrame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PGroupViewProperties properties, PtrTarget* context) const override;

private:
    struct Context;

    Context& GetGroupViewStyleData(const PView& target, const PGroupViewStyleData& data, PtrTarget* context) const;
    void PaintGroupViewFrame(PView& target, const PRect& updateRect, const PRect& frame, Context& state) const;

    const PGroupViewSkin m_GroupViewSkin;
};
