// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <ApplicationServer/DisplayDriver.h>
#include <GUI/Region.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>
#include "ServerView.h"

uint32_t PServerView::s_BackgroundGeneration = 1;

///////////////////////////////////////////////////////////////////////////////
/// Preserve movement snapshots on a geometry-generation wrap. A brush revision
/// wrap invalidates only snapshots that refer to that changed brush.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::InvalidateBrushCaches(const PServerBrush* changedBrush)
{
    m_BackgroundCacheGeneration = 0;
    m_DrawingCacheGeneration = 0;
    if (changedBrush != nullptr)
    {
        for (PServerBrushLayer& layer : m_PreviousBackgroundLayers)
        {
            if (layer.Brush == changedBrush) {
                layer.Revision = 0;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// The existing handler registry includes hidden and detached server views.
/// This runs only on counter wrap, on the application-server looper.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::InvalidateAllBrushCaches(const PServerBrush* changedBrush)
{
    PLooper* looper = PLooper::GetCurrentLooper();
    assert(looper != nullptr);
    for (const auto& [handle, handler] : looper->GetHandlers())
    {
        PServerView* view = dynamic_cast<PServerView*>(ptr_raw_pointer_cast(handler));
        if (view != nullptr) {
            view->InvalidateBrushCaches(changedBrush);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetBrush(Ptr<PServerBrush> brush)
{
    if (m_DrawingBrush.Brush != brush)
    {
        m_DrawingBrush.Brush = std::move(brush);
        m_DrawingCacheGeneration = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetBackgroundBrush(Ptr<PServerBrush> brush)
{
    if (m_BackgroundBrush.Brush != brush)
    {
        if (brush != nullptr) {
            brush->SignalChanged.Connect(this, &PServerView::BackgroundBrushChanged);
        }
        if (m_BackgroundBrush.Brush != nullptr) {
            m_BackgroundBrush.Brush->SignalChanged.Disconnect(this, &PServerView::BackgroundBrushChanged);
        }
        m_BackgroundBrush.Brush = std::move(brush);
        BackgroundBrushChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetBrushOrigin(const PPoint& origin)
{
    if (!PMath::is_finite(origin.x) || !PMath::is_finite(origin.y)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_DrawingBrush.Origin != origin)
    {
        m_DrawingBrush.Origin = origin;
        m_DrawingCacheGeneration = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetBackgroundBrushOrigin(const PPoint& origin)
{
    if (!PMath::is_finite(origin.x) || !PMath::is_finite(origin.y)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_BackgroundBrush.Origin != origin)
    {
        m_BackgroundBrush.Origin = origin;
        if (m_BackgroundBrush.Brush != nullptr && m_BackgroundBrush.Brush->GetDescription().Style == PBrushStyle::TexturePattern) {
            BackgroundBrushChanged();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillRect(const PRect& rect, Ptr<PServerBrush> brush)
{
    if (brush != nullptr && brush->GetDescription().Style != PBrushStyle::None) {
        FillResolvedRect(rect, GetDrawingBrushState(brush));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::EraseRect(const PRect& rect)
{
    FillResolvedRect(rect, GetBackgroundBrushState());
}

///////////////////////////////////////////////////////////////////////////////
/// Clear cache stamps before reusing a geometry generation. Keep movement
/// snapshots so pending copies can still compare their original mappings.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AdvanceBackgroundGeneration()
{
    if (++s_BackgroundGeneration == 0)
    {
        InvalidateAllBrushCaches();
        s_BackgroundGeneration = 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Solid compositions retain the hardware fill path. Patterned compositions use
/// the same prepared sampling state as text, without changing brush bindings.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillResolvedRect(const PRect& rect, const PServerBrushState& state)
{
    if (state.IsSolid())
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
    return GetDrawingBrushState(m_DrawingBrush.Brush);
}

///////////////////////////////////////////////////////////////////////////////
/// Cache the last brush used for drawing, including explicit FillRect sources.
/// Its origin always comes from the view. Identity, revision, and background
/// generation keep repeated fills cheap without changing the selected brush.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PServerBrushState& PServerView::GetDrawingBrushState(const Ptr<PServerBrush>& brush) const
{
    if (brush == nullptr || brush->GetDescription().Style == PBrushStyle::None) {
        return GetBackgroundBrushState();
    }
    const uint32_t revision = brush->GetRevision();
    if (m_DrawingCacheBrush != brush || m_DrawingCacheGeneration != s_BackgroundGeneration || m_DrawingCacheRevision != revision)
    {
        const PServerBrushBinding binding{brush, m_DrawingBrush.Origin};
        const PServerBrushState* base = BrushCoversView(binding) ? nullptr : &GetBackgroundBrushState();
        ResolveBrushState(m_DrawingBrushState, binding, base);
        m_DrawingCacheBrush = brush;
        m_DrawingCacheGeneration = s_BackgroundGeneration;
        m_DrawingCacheRevision = revision;
        if (brush->GetDescription().Style == PBrushStyle::TexturePattern
            && brush->GetDescription().Image.LayoutMode == PImageLayoutMode::ViewRelative) {
            m_HasViewRelativeDrawingBrushes = true;
        }
    }
    return m_DrawingBrushState;
}

///////////////////////////////////////////////////////////////////////////////
/// Color-only primitive fills preserve the selected solid's alpha (including
/// alpha zero, which disables ellipse/pie fills). Text uses resolved backgrounds.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::HasDrawingBrush() const
{
    return m_DrawingBrush.Brush != nullptr && m_DrawingBrush.Brush->GetDescription().Style != PBrushStyle::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PServerView::GetDrawingBrushColor() const
{
    if (!HasDrawingBrush()) {
        return PColor(0, 0, 0, 0);
    }
    if (m_DrawingBrush.Brush->GetDescription().Style == PBrushStyle::SolidPattern) {
        return m_DrawingBrush.Brush->GetDescription().Color;
    }
    const PServerBrushState& state = GetDrawingBrushState();
    if (!state.IsSolid()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return state.Color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::BrushCoversView(const PServerBrushBinding& binding) const
{
    return binding.Brush != nullptr && binding.Brush->GetDescription().IsOpaque();
}

///////////////////////////////////////////////////////////////////////////////
/// Collapse constant layers once. Patterned compositions retain resource
/// identity and revision alongside their original screen-space sampling
/// rectangles. The selecting view supplies the reference size; inherited
/// layers keep their owner's prepared mapping through scrolling and clipping.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ResolveBrushState(PServerBrushState& state, const PServerBrushBinding& binding, const PServerBrushState* base) const
{
    state.Layers.clear();
    state.Color = (base != nullptr) ? base->Color : PColor(255, 255, 255);
    if (binding.Brush == nullptr || binding.Brush->GetDescription().Style == PBrushStyle::None)
    {
        if (base != nullptr && !base->IsSolid()) {
            state.Layers = base->Layers;
        }
        return;
    }
    const PBrushDescription& description = binding.Brush->GetDescription();
    if (description.Style == PBrushStyle::SolidPattern && (base == nullptr || base->IsSolid()))
    {
        state.Color = PBrushUtils::Composite(description.Color, state.Color);
        return;
    }
    if (base != nullptr)
    {
        if (base->IsSolid())
        {
            PServerBrushLayer constant;
            constant.Color = base->Color;
            state.Layers.push_back(constant);
        }
        else
        {
            state.Layers = base->Layers;
        }
    }
    else if (!description.IsOpaque())
    {
        PServerBrushLayer constant;
        constant.Color = state.Color;
        state.Layers.push_back(constant);
    }
    const PIPoint origin(binding.Origin + m_ScrollOffset + m_ScreenPos);
    const PIPoint viewSize = GetIFrame().Size();
    const PIPoint referenceSize(std::max<int32_t>(0, viewSize.x), std::max<int32_t>(0, viewSize.y));
    const PIPoint size = (description.Style == PBrushStyle::TexturePattern) ? PBrushUtils::TileSize(description, referenceSize) : PIPoint(0, 0);
    state.Layers.push_back({binding.Brush, PBrushUtils::TileBounds(origin, size), referenceSize, binding.Brush->GetRevision()});
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BackgroundBrushChanged()
{
    AdvanceBackgroundGeneration();
    if (m_BackgroundBrush.Brush == nullptr || m_BackgroundBrush.Brush->GetDescription().Style != PBrushStyle::SolidPattern
        || !m_BackgroundBrush.Brush->GetDescription().IsOpaque()) {
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
    if (state.IsSolid() || m_PreviousBackgroundLayers.empty()) {
        return state.IsSolid() && m_PreviousBackgroundLayers.empty() && state.Color == m_PreviousBackgroundColor;
    }
    if (layers.size() != m_PreviousBackgroundLayers.size()) {
        return false;
    }
    for (size_t i = 0; i < layers.size(); ++i)
    {
        const PServerBrushLayer& previous = m_PreviousBackgroundLayers[i];
        const PServerBrushLayer& current = layers[i];
        if (!current.CanTranslateFrom(previous, movement)) {
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
        if (!current.CanTranslateFrom(previous, movement)) {
            break;
        }
        if (current.Brush == nullptr)
        {
            region.Exclude(footprint);
        }
        else
        {
            PBrushUtils::VisitOpaqueRegions(
                current.Brush->GetDescription(),
                current.Bounds,
                footprint,
                [&region](const PIRect& opaque) { region.Exclude(opaque); });
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
