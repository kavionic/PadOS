// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRenderer.h>

class PViewRendererDropdownMenuPopupFrame : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::DropdownMenuPopupFrame;

    virtual PRect GetContentFrame(const PView& target, PtrTarget* context) const = 0;
};
