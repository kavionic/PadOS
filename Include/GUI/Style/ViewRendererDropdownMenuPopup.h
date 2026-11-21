// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/DropdownMenuPopupStyleData.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererDropdownMenuPopup : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::DropdownMenuPopup;

    virtual void PropertyChanged(PView& target, PDropdownMenuPopupProperties properties, PtrTarget* context) const = 0;
    virtual PRect GetItemFrame(const PView& target, size_t index, PtrTarget* context) const = 0;
    virtual size_t GetItemFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const = 0;
};
