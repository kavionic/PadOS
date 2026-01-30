// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/ViewScroller.h>

class PDropdownMenu;

namespace osi
{
class DropdownMenuPopupView;

class DropdownMenuPopupWindow : public PView, public PViewScroller
{
public:
    DropdownMenuPopupWindow(const std::vector<PString>& itemList, size_t selection);
    // From View:
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    void MakeSelectionVisible();

    Signal<void, size_t, bool>  SignalSelectionChanged;
private:
    Ptr<DropdownMenuPopupView> m_ContentView;
};


class DropdownMenuPopupView : public PView
{
public:
    DropdownMenuPopupView(const std::vector<PString>& itemList, size_t selection, Signal<void, size_t, bool>& signalSelectionChanged);

    // From View:
    virtual void    OnPaint(const PRect& updateRect) override;

    virtual bool    OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;

    virtual bool    OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual void    Activated(bool isActive);

    virtual void    CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual PPoint   CalculateContentSize() const override { return m_ContentSize; }

    void MakeSelectionVisible();

    Signal<void, size_t, bool>& SignalSelectionChanged;
private:
    size_t PositionToIndex(const PPoint& position);

    PPoint                   m_ContentSize;
    PFontHeight              m_FontHeight;
    float                   m_GlyphHeight;
    size_t                  m_OldSelection;
    size_t                  m_CurSelection;
    size_t                  m_HitItem = INVALID_INDEX;
    PMouseButton           m_HitButton = PMouseButton::None;
    PPoint                   m_HitPos;
    bool                    m_MouseMoved = false;
    const std::vector<PString>& m_ItemList;
};

} // namespace osi
