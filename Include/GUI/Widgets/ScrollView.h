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
    virtual bool    OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual void    CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    // From ViewScroller:
    virtual Ptr<PView>   SetScrolledView(Ptr<PView> view) override;

private:
    PMouseButton   m_HitButton = PMouseButton::None;

};
