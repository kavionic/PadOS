// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/DropdownMenuStyleData.h>
#include <GUI/Style/ViewRenderer.h>

struct PTextBoxStyle;

class PViewRendererDropdownMenu : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::DropdownMenu;

    virtual void PropertyChanged(PView& target, PDropdownMenuProperties properties, PtrTarget* context) const = 0;
    virtual PRect GetEditorFrame(const PView& target, PtrTarget* context) const = 0;
    virtual Ptr<const PTextBoxStyle> GetEditorStyle(const PView& target, PtrTarget* context) const = 0;
    virtual bool HitTest(const PView& target, const PPoint& position, PtrTarget* context) const = 0;
};
