// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererDropdownMenu.h>
#include <GUI/Styles/SkinnableGUIStyle.h>

class PViewRendererDropdownMenuSkinned : public PViewRendererDropdownMenu
{
public:
    explicit PViewRendererDropdownMenuSkinned(const PDropdownMenuSkin& skin);
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PDropdownMenuProperties properties, PtrTarget* context) const override;
    virtual PRect GetEditorFrame(const PView& target, PtrTarget* context) const override;
    virtual bool HitTest(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;

    static PButtonSkinState GetDropdownMenuState(const PDropdownMenuStyleData& data);
    const PDropdownMenuAppearance& GetDropdownMenuAppearance(const PDropdownMenuStyleData& data) const;
    PRect GetDropdownMenuArrowFrame(const PRect& frame) const;
    static Context& GetDropdownMenuStyleData(const PView& target, const PDropdownMenuStyleData& data, PtrTarget* context);

    const PDropdownMenuSkin m_DropdownMenuSkin;
};
