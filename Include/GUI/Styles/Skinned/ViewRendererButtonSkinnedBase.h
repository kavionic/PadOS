// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleButtonBase.h>
#include <GUI/Styles/SkinnableGUIStyle.h>

class PViewRendererButtonSkinnedBase
{
protected:
    static Ptr<PtrTarget> Setup(PView& target);
    static void PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context);

    struct Context : PViewRendererContext<PViewStyleButtonBase>
    {
        explicit Context(PView& target) : PViewRendererContext(target) {}

        PPoint LabelSize;
        bool IsLabelSizeValid = false;
    };

    static void UpdateMetrics(const PView& target, const PButtonStyleData& data, Context& context);
    static PButtonSkinState GetButtonState(bool isEnabled, const PButtonStyleData& data);
    static void PaintIndicatorButton(
        PView& target,
        const PRect& frame,
        bool isEnabled,
        const PButtonStyleData& data,
        const PButtonIndicatorSkin& skin,
        const PPoint& indicatorPosition,
        const PPoint& labelPosition,
        bool labelOverlapsIndicator,
        PGUIColorID labelColorID);
    static void PaintOverlappingIndicator(
        PView& target,
        const PRect& frame,
        const PString& label,
        const PPoint& labelPosition,
        const PRect& indicatorFrame,
        PBrush brush);
    static float GetSpacedExtent(float firstSize, float secondSize, float spacing);
};
