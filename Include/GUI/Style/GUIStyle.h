// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <utility>

#include <GUI/Brush.h>
#include <GUI/Style/ViewRenderer.h>

class PGUIStyle : public PtrTarget
{
public:
    // Hold the owning application mutex when obtaining or using its brush.
    virtual Ptr<const PBrush> GetBackgroundBrush() const = 0;

    static Ptr<const PViewRenderer> CreateDefaultRenderer(PGUIComponentID componentID);

    // Configure renderers before binding views; existing bindings retain their renderer.
    // A null renderer restores lazy default selection for subsequent lookups.
    void SetRenderer(PGUIComponentID componentID, Ptr<const PViewRenderer> renderer);

    // Lookup may populate the cache. Serialize access with the owning application mutex.
    Ptr<const PViewRenderer> GetRenderer(PGUIComponentID componentID) const;

    template<class TRendererType>
    Ptr<const TRendererType> GetRenderer(PGUIComponentID componentID) const
    {
        return ptr_dynamic_cast<const TRendererType>(GetRenderer(componentID));
    }

private:
    mutable std::array<Ptr<const PViewRenderer>, std::to_underlying(PGUIComponentID::Count)> m_Renderers;
};
