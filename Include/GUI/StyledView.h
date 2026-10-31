// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/View.h>

class PStyledView : public PView
{
public:
    using PView::PView;

    void OnStyleChanged() override;
};
