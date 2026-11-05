// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/View.h>
#include <System/ExceptionHandling.h>

Ptr<const PBrush> PView::GetStandardBrush(PStandardColorID color) const
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return application->GetStandardBrush(color);
}

Ptr<const PBrush> PView::GetStandardBrush(PNamedColors color) const
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return application->GetStandardBrush(color);
}

void PView::SetBackgroundBrush(Ptr<const PBrush> brush)
{
    SetBrushBinding(true, std::move(brush), PRect(), true);
}

void PView::SetBackgroundBrush(Ptr<const PBrush> brush, const PRect& bounds)
{
    SetBrushBinding(true, std::move(brush), bounds, false);
}

void PView::SetBrush(Ptr<const PBrush> brush)
{
    SetBrushBinding(false, std::move(brush), PRect(), true);
}

void PView::SetBrush(Ptr<const PBrush> brush, const PRect& bounds)
{
    SetBrushBinding(false, std::move(brush), bounds, false);
}

void PView::EraseRect(const PRect& rect)
{
    Post<ASViewEraseRect>(rect, false);
}

void PView::SendBrush(bool background)
{
    const PViewBrushBinding& binding = background ? m_BackgroundBrush : m_DrawingBrush;
    const handle_id handle = (binding.Brush != nullptr) ? binding.Brush->GetHandle() : INVALID_HANDLE;
    if (binding.Brush != nullptr && GetApplication() != nullptr && binding.Brush->GetApplication() != GetApplication()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    Post<ASViewSetBrush>(background, binding.Source, handle, binding.Bounds, binding.AutomaticBounds);
}

void PView::SetBrushBinding(bool background, Ptr<const PBrush> brush, const PRect& bounds, bool automaticBounds)
{
    if (!automaticBounds && !bounds.IsValid()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (brush != nullptr && GetApplication() != nullptr && brush->GetApplication() != GetApplication()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PViewBrushBinding& binding = background ? m_BackgroundBrush : m_DrawingBrush;
    const PBrushSource source = (brush != nullptr) ? PBrushSource::Brush : PBrushSource::Inherit;
    if (binding.Source != source || binding.Brush != brush || binding.Bounds != bounds || binding.AutomaticBounds != automaticBounds)
    {
        binding.Brush = std::move(brush);
        binding.Bounds = bounds;
        binding.Source = source;
        binding.AutomaticBounds = automaticBounds;
        SendBrush(background);
        if (background && m_BeginPainCount == 0) {
            Invalidate();
        }
    }
}
