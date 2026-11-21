// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ProgressBarStyleData.h>
#include <GUI/Style/ViewRenderer.h>

class PViewRendererProgressBar : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::ProgressBar;

    virtual void PropertyChanged(PView& target, PProgressBarProperties properties, PtrTarget* context) const = 0;
};
