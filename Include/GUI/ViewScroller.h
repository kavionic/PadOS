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
#include <Utils/InertialScroller.h>

class PViewScroller;


namespace osi
{

class ViewScrollerSignalTarget : public SignalTarget
{
public:
    ViewScrollerSignalTarget();

    void SetViewScroller(PViewScroller* viewScroller) { m_ViewScroller = viewScroller; }

    Ptr<PView>   SetScrolledView(Ptr<PView> view);
    Ptr<PView>   GetScrolledView() const { return m_ScrolledView.Lock(); }

    void BeginSwipe(const PPoint& position);
    void SwipeMove(const PPoint& position);
    void EndSwipe();

private:
    friend class ::PViewScroller;

    void UpdateScroller();
    void SlotInertialScrollUpdate(const PPoint& position);

    PInertialScroller    m_InertialScroller;

    PViewScroller* m_ViewScroller;

    WeakPtr<PView> m_ScrolledView;
};

} // namespace osi

class PViewScroller
{
public:
    PViewScroller();

    static PViewScroller* GetViewScroller(PView* view);

    PInertialScroller::State GetInertialScrollerState() const { return m_Handler.m_InertialScroller.GetState(); }

    void  SetMaxHOverscroll(float value)    { m_Handler.m_InertialScroller.SetMaxHOverscroll(value); }
    float GetMaxHOverscroll() const         { return m_Handler.m_InertialScroller.GetMaxHOverscroll(); }

    void  SetMaxVOverscroll(float value)    { m_Handler.m_InertialScroller.SetMaxVOverscroll(value); }
    float GetMaxVOverscroll() const         { return m_Handler.m_InertialScroller.GetMaxVOverscroll(); }

    void  SetMaxOverscroll(float horizontal, float vertical) { m_Handler.m_InertialScroller.SetMaxOverscroll(horizontal, vertical); }

    void    SetStartScrollThreshold(float threshold) { m_Handler.m_InertialScroller.SetStartScrollThreshold(threshold); }
    float   GetStartScrollThreshold() const { return m_Handler.m_InertialScroller.GetStartScrollThreshold(); }

    void BeginSwipe(const PPoint& position)  { m_Handler.BeginSwipe(position); }
    void SwipeMove(const PPoint& position)   { m_Handler.SwipeMove(position); }
    void EndSwipe()                         { m_Handler.EndSwipe(); }

    virtual Ptr<PView>   SetScrolledView(Ptr<PView> view) { return m_Handler.SetScrolledView(view); }
    Ptr<PView>           GetScrolledView() const           { return m_Handler.GetScrolledView(); }

private:
    osi::ViewScrollerSignalTarget  m_Handler;
};
