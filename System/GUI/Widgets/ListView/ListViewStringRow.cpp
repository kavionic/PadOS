// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    const float textHeight = fontHeight.ascender + fontHeight.descender;
    const float baselineY = frame.top + (frame.Height() + 1.0f - textHeight) * 0.5f + fontHeight.ascender;
    const PPoint baseline(frame.left + 3.0f, baselineY);
    const PString& text = m_Strings[column].first;

    if (column != 0 || (!selected && !highlighted))
    {
        view->SetFgColor(0, 0, 0);
        view->SetBgColor(255, 255, 255);
        view->DrawString(text, baseline, frame);
        return;
    }

    const PPoint scrollOffset = view->GetScrollOffset();
    const PIRect pixelFrame = PIRect(frame + scrollOffset) & PIRect(view->GetNormalizedBounds());
    const PPoint fillOffset = PPoint(0.5f, 0.5f) - scrollOffset;
    const PIPoint pixelBaseline(baseline + scrollOffset);
    const float textWidth = selected ? view->GetStringWidth(text) : std::max(0.0f, m_Strings[column].second - 5.0f);
    const PIRect textFrame(
        pixelBaseline.x,
        pixelBaseline.y - int(fontHeight.ascender),
        pixelBaseline.x + int(ceilf(textWidth - 0.5f)),
        pixelBaseline.y + int(fontHeight.descender));
    PIRect selectionFrame = textFrame & pixelFrame;
    if (selected)
    {
        const PRect selectionArea(
            frame.left,
            baselineY - fontHeight.ascender - 1.0f,
            frame.left + textWidth + 4.0f,
            baselineY + fontHeight.descender + 1.0f);
        selectionFrame = PIRect(selectionArea + scrollOffset) & pixelFrame;
    }

    const auto fillOutside = [&view, fillOffset](const PIRect& outer, const PIRect& inner, PColor color)
    {
        const PIRect clipped = outer & inner;
        if (!clipped.IsValid())
        {
            view->FillRect(PRect(outer) + fillOffset, color);
        }
        else
        {
            if (outer.top < clipped.top) {
                view->FillRect(PRect(PIRect(outer.left, outer.top, outer.right, clipped.top)) + fillOffset, color);
            }
            if (clipped.bottom < outer.bottom) {
                view->FillRect(PRect(PIRect(outer.left, clipped.bottom, outer.right, outer.bottom)) + fillOffset, color);
            }
            if (outer.left < clipped.left) {
                view->FillRect(PRect(PIRect(outer.left, clipped.top, clipped.left, clipped.bottom)) + fillOffset, color);
            }
            if (clipped.right < outer.right) {
                view->FillRect(PRect(PIRect(clipped.right, clipped.top, outer.right, clipped.bottom)) + fillOffset, color);
            }
        }
    };
    fillOutside(pixelFrame, selectionFrame, PColor(255, 255, 255));

    view->SetFgColor(255, 255, 255);
    if (highlighted)
    {
        if (selected) {
            fillOutside(selectionFrame, textFrame, PColor(0, 0, 0));
        }
        view->SetBgColor(0, 50, 200);
        view->DrawString(text, baseline, PRect(textFrame & pixelFrame) + fillOffset);
    }
    else
    {
        view->SetBgColor(0, 0, 0);
        view->DrawString(text, baseline, PRect(selectionFrame) + fillOffset);
    }
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
