// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/ViewStyle.h>
#include <System/ExceptionHandling.h>
#include <GUI/Style/ViewRenderer.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewStyleBase::~PViewStyleBase()
{
    // The implementing widget must release while its style interface is alive.
    assert(m_ComponentID == PGUIComponentID::None && m_Renderer == nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewStyleBase::HandleStyleChanged()
{
    if (m_ComponentID != PGUIComponentID::None)
    {
        UpdateRenderer();
        m_View.PreferredSizeChanged();
        m_View.Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewStyleBase::HandleScreenAttachmentChanged(bool attached)
{
    m_IsScreenAttached = attached;
    if (attached)
    {
        UpdateRenderer();
        if (m_Renderer != nullptr && !m_IsRendererAttached)
        {
            PScopeFail rollback([this]() { TeardownRenderer(); });
            m_IsRendererAttached = true;
            m_Renderer->AttachedToScreen(m_View, GetRendererContext());
        }
    }
    else if (m_IsRendererAttached)
    {
        m_IsRendererAttached = false;
        m_Renderer->DetachedFromScreen(m_View, GetRendererContext());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewStyleBase::HandleVisibilityChanged(bool visible)
{
    const PViewRenderer* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->VisibilityChanged(m_View, visible, GetRendererContext());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PViewRenderer* PViewStyleBase::GetRenderer() const
{
    const_cast<PViewStyleBase*>(this)->UpdateRenderer();
    return ptr_raw_pointer_cast(m_Renderer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewStyleBase::ReleaseRenderer() noexcept
{
    m_ComponentID = PGUIComponentID::None;
    TeardownRenderer();
    m_IsScreenAttached = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewStyleBase::InitializeRenderer(PGUIComponentID componentID)
{
    assert(m_ComponentID == PGUIComponentID::None);
    m_ComponentID = componentID;
    m_IsScreenAttached = m_View.HasFlags(PViewFlag::IsAttachedToScreen);
    PScopeFail rollback([this]() { ReleaseRenderer(); });
    UpdateRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewStyleBase::UpdateRenderer()
{
    if (m_ComponentID == PGUIComponentID::None || m_IsUpdatingRenderer) {
        return;
    }
    const PGUIStyle* selectedStyle = m_View.GetStyle();
    if (m_RendererStyle == selectedStyle) {
        return;
    }

    const Ptr<const PGUIStyle> nextStyle = ptr_tmp_cast(selectedStyle);
    Ptr<const PViewRenderer> nextRenderer;
    if (nextStyle != nullptr)
    {
        nextRenderer = nextStyle->GetRenderer(m_ComponentID);
        if (nextRenderer == nullptr || !IsRendererCompatible(ptr_raw_pointer_cast(nextRenderer))) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "GUI style provides an incompatible component renderer");
        }
    }

    m_IsUpdatingRenderer = true;
    PScopeExit finishUpdate([this]() { m_IsUpdatingRenderer = false; });
    TeardownRenderer();
    if (nextRenderer != nullptr)
    {
        Ptr<PtrTarget> context = nextRenderer->Setup(m_View);
        m_RendererStyle = nextStyle;
        m_Renderer = std::move(nextRenderer);
        m_RendererContext = std::move(context);
        PScopeFail rollback([this]() { TeardownRenderer(); });
        if (m_IsScreenAttached)
        {
            m_IsRendererAttached = true;
            m_Renderer->AttachedToScreen(m_View, GetRendererContext());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewStyleBase::TeardownRenderer() noexcept
{
    const Ptr<const PGUIStyle> previousStyle = std::move(m_RendererStyle);
    const Ptr<const PViewRenderer> renderer = std::move(m_Renderer);
    const Ptr<PtrTarget> context = std::move(m_RendererContext);
    const bool wasAttached = m_IsRendererAttached;
    m_IsRendererAttached = false;
    if (renderer != nullptr)
    {
        if (wasAttached) {
            renderer->DetachedFromScreen(m_View, ptr_raw_pointer_cast(context));
        }
        renderer->Teardown(m_View, ptr_raw_pointer_cast(context));
    }
}
