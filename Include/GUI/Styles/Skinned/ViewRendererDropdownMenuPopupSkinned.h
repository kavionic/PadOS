// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Style/ViewRendererDropdownMenuPopup.h>

class PViewRendererDropdownMenuPopupSkinned : public PViewRendererDropdownMenuPopup
{
public:
    explicit PViewRendererDropdownMenuPopupSkinned(const PDropdownMenuSkin& skin);
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PDropdownMenuPopupProperties properties, PtrTarget* context) const override;
    virtual PRect GetItemFrame(const PView& target, size_t index, PtrTarget* context) const override;
    virtual size_t GetItemFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;

    Context& GetDropdownMenuPopupStyleData(const PView& target, const PDropdownMenuPopupStyleData& data, PtrTarget* context) const;
    void PaintDropdownMenuRow(PView& target, const PDropdownMenuPopupStyleData& data, Context& state, size_t index, PtrTarget* context) const;

    const PDropdownMenuSkin m_DropdownMenuSkin;
};
