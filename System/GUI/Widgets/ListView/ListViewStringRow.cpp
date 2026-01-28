// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/Widgets/ListViewStringRow.h>
#include <GUI/View.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewStringRow::AttachToView(Ptr<View> view, int column)
{
    if (column >= m_Strings.size()) m_Strings.resize(column + 1);

    m_Strings[column].second = view->GetStringWidth(m_Strings[column].first) + 5.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewStringRow::SetRect(const Rect& rect, size_t column)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewStringRow::AppendString(const PString& string)
{
    m_Strings.push_back(std::make_pair(string, 0.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewStringRow::SetString(size_t index, const PString& string)
{
    if (index >= m_Strings.size()) m_Strings.resize(index + 1);
    m_Strings[index].first = string;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& ListViewStringRow::GetString(size_t index) const
{
    return (index < m_Strings.size()) ? m_Strings[index].first : PString::zero;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float ListViewStringRow::GetWidth(Ptr<View> view, size_t column)
{
    return (column < m_Strings.size()) ? m_Strings[column].second : 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float ListViewStringRow::GetHeight(Ptr<View> view)
{
    const FontHeight fontHeight = view->GetFontHeight();
    return fontHeight.ascender + fontHeight.descender;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewStringRow::Paint(const Rect& frame, Ptr<View> view, size_t column, bool selected, bool highlighted, bool hasFocus)
{
    if (column >= m_Strings.size()) {
        return;
    }

    const FontHeight fontHeight = view->GetFontHeight();

    view->SetFgColor(255, 255, 255);
    view->FillRect(frame);

    const float textHeight = fontHeight.ascender + fontHeight.descender;
    const float baseLine = frame.top + (frame.Height() + 1.0f) / 2 - textHeight / 2 + fontHeight.ascender;

    view->MovePenTo(frame.left + 3.0f, baseLine);

    if (highlighted && column == 0) {
        view->SetFgColor(255, 255, 255);
        view->SetBgColor(0, 50, 200);
    } else if (selected && column == 0) {
        view->SetFgColor(255, 255, 255);
        view->SetBgColor(0, 0, 0);
    } else {
        view->SetBgColor(255, 255, 255);
        view->SetFgColor(0, 0, 0);
    }

    if (selected && column == 0)
    {
        Rect rect = frame;
        rect.right = rect.left + view->GetStringWidth(m_Strings[column].first.c_str()) + 4;
        rect.top = baseLine - fontHeight.ascender - 1;
        rect.bottom = baseLine + fontHeight.descender + 1;
        view->FillRect(rect, Color(0, 0, 0, 0));
    }
    view->DrawString(m_Strings[column].first.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewStringRow::IsLessThan(Ptr<const ListViewRow> other, size_t column) const
{
    Ptr<const ListViewStringRow> row = ptr_dynamic_cast<const ListViewStringRow>(other);
    if (row == nullptr || column >= m_Strings.size() || column >= row->m_Strings.size()) {
        return false;
    }
    return m_Strings[column].first < row->m_Strings[column].first;
}
