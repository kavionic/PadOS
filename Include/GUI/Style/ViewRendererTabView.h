// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/TabViewStyleData.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererTabView : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::TabView;

    virtual void PaintTabs(PView& tabsTarget, const PView& tabView, const PRect& updateRect, PtrTarget* context) const = 0;
    // Changes are reported after updating the semantic state. Each call may
    // identify at most one inserted, deleted, or renamed tab.
    virtual void PropertyChanged(PView& target, PTabViewProperties properties, size_t tabIndex, PtrTarget* context) const = 0;
    virtual PTabViewFrames GetFrames(const PView& target, PtrTarget* context) const = 0;
    virtual PPoint GetTabsContentSize(const PView& target, PtrTarget* context) const = 0;
    // The position is expressed in the tabs view's scrolled content coordinates.
    virtual size_t GetTabFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const = 0;

protected:
    static PGUIColorID GetLabelColorID(const PTabViewStyleData& data, size_t index)
    {
        const bool selected = index == data.Selection;
        if (index == data.PressedTab) {
            return selected ? PGUIColorID::TabLabelSelectedPressed : PGUIColorID::TabLabelPressed;
        } else if (index == data.HighlightedTab) {
            return selected ? PGUIColorID::TabLabelSelectedHovered : PGUIColorID::TabLabelHovered;
        } else {
            return selected ? PGUIColorID::TabLabelSelectedNormal : PGUIColorID::TabLabelNormal;
        }
    }
};
