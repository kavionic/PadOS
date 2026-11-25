// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <optional>
#include <utility>

#include <GUI/Style/GUIStyle.h>
#include <GUI/ViewFactory.h>
#include <Storage/Path.h>
#include <System/ExceptionHandling.h>

#include "GUIStyleVariantItemView.h"

namespace guistylepreferences
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GUIStyleVariantItemView::GUIStyleVariantItemView() : PGroupView(PString::zero, PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize)
{
    PViewFactory::Get().LoadView(ptr_tmp_cast(this), "Preferences/GUIStyleVariantItemView.xml");
    FindChild("preview", m_PreviewView);
    FindChild("name", m_NameView);
    if (m_PreviewView == nullptr || m_NameView == nullptr) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Failed to load GUI style list item layout");
    }
    OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStyleVariantItemView::OnStyleChanged()
{
    PGroupView::OnStyleChanged();
    UpdateSelection(m_IsSelected, m_IsHighlighted);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStyleVariantItemView::AttachedToScreen()
{
    PGroupView::AttachedToScreen();
    UpdatePreview();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStyleVariantItemView::DetachedFromScreen()
{
    m_PreviewView->ClearBitmap();
    PGroupView::DetachedFromScreen();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStyleVariantItemView::SetData(Ptr<const GUIStyleVariantItemData> item, bool isSelected, bool isHighlighted)
{
    m_Item = std::move(item);
    m_PreviewView->ClearBitmap();
    m_NameView->SetText((m_Item != nullptr) ? m_Item->Variant.DisplayName : PString::zero);
    UpdatePreview();
    UpdateSelection(isSelected, isHighlighted);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStyleVariantItemView::UpdateSelection(bool isSelected, bool isHighlighted)
{
    m_IsSelected = isSelected;
    m_IsHighlighted = isHighlighted;
    if (isSelected && isHighlighted) {
        SetBackgroundBrush(PNamedColors::darkblue);
    } else if (isSelected) {
        SetBackgroundBrush(PNamedColors::royalblue);
    } else if (isHighlighted) {
        SetBackgroundBrush(PNamedColors::lightblue);
    } else {
        SetBackgroundBrush(PGUIColorID::Background);
    }
    if (isSelected) {
        m_NameView->SetTextColor(PColor(PNamedColors::white));
    } else if (isHighlighted) {
        m_NameView->SetTextColor(PColor(PNamedColors::black));
    } else {
        m_NameView->SetTextColor(std::nullopt);
    }
    Invalidate();
    m_NameView->Invalidate();
    m_PreviewView->Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStyleVariantItemView::UpdatePreview()
{
    if (HasFlags(PViewFlag::IsAttachedToScreen) && m_Item != nullptr && !m_Item->Variant.PreviewPath.empty()) {
        m_PreviewView->LoadBitmap(PPath(m_Item->Variant.PreviewPath));
    }
}

} // namespace guistylepreferences
