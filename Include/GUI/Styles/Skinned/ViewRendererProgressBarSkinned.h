// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Style/ViewRendererProgressBar.h>

class PViewRendererProgressBarSkinned : public PViewRendererProgressBar
{
public:
    explicit PViewRendererProgressBarSkinned(const PProgressBarSkin& skin);
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PProgressBarProperties properties, PtrTarget* context) const override;

private:
    struct Context;

    const PProgressBarOrientationSkin& GetProgressBarOrientation(POrientation orientation) const;
    static PRect GetProgressBarFillFrame(const PRect& bounds, const PProgressBarOrientationSkin& skin, const PProgressBarStyleData& data);

    const PProgressBarSkin m_ProgressBarSkin;
};
