// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererTextBox.h>

class PViewRendererTextBoxDefault : public PViewRendererTextBox
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PTextBoxProperties properties, PtrTarget* context) const override;
    virtual PRect GetEditorFrame(const PView& target, PtrTarget* context) const override;
    virtual PPoint GetSizeForContent(const PView& target, const PPoint& contentSize, PtrTarget* context) const override;

private:
    static constexpr float EDITOR_INSET = 4.0f;
};
