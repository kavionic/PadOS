// This file is part of PadOS.
//
// Copyright (c) 2021-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

    virtual void OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;

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

    PPointerID   m_DragHitPointerID = PInvalidPointerID;
    PPoint           m_DragHitPos;
};
