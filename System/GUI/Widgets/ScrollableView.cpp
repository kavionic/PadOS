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
// Created: 23.05.2025 21:30

#include <GUI/Widgets/ScrollableView.h>

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ScrollableView::ScrollableView(const PString& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ScrollableView::ScrollableView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : View(context, parent, xmlData)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollableView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    if (m_ContentView != nullptr) {
        m_ContentView->CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    } else {
        View::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point ScrollableView::CalculateContentSize() const
{
    if (m_ContentView != nullptr) {
        return m_ContentView->GetPreferredSize(PrefSizeType::Smallest);
    } else {
        return View::CalculateContentSize();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollableView::OnLayoutChanged()
{
    if (m_ContentView != nullptr)
    {
        const Rect bounds = GetNormalizedBounds();
        const Point minPreferredSize = m_ContentView->GetPreferredSize(PrefSizeType::Smallest);
        const Point maxPreferredSize = m_ContentView->GetPreferredSize(PrefSizeType::Greatest);

        const Point contentViewSize(std::clamp(bounds.Width(), minPreferredSize.x, maxPreferredSize.x), std::clamp(bounds.Height(), minPreferredSize.y, maxPreferredSize.y));

        m_ContentView->SetFrame(Rect::FromSize(contentViewSize));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollableView::SetContentView(Ptr<View> contentView)
{
    if (m_ContentView != nullptr)
    {
        m_ContentView->RemoveThis();
        m_ContentView = nullptr;
    }
    m_ContentView = contentView;
    if (m_ContentView != nullptr)
    {
        m_ContentView->SetFrame(Rect::FromSize(Point(GetBounds().Width(), m_ContentView->GetPreferredSize(PrefSizeType::Smallest).y)));
        AddChild(m_ContentView);
        InvalidateLayout();
    }
}

} // namespace os
