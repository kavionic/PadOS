// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererProgressBar.h>

class PViewRendererProgressBarDefault : public PViewRendererProgressBar
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PProgressBarProperties properties, PtrTarget* context) const override;

private:
    struct Context;
    static constexpr float PROGRESS_BAR_MIN_LENGTH = 50.0f;
    static constexpr float PROGRESS_BAR_THICKNESS = 24.0f;
    static constexpr float PROGRESS_BAR_BORDER = 2.0f;
    static void InvalidateProgressBarResize(PView& target, const PProgressBarStyleData& data, const PRect& previousFrame);
};
