// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/Widgets/TextEditView.h>
#include <GUI/Widgets/TextBox.h>
#include <Utils/UTF8Utils.h>
#include <Math/Misc.h>

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

TextEditView::TextEditView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : Control(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, TextBoxFlags::FlagMap, "flags", 0) | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize);
    m_Text = context.GetAttribute(xmlData, "text", String::zero);

    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::Initialize()
{
    m_Style = TextBox::GetDefaultStyle();
    m_CursorTimer.Set(TimeValNanos::FromSeconds(0.8));
    m_CursorTimer.SignalTrigged.Connect(this, &TextEditView::SlotCursorTimer);

    SetFocusKeyboardMode(FocusKeyboardMode::Alphanumeric);
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
    ContentSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::OnFlagsChanged(uint32_t changedFlags)
{
    Control::OnFlagsChanged(changedFlags);
    if (changedFlags & TextBoxFlags::ReadOnly) {
        if (HasFlags(TextBoxFlags::ReadOnly)) SetKeyboardFocus(false);
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    Point size = GetSizeForString(m_Text, includeWidth, includeHeight);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point TextEditView::CalculateContentSize() const
{
    Point size = GetSizeForString(m_Text, true, true);
    if (!HasFlags(TextBoxFlags::ReadOnly) && HasKeyboardFocus()) {
        size.x += 10.0f;
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::OnPaint(const Rect& updateRect)
{
    if (!IsEnabled())
    {
        SetBgColor(m_Style->DisabledBackgroundColor);
        SetFgColor(m_Style->DisabledTextColor);
    }
    else if (HasFlags(TextBoxFlags::ReadOnly))
    {
        SetBgColor(m_Style->ReadOnlyBackgroundColor);
        SetFgColor(m_Style->ReadOnlyTextColor);
    }
    else
    {
        SetBgColor(m_Style->BackgroundColor);
        SetFgColor(m_Style->TextColor);
    }

    SetEraseColor(GetBgColor());
    Rect bounds = GetBounds();
    EraseRect(bounds);

    MovePenTo(0.0f, 0.0f);
    DrawString(m_Text);

    if (m_CursorVisible) {
        FillRect(GetCursorFrame());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TextEditView::OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (HasFlags(TextBoxFlags::ReadOnly) || m_HitButton != MouseButton_e::None) {
        return false;
    }

    MakeFocus(pointID, true);

    m_HitButton = pointID;
    m_HitPos    = position;

    m_IsScrolling = false;

    if (HasKeyboardFocus())
    {
        m_CursorFrozen = true;
        UpdateCursorTimer();
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TextEditView::OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return false;
    }

    if (m_IsScrolling)
    {
        ViewScroller* scroller = ViewScroller::GetViewScroller(this);
        if (scroller != nullptr) {
            scroller->EndSwipe();
        }
    }
    else
    {
        if (!m_IsDraggingCursor) {
            SetCursorPos(ViewPosToCursorPos(position));
        }
        SetKeyboardFocus(true);
    }
    m_IsScrolling       = false;
    m_IsDraggingCursor  = false;
    m_CursorFrozen      = false;
    m_HitButton         = MouseButton_e::None;

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
        return false;
    }

    if (HasKeyboardFocus())
    {
        if (!m_IsDraggingCursor && (position - m_HitPos).LengthSqr() > square(BEGIN_DRAG_OFFSET)) {
            m_IsDraggingCursor = true;
            m_HitPos.x -= m_CursorViewPos.x;
        }
        if (m_IsDraggingCursor) {
            SetCursorPos(ViewPosToCursorPos(position - m_HitPos));
        }
    }
    else if (!m_IsScrolling)
    {
        if ((position - m_HitPos).LengthSqr() > square(BEGIN_DRAG_OFFSET))
        {
            m_IsScrolling = true;
            ViewScroller* scroller = ViewScroller::GetViewScroller(this);
            if (scroller != nullptr) {
                scroller->BeginSwipe(ConvertToScreen(m_HitPos));
            }
        }
    }
    else
    {
        ViewScroller* scroller = ViewScroller::GetViewScroller(this);
        if (scroller != nullptr) {
            scroller->SwipeMove(ConvertToScreen(position));
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::OnKeyUp(KeyCodes keyCode, const String& text, const KeyEvent& event)
{
    if (keyCode == KeyCodes::CURSOR_LEFT)
    {
        MoveCursor(-1);
    }
    else if (keyCode == KeyCodes::CURSOR_RIGHT)
    {
        MoveCursor(1);
    }
    else if (keyCode == KeyCodes::BACKSPACE)
    {
        if (m_CursorPos != 0)
        {
            if (MoveCursor(-1)) {
                Delete();
            }
        }
    }
    else if (keyCode == KeyCodes::ENTER)
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

void TextEditView::SetText(const String& text, bool sendEvent)
{
    m_Text = text;

    PreferredSizeChanged();
    ContentSizeChanged();
    Invalidate();

    if (sendEvent) {
        SignalTextChanged(m_Text, true, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::InsertText(const String& text, bool sendEvent)
{
    Rect damageRect = GetBounds();
    damageRect.left = m_CursorViewPos.x;

    m_Text.insert(m_CursorPos, text);
    SetCursorPos(m_CursorPos + text.size());

    PreferredSizeChanged();
    ContentSizeChanged();
    if (damageRect.IsValid()) {
        Invalidate(damageRect);
    }
    if (sendEvent) {
        SignalTextChanged(m_Text, true, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::Delete()
{
    m_Text.erase(m_Text.begin() + m_CursorPos);
    PreferredSizeChanged();
    ContentSizeChanged();
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
        if (m_CursorVisible) {
            Invalidate(GetCursorFrame());
        }
        m_CursorPos = position;
        m_CursorViewPos.x = GetStringWidth(m_Text.c_str(), m_CursorPos);

        if (m_CursorTimer.IsRunning())
        {
            m_CursorTimer.Stop();
            m_CursorVisible = true;
            m_CursorTimer.Start(false, GetLooper());
        }
        if (m_CursorVisible) {
            Invalidate(GetCursorFrame());
        }
        if (HasKeyboardFocus())
        {
            Rect bounds = GetBounds();
            float maxScroll = bounds.Width() - GetContentSize().x;
            if (maxScroll < 0.0f)
            {
                float prevOffset = GetScrollOffset().x;
                float offset = prevOffset;
                if (m_CursorViewPos.x < bounds.left + 20.0f)
                {
                    offset = std::clamp(20.0f - m_CursorViewPos.x, maxScroll, 0.0f);
                }
                else if (m_CursorViewPos.x > bounds.right - 20.0f)
                {
                    offset = std::clamp(bounds.Width() - m_CursorViewPos.x - 20.0f, maxScroll, 0.0f);
                }
                if (offset != prevOffset)
                {
                    m_HitPos.x += prevOffset - offset;
                    ScrollTo(offset, 0.0f);
                }
            }
            else
            {
                ScrollTo(0.0f, 0.0f);
            }
        }
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

Ptr<const TextBoxStyle> TextEditView::GetStyle() const
{
    return m_Style;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::SetStyle(Ptr<const TextBoxStyle> style)
{
    if (style != nullptr) {
        m_Style = style;
    } else {
        m_Style = TextBox::GetDefaultStyle();
    }
    Invalidate();
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
    const bool hasFocus = !HasFlags(TextBoxFlags::ReadOnly) && HasKeyboardFocus();
    bool showCursor = m_CursorVisible;
    if (!m_CursorFrozen && hasFocus)
    {
        if (!m_CursorTimer.IsRunning())
        {
            showCursor = true;
            m_CursorTimer.Start(false, GetLooper());
        }
    }                                                                                                                                
    else
    {
        if (m_CursorTimer.IsRunning())
        {
            m_CursorTimer.Stop();
        }
        showCursor = hasFocus; // false if no focus, true if frozen.
    }
    if (showCursor != m_CursorVisible)
    {
        m_CursorVisible = showCursor;
        Invalidate(GetCursorFrame());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextEditView::SlotCursorTimer()
{
    m_CursorVisible = !m_CursorVisible;
    Invalidate(GetCursorFrame());
}


} // namespace os
