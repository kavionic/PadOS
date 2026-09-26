// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GUI/View.h"


class PListViewScrolledView;

class PListViewColumnView : public PView
{
public:
    PListViewColumnView(Ptr<PListViewScrolledView> parent, const PString& title);
    ~PListViewColumnView();

    virtual void OnPaint(const PRect& updateRect) override;
    void         Refresh(const PRect& updateRect);
private:
    friend class PListViewHeaderView;
    friend class PListViewScrolledView;

    PString m_Title;
    float   m_ContentWidth;
};
