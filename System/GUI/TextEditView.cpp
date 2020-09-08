// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.09.2020 13:30

#include <GUI/TextEditView.h>
#include <GUI/TextBox.h>
#include <Utils/UTF8Utils.h>

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextEditView::TextEditView(const String& name, const String& text, Ptr<View> parent, uint32_t flags) : Control(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize), m_Text(text)
{
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextEditView::TextEditView(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData) : Control(context, parent, xmlData)
{
    MergeFlags(context->GetFlagsAttribute<uint32_t>(xmlData, TextBoxFlags::FlagMap, "flags", 0) | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize);
    m_Text = context->GetAttribute(xmlData, "text", String::zero);

    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::Initialize()
{
    m_CursorTimer.Set(TimeValMicros::FromSeconds(0.8));
    m_CursorTimer.SignalTrigged.Connect(this, &TextEditView::SlotCursorTimer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextEditView::~TextEditView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::OnKeyboardFocusChanged(bool hasFocus)
{
    UpdateCursorTimer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::OnFlagsChanged(uint32_t changedFlags)
{
    Control::OnFlagsChanged(changedFlags);
    if (changedFlags & TextBoxFlags::ReadOnly) {
        if (HasFlags(TextBoxFlags::ReadOnly)) SetKeyboardFocus(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size = GetSizeForString(m_Text, includeWidth, includeHeight);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::Paint(const Rect& updateRect)
{
    SetEraseColor(GetBgColor());
    Rect bounds = GetBounds();
    EraseRect(bounds);

    MovePenTo(0.0f, 0.0f);
    SetFgColor(0, 0, 0);
    DrawString(m_Text);

    if (!HasFlags(TextBoxFlags::ReadOnly) && HasKeyboardFocus() && (m_CursorActive || m_CursorFrozen)) {
        FillRect(GetCursorFrame());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TextEditView::OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (HasFlags(TextBoxFlags::ReadOnly) || m_HitButton != MouseButton_e::None) {
        return View::OnTouchDown(pointID, position, event);
    }
    if (GetBounds().DoIntersect(position))
    {
        m_HitButton = pointID;
        m_HitPos = position;

        SetKeyboardFocus(true);

        SetCursorPos(ViewPosToCursorPos(position));

        m_CursorFrozen = true;
        MakeFocus(pointID, true);
        UpdateCursorTimer();
    }
    else
    {
        SetKeyboardFocus(false);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TextEditView::OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchUp(pointID, position, event);
    }
    m_CursorFrozen = false;
    m_HitButton = MouseButton_e::None;
    MakeFocus(pointID, false);

    UpdateCursorTimer();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TextEditView::OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchMove(pointID, position, event);
    }
    SetCursorPos(ViewPosToCursorPos(position));

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::SetText(const String& text)
{
    m_Text = text;

    PreferredSizeChanged();
    Invalidate();

    SignalTextChanged(m_Text, true, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::InsertText(const String& text)
{
    Rect damageRect = GetBounds();
    damageRect.left = m_CursorViewPos.x;

    m_Text.insert(m_CursorPos, text);
    SetCursorPos(m_CursorPos + text.size());

    PreferredSizeChanged();
    if (damageRect.IsValid()) {
        Invalidate(damageRect);
    }
    SignalTextChanged(m_Text, true, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::Delete()
{
    m_Text.erase(m_Text.begin() + m_CursorPos);
    PreferredSizeChanged();
    Rect damageRect = GetBounds();
    damageRect.left = m_CursorViewPos.x;
    if (damageRect.IsValid()) {
        Invalidate(damageRect);
    }
    SignalTextChanged(m_Text, true, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TextEditView::MoveCursor(int delta)
{
    if (delta < 0)
    {
        if (m_CursorPos > 0)
        {
            size_t newPos = m_CursorPos - 1;
            while (newPos > 0 && !is_first_utf8_byte(m_Text[newPos])) {
                newPos--;
            }
            return SetCursorPos(newPos);
        }
    }
    else
    {
        if (m_CursorPos < m_Text.size())
        {
            size_t newPos = m_CursorPos + utf8_char_length(m_Text[m_CursorPos]);
            if (newPos > m_Text.size()) newPos = m_Text.size();
            return SetCursorPos(newPos);
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TextEditView::SetCursorPos(size_t position)
{
    if (position > m_Text.size()) position = m_Text.size();
    if (position != m_CursorPos)
    {
        Invalidate(GetCursorFrame());
        m_CursorPos = position;
        m_CursorViewPos.x = GetStringWidth(m_Text.c_str(), m_CursorPos);
        Invalidate(GetCursorFrame());

        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t TextEditView::ViewPosToCursorPos(const Point& position) const
{
    Ptr<Font> font = GetFont();
    if (font != nullptr)
    {
        return font->GetStringLength(m_Text, position.x, false);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// Calculate the minimum size the text-box must have to fit the given text.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point TextEditView::GetSizeForString(const String& text, bool includeWidth, bool includeHeight) const
{
    Point size;
    if (includeWidth) {
        size.x = GetStringWidth(text);
    }
    if (includeHeight)
    {
        FontHeight fontHeight = GetFontHeight();
        size.y = fontHeight.descender - fontHeight.ascender;
        if (HasFlags(TextBoxFlags::IncludeLineGap)) {
            size.y += fontHeight.line_gap;
        }
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::HandleKeyPress(KeyCodes keyCode, const String& text)
{
    if (keyCode == KeyCodes::BACKSPACE)
    {
        if (m_CursorPos != 0)
        {
            if (MoveCursor(-1)) {
                Delete();
            }
        }
    }
    if (keyCode == KeyCodes::ENTER)
    {
        SetKeyboardFocus(false);
    }
    else if (!text.empty())
    {
        InsertText(text);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect TextEditView::GetCursorFrame()
{
    FontHeight fontHeight = GetFontHeight();
    const float height = fontHeight.descender - fontHeight.ascender;
    return Rect(m_CursorViewPos.x, m_CursorViewPos.y + 2.0f, m_CursorViewPos.x + 2.0f, m_CursorViewPos.y + height - 4.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::UpdateCursorTimer()
{
    if (!m_CursorFrozen && HasKeyboardFocus())
    {
        if (!m_CursorTimer.IsRunning())
        {
            m_CursorActive = true;
            m_CursorTimer.Start(false, GetLooper());
            Invalidate(GetCursorFrame());
        }
    }
    else
    {
        if (m_CursorTimer.IsRunning())
        {
            m_CursorActive = false;
            m_CursorTimer.Stop();
            Invalidate(GetCursorFrame());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::SlotCursorTimer()
{
    m_CursorActive = !m_CursorActive;
    Invalidate(GetCursorFrame());
}


} // namespace os
