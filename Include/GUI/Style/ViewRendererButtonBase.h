// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRenderer.h>
#include <GUI/Style/ButtonStyleData.h>

class PViewRendererButtonBase : public PViewRenderer
{
public:
    virtual void PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context) const = 0;
};
