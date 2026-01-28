// This file is part of PadOS.
//
// Copyright (C) 2021-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.04.2021 15:40

#pragma once

#include <GUI/View.h>

namespace os
{

class Window : public View
{
public:
    Window(const PString& title);

    // From View:
    virtual void OnPaint(const Rect& updateRect) override;
    virtual void OnFrameSized(const Point& delta) override;
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;

    virtual bool OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;

    // From Window:
    void SetClient(Ptr<View> client);
    Ptr<View> GetClient();

    void Open(Application* application = nullptr);
    void Close();

private:
    void SlotClientPreferredSizeChanged();
    Ptr<View> m_ClientView;

    Rect    m_ClientBorders;

    PString m_Title;

    MouseButton_e   m_DragHitButton = MouseButton_e::None;
    Point           m_DragHitPos;
};

} // namespace os
