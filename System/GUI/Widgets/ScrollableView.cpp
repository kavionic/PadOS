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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollableView::PScrollableView(const PString& name, Ptr<PView> parent, uint32_t flags) : PView(name, parent, flags)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollableView::PScrollableView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PView(context, parent, xmlData)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollableView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    if (m_ContentView != nullptr) {
        m_ContentView->CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PScrollableView::CalculateContentSize() const
{
    if (m_ContentView != nullptr) {
        return m_ContentView->GetPreferredSize(PPrefSizeType::Smallest);
    } else {
        return PView::CalculateContentSize();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollableView::OnLayoutChanged()
{
    if (m_ContentView != nullptr)
    {
        const PRect bounds = GetNormalizedBounds();
        const PPoint minPreferredSize = m_ContentView->GetPreferredSize(PPrefSizeType::Smallest);
        const PPoint maxPreferredSize = m_ContentView->GetPreferredSize(PPrefSizeType::Greatest);

        const PPoint contentViewSize(std::clamp(bounds.Width(), minPreferredSize.x, maxPreferredSize.x), std::clamp(bounds.Height(), minPreferredSize.y, maxPreferredSize.y));

        m_ContentView->SetFrame(PRect::FromSize(contentViewSize));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollableView::SetContentView(Ptr<PView> contentView)
{
    if (m_ContentView != nullptr)
    {
        m_ContentView->RemoveThis();
        m_ContentView = nullptr;
    }
    m_ContentView = contentView;
    if (m_ContentView != nullptr)
    {
        m_ContentView->SetFrame(PRect::FromSize(PPoint(GetBounds().Width(), m_ContentView->GetPreferredSize(PPrefSizeType::Smallest).y)));
        AddChild(m_ContentView);
        InvalidateLayout();
    }
}
