// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/View.h>

// The binding must be torn down before the view style interface is destroyed.
// These borrowed references do not keep the view alive.
template<class TViewStyleType>
class PViewRendererContext : public PtrTarget
{
public:
    explicit PViewRendererContext(PView& target)
        : m_View(target), m_Target(dynamic_cast<const TViewStyleType&>(target)) {}

    PView& GetView() const { return m_View; }
    const TViewStyleType& GetTarget() const { return m_Target; }

private:
    PView& m_View;
    const TViewStyleType& m_Target;
};
