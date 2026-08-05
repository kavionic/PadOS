// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 29.08.2020 00:30

#pragma once

#include <GUI/View.h>
#include <GUI/ViewScroller.h>


class PScrollView : public PView, public PViewScroller
{
public:
    PScrollView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PScrollView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    // From View:
    virtual void    OnLayoutChanged() override;
    virtual void    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual PPoint  OnPointerWheel(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason) override;
    virtual void    CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    // From ViewScroller:
    virtual Ptr<PView>   SetScrolledView(Ptr<PView> view) override;

private:
    bool CanScrollHorizontally() const;
    bool CanScrollVertically() const;
    bool ShouldCaptureScroll(const PPoint& position) const;
    bool BeginPointerScrollTracking(PPointerID pointerID, const PPoint& position);
    void EndPointerScrollTracking(PPointerID pointerID);

    PPointerID   m_HitPointerID = PInvalidPointerID;
    PPoint       m_HitPos;

};
