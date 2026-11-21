// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRenderer.h>
#include <GUI/Style/GroupViewStyleData.h>

class PViewRendererGroupView : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::GroupView;

    // Preferred-size results describe the frame and label. The widget combines
    // them with its child layout, respecting its width/height inclusion flags.
    virtual void PropertyChanged(PView& target, PGroupViewProperties properties, PtrTarget* context) const = 0;
};
