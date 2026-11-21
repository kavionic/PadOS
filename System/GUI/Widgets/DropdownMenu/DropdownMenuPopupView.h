// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/DropdownMenuPopupStyleData.h>
#include <GUI/View.h>
#include <GUI/ViewScroller.h>
#include <GUI/Style/ViewStyleDropdownMenuPopup.h>
#include <GUI/Style/ViewStyleDropdownMenuPopupFrame.h>

class PDropdownMenu;

namespace osi
{
class DropdownMenuPopupView;

class DropdownMenuPopupWindow : public PView, public PViewScroller, public PViewStyleDropdownMenuPopupFrame
{
public:
    DropdownMenuPopupWindow(const std::vector<PString>& itemList, size_t selection);
    virtual ~DropdownMenuPopupWindow() override;

    virtual PPoint GetPopupContentSize() const override;
    // From View:
    virtual void OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void OnStyleChanged() override;
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    void MakeSelectionVisible();
    void ItemsChanged();

    Signal<void, size_t, bool>  SignalSelectionChanged;
private:
    Ptr<DropdownMenuPopupView> m_ContentView;
};


class DropdownMenuPopupView : public PView, public PViewStyleDropdownMenuPopup
{
public:
    DropdownMenuPopupView(const std::vector<PString>& itemList, size_t selection, Signal<void, size_t, bool>& signalSelectionChanged);
    virtual ~DropdownMenuPopupView() override;

    virtual const PDropdownMenuPopupStyleData& GetDropdownMenuPopupStyleData() const override { return m_PopupStyleData; }

    // From View:
    virtual void    OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void    OnStyleChanged() override;
    virtual void    OnFrameSized(const PPoint& delta) override;
    virtual void    OnPaint(const PRect& updateRect) override;

    virtual void    Activated(bool isActive);

    virtual void    CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual PPoint   CalculateContentSize() const override;

    virtual void    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;

    void MakeSelectionVisible();
    void ItemsChanged();

    Signal<void, size_t, bool>& SignalSelectionChanged;
private:
    void NotifyStylePropertyChanged(PDropdownMenuPopupProperties properties);
    size_t PositionToIndex(const PPoint& position);

    PDropdownMenuPopupStyleData m_PopupStyleData;
    size_t                  m_OldSelection;
    PPointerID           m_HitPointerID = PInvalidPointerID;
    PPoint                   m_HitPos;
    bool                    m_MouseMoved = false;
};

} // namespace osi
