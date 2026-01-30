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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewStringRow::AttachToView(Ptr<PView> view, int column)
{
    if (column >= m_Strings.size()) m_Strings.resize(column + 1);

    m_Strings[column].second = view->GetStringWidth(m_Strings[column].first) + 5.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewStringRow::SetRect(const PRect& rect, size_t column)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewStringRow::AppendString(const PString& string)
{
    m_Strings.push_back(std::make_pair(string, 0.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewStringRow::SetString(size_t index, const PString& string)
{
    if (index >= m_Strings.size()) m_Strings.resize(index + 1);
    m_Strings[index].first = string;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& PListViewStringRow::GetString(size_t index) const
{
    return (index < m_Strings.size()) ? m_Strings[index].first : PString::zero;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PListViewStringRow::GetWidth(Ptr<PView> view, size_t column)
{
    return (column < m_Strings.size()) ? m_Strings[column].second : 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PListViewStringRow::GetHeight(Ptr<PView> view)
{
    const PFontHeight fontHeight = view->GetFontHeight();
    return fontHeight.ascender + fontHeight.descender;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewStringRow::Paint(const PRect& frame, Ptr<PView> view, size_t column, bool selected, bool highlighted, bool hasFocus)
{
    if (column >= m_Strings.size()) {
        return;
    }

    const PFontHeight fontHeight = view->GetFontHeight();

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
        PRect rect = frame;
        rect.right = rect.left + view->GetStringWidth(m_Strings[column].first.c_str()) + 4;
        rect.top = baseLine - fontHeight.ascender - 1;
        rect.bottom = baseLine + fontHeight.descender + 1;
        view->FillRect(rect, PColor(0, 0, 0, 0));
    }
    view->DrawString(m_Strings[column].first.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PListViewStringRow::IsLessThan(Ptr<const PListViewRow> other, size_t column) const
{
    Ptr<const PListViewStringRow> row = ptr_dynamic_cast<const PListViewStringRow>(other);
    if (row == nullptr || column >= m_Strings.size() || column >= row->m_Strings.size()) {
        return false;
    }
    return m_Strings[column].first < row->m_Strings[column].first;
}
