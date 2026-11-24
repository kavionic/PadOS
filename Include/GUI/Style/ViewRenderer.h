// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/View.h>

enum class PGUIComponentID : uint8_t
{
    None,
    Button,
    CheckBox,
    RadioButton,
    Slider,
    ScrollBar,
    DropdownMenu,
    DropdownMenuPopupFrame,
    DropdownMenuPopup,
    GroupView,
    ProgressBar,
    TabView,
    TextBox,
    Count
};

// Renderer instances are immutable and shared. Each binding owns its context.
class PViewRenderer : public PtrTarget
{
public:
    // Setup runs with the target interface initialized. It must roll back any
    // subscriptions if it throws. A successful setup may return a null context.
    virtual Ptr<PtrTarget> Setup(PView& target) const = 0;
    // The target interface and context remain valid throughout teardown.
    virtual void Teardown(PView& target, PtrTarget* context) const noexcept {}

    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const = 0;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const = 0;

    // A failed attachment is paired with DetachedFromScreen to release any
    // resources acquired before the failure. Ordinary detachment keeps context.
    virtual void AttachedToScreen(PView& target, PtrTarget* context) const {}
    virtual void DetachedFromScreen(PView& target, PtrTarget* context) const noexcept {}
    virtual void VisibilityChanged(PView& target, bool visible, PtrTarget* context) const {}
};
