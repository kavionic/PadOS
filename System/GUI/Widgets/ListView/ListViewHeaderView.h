// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GUI/View.h"

class PListView;
class PListViewScrolledView;

class PListViewHeaderView : public PView
{
public:
    friend class PListView;
    friend class PListViewRow;
    friend class PListViewColumnView;

    PListViewHeaderView(Ptr<PListView> parent);

private:
    void  DrawButton(const char* title, const PRect& frame, Ptr<PFont> font, PFontHeight* fontHeight);
    virtual void    OnPaint(const PRect& updateRect) override;

    virtual void    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnFrameSized(const PPoint& deltaSize) override;
    virtual void    OnViewScrolled(const PPoint& delta) override;

    virtual bool    HasPointerCapture(PPointerID pointerID) const override;

    void    Layout();

    Ptr<PListViewScrolledView>  m_ScrolledContainerView;

    size_t                  m_SizeColumn = INVALID_INDEX;
    size_t                  m_DragColumn = INVALID_INDEX;
    PPoint                   m_HitPos;
    float                   m_HeaderHeight;
};
