// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyle.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrush PView::GetStandardBrush(PStandardColorID color) const
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

PBrush PView::GetStandardBrush(PNamedColors color) const
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

void PView::SetBackgroundBrush(const PBrush& brush)
{
    PApplication* application = GetApplication();
    if (brush.IsValid() && application != nullptr && brush.GetApplication() != application) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_BackgroundBrush != brush)
    {
        Post<ASViewSetBackgroundBrush>(GetServerBrushData(brush));
        m_BackgroundBrush = brush;
        if (m_BeginPainCount == 0) {
            Invalidate();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetBackgroundBrush(PGUIColorID color)
{
    const PGUIStyle* style = GetStyle();
    if (style == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    SetBackgroundBrush(style->GetBrush(color));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetBrush(const PBrush& brush)
{
    PApplication* application = GetApplication();
    if (brush.IsValid() && application != nullptr && brush.GetApplication() != application) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_DrawingBrush != brush)
    {
        Post<ASViewSetBrush>(GetServerBrushData(brush));
        m_DrawingBrush = brush;
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
        if (m_BackgroundBrush.IsValid() && m_BackgroundBrush.GetStyle() == PBrushStyle::TexturePattern && m_BeginPainCount == 0) {
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

void PView::FillRect(const PRect& rect, const PBrush& brush)
{
    if (brush.IsValid()) {
        Post<ASViewFillRectBrush>(rect, GetServerBrushData(brush));
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

PBrushData PView::GetServerBrushData(const PBrush& brush) const
{
    if (m_ServerHandle == INVALID_HANDLE) {
        return PBrush::zero.GetServerData();
    }
    if (brush.IsValid())
    {
        PApplication* owner = brush.GetApplication();
        if (owner == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NOENT);
        }
        PApplication* application = GetApplication();
        if (application != nullptr && application != owner) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
    }
    return brush.GetServerData();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::FillTextBackground(const PRect& rect, const PBrush& backgroundBrush)
{
    if (backgroundBrush.IsValid() && backgroundBrush.GetStyle() != PBrushStyle::None) {
        FillRect(rect, backgroundBrush);
    } else {
        EraseRect(rect);
    }
}
