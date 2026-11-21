// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cassert>
#include <type_traits>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRenderer.h>

// Mix into a PView-derived widget to own its single renderer binding.
class PViewStyleBase
{
public:
    using RendererType = PViewRenderer;

    // Bind the already-constructed PView base; renderer setup is deferred.
    explicit PViewStyleBase(PView* view) : m_View((assert(view != nullptr), *view)) {}
    virtual ~PViewStyleBase();

    PViewStyleBase(const PViewStyleBase&) = delete;
    PViewStyleBase& operator=(const PViewStyleBase&) = delete;

    // PView forwards lifecycle notifications through this interface.
    void HandleStyleChanged();
    void HandleScreenAttachmentChanged(bool attached);
    void HandleVisibilityChanged(bool visible);

    const PViewRenderer* GetRenderer() const;
    PtrTarget* GetRendererContext() const { return ptr_raw_pointer_cast(m_RendererContext); }

protected:
    // Release before the widget's style properties or interface are destroyed.
    void ReleaseRenderer() noexcept;

    // Initialize after the widget's style properties are ready.
    void InitializeRenderer(PGUIComponentID componentID);

private:
    virtual bool IsRendererCompatible(const PViewRenderer* renderer) const = 0;
    void UpdateRenderer();
    void TeardownRenderer() noexcept;

    // Borrowed from the widget containing this mixin.
    PView& m_View;
    Ptr<const PGUIStyle> m_RendererStyle;
    Ptr<const PViewRenderer> m_Renderer;
    Ptr<PtrTarget> m_RendererContext;
    PGUIComponentID m_ComponentID = PGUIComponentID::None;
    bool m_IsUpdatingRenderer = false;
    bool m_IsRendererAttached = false;
    bool m_IsScreenAttached = false;
};

// An existing styled base can specialize its binding without adding a second
// binding. Its renderer interface must remain valid through the base class.
template<class TRendererType, class TBaseType = PViewStyleBase>
class PViewStyle : public TBaseType
{
public:
    using RendererType = TRendererType;
    using TBaseType::TBaseType;

    static_assert(std::is_base_of_v<PViewStyleBase, TBaseType>);
    static_assert(std::is_base_of_v<typename TBaseType::RendererType, RendererType>);

    const RendererType* GetRenderer() const { return static_cast<const RendererType*>(PViewStyleBase::GetRenderer()); }

protected:
    void InitializeRenderer() { PViewStyleBase::InitializeRenderer(RendererType::COMPONENT_ID); }

private:
    virtual bool IsRendererCompatible(const PViewRenderer* renderer) const override { return dynamic_cast<const RendererType*>(renderer) != nullptr; }
};
