// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PView::GetStandardBrush(PStandardColorID color) const
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return application->GetStandardBrush(color);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PView::GetStandardBrush(PNamedColors color) const
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return application->GetStandardBrush(color);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetBackgroundBrush(Ptr<const PBrush> brush)
{
    PApplication* application = GetApplication();
    if (brush != nullptr && application != nullptr && brush->GetApplication() != application) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_BackgroundBrush != brush)
    {
        m_BackgroundBrush = std::move(brush);
        Post<ASViewSetBackgroundBrush>((m_BackgroundBrush != nullptr) ? m_BackgroundBrush->GetHandle() : INVALID_HANDLE);
        if (m_BeginPainCount == 0) {
            Invalidate();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetBrush(Ptr<const PBrush> brush)
{
    PApplication* application = GetApplication();
    if (brush != nullptr && application != nullptr && brush->GetApplication() != application) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_DrawingBrush != brush)
    {
        m_DrawingBrush = std::move(brush);
        Post<ASViewSetBrush>((m_DrawingBrush != nullptr) ? m_DrawingBrush->GetHandle() : INVALID_HANDLE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetBackgroundBrushOrigin(const PPoint& origin)
{
    if (!PMath::is_finite(origin.x) || !PMath::is_finite(origin.y)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_BackgroundBrushOrigin != origin)
    {
        m_BackgroundBrushOrigin = origin;
        Post<ASViewSetBackgroundBrushOrigin>(origin);
        if (m_BackgroundBrush != nullptr && m_BackgroundBrush->GetStyle() == PBrushStyle::TexturePattern && m_BeginPainCount == 0) {
            Invalidate();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetBrushOrigin(const PPoint& origin)
{
    if (!PMath::is_finite(origin.x) || !PMath::is_finite(origin.y)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_DrawingBrushOrigin != origin)
    {
        m_DrawingBrushOrigin = origin;
        Post<ASViewSetBrushOrigin>(origin);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::FillRect(const PRect& rect, Ptr<const PBrush> brush)
{
    if (brush != nullptr) {
        Post<ASViewFillRectBrush>(rect, brush->GetHandle());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::EraseRect(const PRect& rect)
{
    Post<ASViewEraseRect>(rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::FillTextBackground(const PRect& rect)
{
    if (m_DrawingBrush != nullptr && m_DrawingBrush->GetStyle() != PBrushStyle::None) {
        FillRect(rect, m_DrawingBrush);
    } else {
        EraseRect(rect);
    }
}
