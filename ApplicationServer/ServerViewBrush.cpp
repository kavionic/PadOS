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
/// Preserve movement snapshots when resetting geometry-generation stamps.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::InvalidateBrushCaches()
{
    m_BackgroundCacheGeneration = 0;
    m_DrawingCacheGeneration = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// The existing handler registry includes hidden and detached server views.
/// This runs only on counter wrap, on the application-server looper.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::InvalidateAllBrushCaches()
{
    PLooper* looper = PLooper::GetCurrentLooper();
    assert(looper != nullptr);
    for (const auto& [handle, handler] : looper->GetHandlers())
    {
        PServerView* view = dynamic_cast<PServerView*>(ptr_raw_pointer_cast(handler));
        if (view != nullptr) {
            view->InvalidateBrushCaches();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetBrush(const PServerBrush& brush)
{
    if (m_DrawingBrush.Brush != brush)
    {
        m_DrawingBrush.Brush = brush;
        m_DrawingCacheGeneration = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetBackgroundBrush(const PServerBrush& brush)
{
    if (m_BackgroundBrush.Brush != brush)
    {
        m_BackgroundBrush.Brush = brush;
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
        if (m_BackgroundBrush.Brush.GetData().Style == PBrushStyle::TexturePattern) {
            BackgroundBrushChanged();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillRect(const PRect& rect, const PServerBrush& brush)
{
    if (brush.GetData().Style != PBrushStyle::None) {
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
/// Resolve only when geometry, ancestry, or a brush binding changes.
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
/// Drawing bindings hold immutable values and do not invalidate views.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PServerBrushState& PServerView::GetDrawingBrushState() const
{
    return GetDrawingBrushState(m_DrawingBrush.Brush);
}

///////////////////////////////////////////////////////////////////////////////
/// Cache the last brush used for drawing, including explicit FillRect sources.
/// Its origin always comes from the view. Brush equality and background
/// generation keep repeated fills cheap without changing the selected brush.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PServerBrushState& PServerView::GetDrawingBrushState(const PServerBrush& brush) const
{
    if (brush.GetData().Style == PBrushStyle::None) {
        return GetBackgroundBrushState();
    }
    if (m_DrawingCacheBrush != brush || m_DrawingCacheGeneration != s_BackgroundGeneration)
    {
        const PServerBrushBinding binding{brush, m_DrawingBrush.Origin};
        const PServerBrushState* base = BrushCoversView(binding) ? nullptr : &GetBackgroundBrushState();
        ResolveBrushState(m_DrawingBrushState, binding, base);
        m_DrawingCacheBrush = brush;
        m_DrawingCacheGeneration = s_BackgroundGeneration;
        if (brush.GetData().Style == PBrushStyle::TexturePattern && brush.GetImage()->GetDescription().LayoutMode == PImageLayoutMode::ViewRelative) {
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
    return m_DrawingBrush.Brush.GetData().Style != PBrushStyle::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PServerView::GetDrawingBrushColor() const
{
    if (!HasDrawingBrush()) {
        return PColor(0, 0, 0, 0);
    }
    if (m_DrawingBrush.Brush.GetData().Style == PBrushStyle::SolidPattern) {
        return m_DrawingBrush.Brush.GetData().Color;
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
    return binding.Brush.IsOpaque();
}

///////////////////////////////////////////////////////////////////////////////
/// Collapse constant layers once. Patterned compositions retain immutable
/// brush values alongside their original screen-space sampling rectangles.
/// The selecting view supplies the reference size; inherited layers keep
/// their owner's prepared mapping through scrolling and clipping.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ResolveBrushState(PServerBrushState& state, const PServerBrushBinding& binding, const PServerBrushState* base) const
{
    state.Layers.clear();
    state.Color = (base != nullptr) ? base->Color : PColor(255, 255, 255);
    const PBrushData& data = binding.Brush.GetData();
    if (data.Style == PBrushStyle::None)
    {
        if (base != nullptr && !base->IsSolid()) {
            state.Layers = base->Layers;
        }
        return;
    }
    if (data.Style == PBrushStyle::SolidPattern && (base == nullptr || base->IsSolid()))
    {
        state.Color = PBrushUtils::Composite(data.Color, state.Color);
        return;
    }
    if (base != nullptr)
    {
        if (base->IsSolid()) {
            state.Layers.push_back({PServerBrush(base->Color), {}, {}});
        } else {
            state.Layers = base->Layers;
        }
    }
    else if (!binding.Brush.IsOpaque())
    {
        state.Layers.push_back({PServerBrush(state.Color), {}, {}});
    }
    const PIPoint origin(binding.Origin + m_ScrollOffset + m_ScreenPos);
    const PIPoint viewSize = GetIFrame().Size();
    const PIPoint referenceSize(std::max<int32_t>(0, viewSize.x), std::max<int32_t>(0, viewSize.y));
    const PIPoint size = (data.Style == PBrushStyle::TexturePattern)
        ? PBrushUtils::TileSize(data, binding.Brush.GetImage()->GetDescription(), referenceSize) : PIPoint(0, 0);
    state.Layers.push_back({binding.Brush, PBrushUtils::TileBounds(origin, size), referenceSize});
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BackgroundBrushChanged()
{
    AdvanceBackgroundGeneration();
    if (m_BackgroundBrush.Brush.GetData().Style != PBrushStyle::SolidPattern || !m_BackgroundBrush.Brush.IsOpaque()) {
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
        if (current.Brush.IsOpaque())
        {
            region.Exclude(footprint);
        }
        else if (current.Brush.GetData().Style == PBrushStyle::TexturePattern)
        {
            PBrushUtils::VisitOpaqueRegions(
                current.Brush.GetData(),
                current.Brush.GetImage()->GetDescription(),
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
