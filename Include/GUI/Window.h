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


class PWindow : public PView
{
public:
    PWindow(const PString& title);

    // From View:
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    virtual bool OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;

    // From Window:
    void SetClient(Ptr<PView> client);
    Ptr<PView> GetClient();

    void Open(PApplication* application = nullptr);
    void Close();

private:
    void SlotClientPreferredSizeChanged();
    Ptr<PView> m_ClientView;

    PRect    m_ClientBorders;

    PString m_Title;

    PMouseButton   m_DragHitButton = PMouseButton::None;
    PPoint           m_DragHitPos;
};
