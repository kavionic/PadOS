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


namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IconView::IconView(const String& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::WillDraw)
{
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IconView::IconView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : View(context, parent, xmlData)
{
    MergeFlags(ViewFlags::WillDraw);
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool IconView::LoadBitmap(const Path& path)
{
    return m_IconView->LoadBitmap(path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool IconView::LoadBitmap(StreamableIO& file)
{
    return m_IconView->LoadBitmap(file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void IconView::SetBitmap(Ptr<Bitmap> bitmap)
{
    m_IconView->SetBitmap(bitmap);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<os::Bitmap> IconView::GetBitmap() const
{
    return m_IconView->GetBitmap();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void IconView::SetLabel(const String& label)
{
    m_LabelView->SetText(label);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void IconView::Clear()
{
    m_IconView->ClearBitmap();
    m_LabelView->SetText(String::zero);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void IconView::SetHighlighting(bool isHighlighted)
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

void IconView::SetSelection(bool isSelected)
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

void IconView::Construct()
{
    m_IconView = ptr_new<BitmapView>();
    m_LabelView = ptr_new<TextView>();

    m_IconView->SetBorders(5.0f, 5.0f, 5.0f, 0.0f);
    m_LabelView->SetBorders(5.0f, 10.0f, 5.0f, 5.0f);

    AddChild(m_IconView);
    AddChild(m_LabelView);

    SetLayoutNode(ptr_new<VLayoutNode>());
}

void IconView::UpdateHighlightColor()
{
    Color color;

    if (m_IsSelected && m_IsHighlighted) {
        color = Color::FromColorID(NamedColors::darkblue);
    } else if (m_IsSelected) {
        color = Color::FromColorID(NamedColors::royalblue);
    } else if (m_IsHighlighted) {
        color = Color::FromColorID(NamedColors::lightblue);
    } else {
        color = get_standard_color(StandardColorID::DefaultBackground);
    }

    m_IconView->SetBgColor(color);
    m_LabelView->SetBgColor(color);
    SetEraseColor(color);

    m_IconView->Invalidate();
    m_LabelView->Invalidate();
    Invalidate();
}

} // namespace os
