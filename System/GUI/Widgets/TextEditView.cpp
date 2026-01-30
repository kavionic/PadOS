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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextEditView::PTextEditView(const PString& name, const PString& text, Ptr<PView> parent, uint32_t flags) : PControl(name, parent, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize), m_Text(text)
{
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextEditView::PTextEditView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PControl(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, PTextBoxFlags::FlagMap, "flags", 0) | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize);
    m_Text = context.GetAttribute(xmlData, "text", PString::zero);

    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::Initialize()
{
    m_Style = PTextBox::GetDefaultStyle();
    m_CursorTimer.Set(TimeValNanos::FromSeconds(0.8));
    m_CursorTimer.SignalTrigged.Connect(this, &PTextEditView::SlotCursorTimer);

    SetFocusKeyboardMode(PFocusKeyboardMode::Alphanumeric);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextEditView::~PTextEditView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::OnKeyboardFocusChanged(bool hasFocus)
{
    UpdateCursorTimer();
    ContentSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::OnFlagsChanged(uint32_t changedFlags)
{
    PControl::OnFlagsChanged(changedFlags);
    if (changedFlags & PTextBoxFlags::ReadOnly) {
        if (HasFlags(PTextBoxFlags::ReadOnly)) SetKeyboardFocus(false);
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PPoint size = GetSizeForString(m_Text, includeWidth, includeHeight);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PTextEditView::CalculateContentSize() const
{
    PPoint size = GetSizeForString(m_Text, true, true);
    if (!HasFlags(PTextBoxFlags::ReadOnly) && HasKeyboardFocus()) {
        size.x += 10.0f;
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::OnPaint(const PRect& updateRect)
{
    if (!IsEnabled())
    {
        SetBgColor(m_Style->DisabledBackgroundColor);
        SetFgColor(m_Style->DisabledTextColor);
    }
    else if (HasFlags(PTextBoxFlags::ReadOnly))
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
    PRect bounds = GetBounds();
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

bool PTextEditView::OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (HasFlags(PTextBoxFlags::ReadOnly) || m_HitButton != PMouseButton::None) {
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

bool PTextEditView::OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (pointID != m_HitButton) {
        return false;
    }

    if (m_IsScrolling)
    {
        PViewScroller* scroller = PViewScroller::GetViewScroller(this);
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
    m_HitButton         = PMouseButton::None;

    MakeFocus(pointID, false);

    UpdateCursorTimer();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTextEditView::OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (pointID != m_HitButton) {
        return false;
    }

    if (HasKeyboardFocus())
    {
        if (!m_IsDraggingCursor && (position - m_HitPos).LengthSqr() > PMath::square(BEGIN_DRAG_OFFSET)) {
            m_IsDraggingCursor = true;
            m_HitPos.x -= m_CursorViewPos.x;
        }
        if (m_IsDraggingCursor) {
            SetCursorPos(ViewPosToCursorPos(position - m_HitPos));
        }
    }
    else if (!m_IsScrolling)
    {
        if ((position - m_HitPos).LengthSqr() > PMath::square(BEGIN_DRAG_OFFSET))
        {
            m_IsScrolling = true;
            PViewScroller* scroller = PViewScroller::GetViewScroller(this);
            if (scroller != nullptr) {
                scroller->BeginSwipe(ConvertToScreen(m_HitPos));
            }
        }
    }
    else
    {
        PViewScroller* scroller = PViewScroller::GetViewScroller(this);
        if (scroller != nullptr) {
            scroller->SwipeMove(ConvertToScreen(position));
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::OnKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& event)
{
    if (keyCode == PKeyCodes::CURSOR_LEFT)
    {
        MoveCursor(-1);
    }
    else if (keyCode == PKeyCodes::CURSOR_RIGHT)
    {
        MoveCursor(1);
    }
    else if (keyCode == PKeyCodes::BACKSPACE)
    {
        if (m_CursorPos != 0)
        {
            if (MoveCursor(-1)) {
                Delete();
            }
        }
    }
    else if (keyCode == PKeyCodes::ENTER)
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

void PTextEditView::SetText(const PString& text, bool sendEvent)
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

void PTextEditView::InsertText(const PString& text, bool sendEvent)
{
    PRect damageRect = GetBounds();
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

void PTextEditView::Delete()
{
    m_Text.erase(m_Text.begin() + m_CursorPos);
    PreferredSizeChanged();
    ContentSizeChanged();
    PRect damageRect = GetBounds();
    damageRect.left = m_CursorViewPos.x;
    if (damageRect.IsValid()) {
        Invalidate(damageRect);
    }
    SignalTextChanged(m_Text, true, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTextEditView::MoveCursor(int delta)
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

bool PTextEditView::SetCursorPos(size_t position)
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
            PRect bounds = GetBounds();
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

size_t PTextEditView::ViewPosToCursorPos(const PPoint& position) const
{
    Ptr<PFont> font = GetFont();
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

PPoint PTextEditView::GetSizeForString(const PString& text, bool includeWidth, bool includeHeight) const
{
    PPoint size;
    if (includeWidth) {
        size.x = GetStringWidth(text);
    }
    if (includeHeight)
    {
        PFontHeight fontHeight = GetFontHeight();
        size.y = fontHeight.descender - fontHeight.ascender;
        if (HasFlags(PTextBoxFlags::IncludeLineGap)) {
            size.y += fontHeight.line_gap;
        }
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PTextBoxStyle> PTextEditView::GetStyle() const
{
    return m_Style;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::SetStyle(Ptr<const PTextBoxStyle> style)
{
    if (style != nullptr) {
        m_Style = style;
    } else {
        m_Style = PTextBox::GetDefaultStyle();
    }
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PTextEditView::GetCursorFrame()
{
    PFontHeight fontHeight = GetFontHeight();
    const float height = fontHeight.descender - fontHeight.ascender;
    return PRect(m_CursorViewPos.x, m_CursorViewPos.y + 2.0f, m_CursorViewPos.x + 2.0f, m_CursorViewPos.y + height - 4.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextEditView::UpdateCursorTimer()
{
    const bool hasFocus = !HasFlags(PTextBoxFlags::ReadOnly) && HasKeyboardFocus();
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

void PTextEditView::SlotCursorTimer()
{
    m_CursorVisible = !m_CursorVisible;
    Invalidate(GetCursorFrame());
}
