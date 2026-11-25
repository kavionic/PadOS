// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/GUIStyleInfo.h>
#include <GUI/Widgets/BitmapView.h>
#include <GUI/Widgets/GroupView.h>
#include <GUI/Widgets/TextView.h>

namespace guistylepreferences
{

struct GUIStyleVariantItemData : PtrTarget
{
    PString StyleName;
    PGUIStyleVariantInfo Variant;
};

class GUIStyleVariantItemView : public PGroupView
{
public:
    GUIStyleVariantItemView();
    virtual void OnStyleChanged() override;
    virtual void AttachedToScreen() override;
    virtual void DetachedFromScreen() override;

    void SetData(Ptr<const GUIStyleVariantItemData> item, bool isSelected, bool isHighlighted);
    void UpdateSelection(bool isSelected, bool isHighlighted);

private:
    void UpdatePreview();

    Ptr<const GUIStyleVariantItemData> m_Item;
    Ptr<PBitmapView> m_PreviewView;
    Ptr<PTextView> m_NameView;
    bool m_IsSelected = false;
    bool m_IsHighlighted = false;
};

} // namespace guistylepreferences
