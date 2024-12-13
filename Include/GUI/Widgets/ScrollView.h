// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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

namespace os
{

class ScrollView : public View, public ViewScroller
{
public:
    ScrollView(const String& name = String::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    ScrollView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    // From View:
    virtual void    FrameSized(const Point& delta) override;
    virtual bool    OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;

    // From ViewScroller:
    virtual Ptr<View>   SetScrolledView(Ptr<View> view) override;

private:
    MouseButton_e   m_HitButton = MouseButton_e::None;

};

} // namespace os
