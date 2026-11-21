// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererGroupView.h>

class PViewRendererGroupViewDefault : public PViewRendererGroupView
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PGroupViewProperties properties, PtrTarget* context) const override;

private:
    struct Context;
    static constexpr float GROUP_VIEW_PEN_WIDTH = 2.0f;
    static constexpr float GROUP_VIEW_LABEL_OFFSET = 30.0f;
    static constexpr float GROUP_VIEW_LABEL_GAP_START = 15.0f;
    static constexpr float GROUP_VIEW_LABEL_SPACING = 10.0f;
    static constexpr float GROUP_VIEW_MIN_SIZE = 5.0f;
    static void UpdateGroupViewMetrics(const PView& target, const PGroupViewStyleData& data, Context& state);
};
