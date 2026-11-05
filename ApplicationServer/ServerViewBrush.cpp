// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <ApplicationServer/DisplayDriver.h>
#include <GUI/Region.h>
#include <System/ExceptionHandling.h>
#include "ServerView.h"

uint64_t PServerView::s_BackgroundGeneration = 1;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetBrush(bool background, PBrushSource source, Ptr<PServerBrush> brush, const PRect& bounds, bool automaticBounds)
{
    if ((source != PBrushSource::Brush && source != PBrushSource::Inherit)
        || ((source == PBrushSource::Brush) != (brush != nullptr))) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PServerBrushBinding& binding = background ? m_BackgroundBrush : m_DrawingBrush;
    if (binding.Source != source || binding.Brush != brush || binding.Bounds != bounds || binding.AutomaticBounds != automaticBounds)
    {
        if (background && binding.Brush != brush)
        {
            if (brush != nullptr) {
                brush->SignalChanged.Connect(this, &PServerView::BackgroundBrushChanged);
            }
            if (binding.Brush != nullptr) {
                binding.Brush->SignalChanged.Disconnect(this, &PServerView::BackgroundBrushChanged);
            }
        }
        binding = {std::move(brush), bounds, source, automaticBounds};
        if (background) {
            BackgroundBrushChanged();
        } else {
            m_DrawingCacheGeneration = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::EraseRect(const PRect& rect, bool drawingBackground)
{
    const PServerBrushState& state = drawingBackground ? GetDrawingBrushState() : GetBackgroundBrushState();
    if (state.IsSolid)
    {
        FillRect(rect, state.Color);
        return;
    }
    const Ptr<const PRegion> region = GetRegion();
    if (region != nullptr)
    {
        m_Bitmap->m_Driver->SetBrush(state.Layers);
        const PIPoint screenPosition(m_ScreenPos);
        const PIRect scrolled(rect + m_ScrollOffset);
        for (const PIRect& clip : region->m_Rects)
        {
            const PIRect clipped = scrolled & clip;
            if (clipped.IsValid()) {
                m_Bitmap->m_Driver->FillBrushRect(m_Bitmap, clipped + screenPosition);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Resolve only when geometry, ancestry, or a contributing brush changes.
/// Opaque solids stop immediately, with no layer allocation or ancestor
/// traversal.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PServerBrushState& PServerView::GetBackgroundBrushState() const
{
    if (m_BackgroundCacheGeneration != s_BackgroundGeneration)
    {
        const PServerBrushState* base = nullptr;
        if (!BrushCoversView(m_BackgroundBrush))
        {
            const Ptr<const PServerView> parent = GetParent();
            if (parent != nullptr) {
                base = &parent->GetBackgroundBrushState();
            }
        }
        ResolveBrushState(m_BackgroundBrushState, m_BackgroundBrush, base);
        m_BackgroundCacheGeneration = s_BackgroundGeneration;
    }
    return m_BackgroundBrushState;
}

///////////////////////////////////////////////////////////////////////////////
/// Drawing-only edits do not invalidate views. The resource revision refreshes
/// this cached composition at the next draw, in command-queue order.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PServerBrushState& PServerView::GetDrawingBrushState() const
{
    if (m_DrawingBrush.Source == PBrushSource::Inherit) {
        return GetBackgroundBrushState();
    }
    const uint64_t revision = m_DrawingBrush.Brush->GetRevision();
    if (m_DrawingCacheGeneration != s_BackgroundGeneration || m_DrawingCacheRevision != revision)
    {
        const PServerBrushState* base = BrushCoversView(m_DrawingBrush) ? nullptr : &GetBackgroundBrushState();
        ResolveBrushState(m_DrawingBrushState, m_DrawingBrush, base);
        m_DrawingCacheGeneration = s_BackgroundGeneration;
        m_DrawingCacheRevision = revision;
    }
    return m_DrawingBrushState;
}

///////////////////////////////////////////////////////////////////////////////
/// Color-only primitive fills preserve the selected solid's alpha (including
/// alpha zero, which disables ellipse/pie fills). Text uses resolved backgrounds.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PServerView::GetDrawingColor() const
{
    if (m_DrawingBrush.Brush != nullptr && m_DrawingBrush.Brush->GetDescription().Type == PBrushType::Solid) {
        return m_DrawingBrush.Brush->GetDescription().Color;
    }
    const PServerBrushState& state = GetDrawingBrushState();
    if (!state.IsSolid) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return state.Color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::BrushCoversView(const PServerBrushBinding& binding) const
{
    if (binding.Brush == nullptr) {
        return false;
    }
    const PBrushDescription& description = binding.Brush->GetDescription();
    if (!description.IsOpaque) {
        return false;
    }
    if (description.Type == PBrushType::Solid || description.Type == PBrushType::BitmapTile) {
        return true;
    }
    const PRect placement = binding.AutomaticBounds ? GetNormalizedBounds() : binding.Bounds;
    const PIRect coverage(placement + m_ScrollOffset);
    const PIRect footprint = GetNormalizedIBounds();
    return (coverage & footprint) == footprint;
}

///////////////////////////////////////////////////////////////////////////////
/// Collapse constant layers once. Patterned compositions retain resource
/// identity and revision alongside their original screen-space sampling
/// rectangles.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ResolveBrushState(PServerBrushState& state, const PServerBrushBinding& binding, const PServerBrushState* base) const
{
    state.Layers.clear();
    state.Color = (base != nullptr) ? base->Color : PColor(255, 255, 255);
    state.IsSolid = base == nullptr || base->IsSolid;
    if (binding.Brush == nullptr)
    {
        if (base != nullptr && !base->IsSolid) {
            state.Layers = base->Layers;
        }
        return;
    }
    const PBrushDescription& description = binding.Brush->GetDescription();
    if (description.Type == PBrushType::Solid && state.IsSolid)
    {
        state.Color = PBrushUtils::Composite(description.Color, state.Color);
        return;
    }
    if (base != nullptr)
    {
        if (base->IsSolid)
        {
            PServerBrushLayer constant;
            constant.Description.Color = base->Color;
            state.Layers.push_back(constant);
        }
        else
        {
            state.Layers = base->Layers;
        }
    }
    else if (!description.IsOpaque)
    {
        PServerBrushLayer constant;
        constant.Description.Color = state.Color;
        state.Layers.push_back(constant);
    }
    const PRect placement = binding.AutomaticBounds ? GetNormalizedBounds() : binding.Bounds;
    state.Layers.push_back({binding.Brush, description, PIRect(placement + m_ScrollOffset) + PIPoint(m_ScreenPos), binding.Brush->GetRevision()});
    state.IsSolid = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BackgroundBrushChanged()
{
    ++s_BackgroundGeneration;
    if (m_BackgroundBrush.Brush == nullptr || m_BackgroundBrush.Brush->GetDescription().Type != PBrushType::Solid
        || !m_BackgroundBrush.Brush->GetDescription().IsOpaque) {
        MarkBackgroundBrushes();
    }
    InvalidateBackgroundDependents(!m_IsUpdating);
    if (m_OwnerApplication != nullptr)
    {
        // Brush messages run after pending geometry updates. Queue repainting
        // directly, including the parent painting required by Transparent views.
        Ptr<PServerView> paintRoot = ptr_tmp_cast(this);
        if (!m_IsUpdating && HasFlags(PViewFlag::Transparent))
        {
            PIRect frame = GetNormalizedIBounds();
            paintRoot = GetOpacParent(paintRoot, &frame);
            if (paintRoot != nullptr) {
                paintRoot->Invalidate(frame);
            }
        }
        if (paintRoot != nullptr) {
            paintRoot->RequestPaintIfNeeded();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::MarkBackgroundBrushes()
{
    for (Ptr<PServerView> view = ptr_tmp_cast(this); view != nullptr; view = view->GetParent()) {
        view->m_HasBackgroundBrushes = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::InvalidateBackgroundDependents(bool invalidateSelf)
{
    if (invalidateSelf) {
        Invalidate();
    }
    for (Ptr<PServerView> child : m_ChildrenList)
    {
        if (!child->BrushCoversView(child->m_BackgroundBrush)) {
            child->InvalidateBackgroundDependents(true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::CaptureBackgrounds()
{
    if (m_HasBackgroundBrushes)
    {
        if (!m_HasBackgroundSnapshot)
        {
            const PServerBrushState& state = GetBackgroundBrushState();
            m_PreviousBackgroundLayers = state.Layers;
            m_PreviousBackgroundColor = state.Color;
            m_PreviousBackgroundIsSolid = state.IsSolid;
            m_HasBackgroundSnapshot = true;
        }
        for (Ptr<PServerView> child : m_ChildrenList) {
            child->CaptureBackgrounds();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ClearBackgroundSnapshots()
{
    if (m_HasBackgroundBrushes && !m_IsScrolling)
    {
        m_HasBackgroundSnapshot = false;
        m_PreviousBackgroundLayers.clear();
        for (Ptr<PServerView> child : m_ChildrenList) {
            child->ClearBackgroundSnapshots();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::CanReuseBackground(const PIPoint& movement) const
{
    if (!m_HasBackgroundSnapshot) {
        return true;
    }
    const PServerBrushState& state = GetBackgroundBrushState();
    const std::vector<PServerBrushLayer>& layers = state.Layers;
    if (state.IsSolid || m_PreviousBackgroundIsSolid) {
        return state.IsSolid && m_PreviousBackgroundIsSolid && state.Color == m_PreviousBackgroundColor;
    }
    if (layers.size() != m_PreviousBackgroundLayers.size()) {
        return false;
    }
    for (size_t i = 0; i < layers.size(); ++i)
    {
        const PServerBrushLayer& previous = m_PreviousBackgroundLayers[i];
        const PServerBrushLayer& current = layers[i];
        if (!previous.HasSameBrush(current)) {
            return false;
        }
        if (!PBrushUtils::CanTranslate(current.Description, previous.Bounds, current.Bounds, movement)) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ExcludeStableBackgrounds(PRegion& region, const PIPoint& movement) const
{
    const PServerBrushState& state = GetBackgroundBrushState();
    const std::vector<PServerBrushLayer>& layers = state.Layers;
    const size_t count = std::min(layers.size(), m_PreviousBackgroundLayers.size());
    const PIRect footprint = GetNormalizedIBounds() + PIPoint(m_ScreenPos);
    for (size_t index = 0; index < count && !region.IsEmpty(); ++index)
    {
        const PServerBrushLayer& current = layers[layers.size() - index - 1];
        const PServerBrushLayer& previous = m_PreviousBackgroundLayers[m_PreviousBackgroundLayers.size() - index - 1];
        if (!current.HasSameBrush(previous)
            || !PBrushUtils::CanTranslate(current.Description, previous.Bounds, current.Bounds, movement)) {
            break;
        }
        const std::array<PIRect, 9> opaqueRegions = PBrushUtils::OpaqueRegions(current.Description, current.Bounds, footprint);
        for (const PIRect& opaque : opaqueRegions)
        {
            if (opaque.IsValid()) {
                region.Exclude(opaque);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ExcludeChangedBackgrounds(PRegion& region, const PIPoint& movement)
{
    if (!m_HasBackgroundBrushes || m_HideCount != 0) {
        return;
    }
    if (!CanReuseBackground(movement) && m_VisibleReg != nullptr)
    {
        PRegion changed;
        const PIPoint screenPosition(m_ScreenPos);
        for (const PIRect& visible : m_VisibleReg->m_Rects)
        {
            for (const PIRect& copied : region.m_Rects)
            {
                const PIRect overlap = (visible + screenPosition) & copied;
                if (overlap.IsValid()) {
                    changed.Include(overlap);
                }
            }
        }
        ExcludeStableBackgrounds(changed, movement);
        for (const PIRect& bounds : changed.m_Rects)
        {
            region.Exclude(bounds);
            Invalidate(bounds - screenPosition);
        }
    }
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->ExcludeChangedBackgrounds(region, movement);
    }
}
