// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 20.05.2025 18:30

#include <GUI/Widgets/IconView.h>
#include <GUI/Widgets/BitmapView.h>
#include <GUI/Widgets/TextView.h>
#include <GUI/Bitmap.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIconView::PIconView(const PString& name, Ptr<PView> parent, uint32_t flags) : PView(name, parent, flags | PViewFlags::WillDraw)
{
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIconView::PIconView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PView(context, parent, xmlData)
{
    MergeFlags(PViewFlags::WillDraw);
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PBitmapView> PIconView::GetIconView()
{
    return m_IconView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBitmapView> PIconView::GetIconView() const
{
    return m_IconView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PTextView> PIconView::GetLabelView()
{
    return m_LabelView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PTextView> PIconView::GetLabelView() const
{
    return m_LabelView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PIconView::LoadBitmap(const PPath& path)
{
    return m_IconView->LoadBitmap(path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PIconView::LoadBitmap(PStreamableIO& file)
{
    return m_IconView->LoadBitmap(file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIconView::SetBitmap(Ptr<PBitmap> bitmap)
{
    m_IconView->SetBitmap(bitmap);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PBitmap> PIconView::GetBitmap() const
{
    return m_IconView->GetBitmap();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIconView::SetLabel(const PString& label)
{
    m_LabelView->SetText(label);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIconView::Clear()
{
    m_IconView->ClearBitmap();
    m_LabelView->SetText(PString::zero);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIconView::SetHighlighting(bool isHighlighted)
{
    if (isHighlighted != m_IsHighlighted)
    {
        m_IsHighlighted = isHighlighted;
        UpdateHighlightColor();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIconView::SetSelection(bool isSelected)
{
    if (isSelected != m_IsSelected)
    {
        m_IsSelected = isSelected;
        UpdateHighlightColor();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIconView::Construct()
{
    m_IconView = ptr_new<PBitmapView>();
    m_LabelView = ptr_new<PTextView>();

    m_IconView->SetBorders(5.0f, 5.0f, 5.0f, 0.0f);
    m_LabelView->SetBorders(5.0f, 10.0f, 5.0f, 5.0f);

    AddChild(m_IconView);
    AddChild(m_LabelView);

    SetLayoutNode(ptr_new<PVLayoutNode>());
}

void PIconView::UpdateHighlightColor()
{
    PColor color;

    if (m_IsSelected && m_IsHighlighted) {
        color = PColor::FromColorID(PNamedColors::darkblue);
    } else if (m_IsSelected) {
        color = PColor::FromColorID(PNamedColors::royalblue);
    } else if (m_IsHighlighted) {
        color = PColor::FromColorID(PNamedColors::lightblue);
    } else {
        color = pget_standard_color(PStandardColorID::DefaultBackground);
    }

    m_IconView->SetBgColor(color);
    m_LabelView->SetBgColor(color);
    SetEraseColor(color);

    m_IconView->Invalidate();
    m_LabelView->Invalidate();
    Invalidate();
}
