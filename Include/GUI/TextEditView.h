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

#pragma once

#include <GUI/Control.h>
#include <Threads/EventTimer.h>

namespace os
{

struct TextBoxStyle;

class TextEditView : public Control
{
public:
    TextEditView(const String& name, const String& text, Ptr<View> parent = nullptr, uint32_t flags = 0);
    TextEditView(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);
    ~TextEditView();

    // From View:
    virtual void    OnKeyboardFocusChanged(bool hasFocus) override;
    virtual void    OnFlagsChanged(uint32_t changedFlags) override;
    virtual void    CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;
    virtual Point   GetContentSize() const override;
    virtual void    Paint(const Rect& updateRect) override;

    virtual bool    OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;

    virtual void OnKeyUp(KeyCodes keyCode, const String& text, const KeyEvent& event) override;

    // From TextEditView:
    void                    SetText(const String& text);
    void                    InsertText(const String& text);
    void                    Delete();

    const String&           GetText() const { return m_Text; }

    bool                    MoveCursor(int delta);
    bool                    SetCursorPos(size_t position);
    size_t                  ViewPosToCursorPos(const Point& position) const;

    Point                   GetSizeForString(const String& text, bool includeWidth = true, bool includeHeight = true) const;

    Ptr<const TextBoxStyle> GetStyle() const;
    void                    SetStyle(Ptr<const TextBoxStyle> style);

    Signal<void, const String&, bool, TextEditView*> SignalTextChanged;
private:
    void Initialize();
    Rect GetCursorFrame();
    void UpdateCursorTimer();
    void SlotCursorTimer();

    String                  m_Text;
    size_t                  m_CursorPos = 0;
    Point                   m_CursorViewPos;
    EventTimer              m_CursorTimer;
    bool                    m_CursorVisible = false;
    bool                    m_CursorFrozen = false;
    bool                    m_IsScrolling = false;
    bool                    m_IsDraggingCursor = false;
    MouseButton_e           m_HitButton = MouseButton_e::None;
    Point                   m_HitPos;

    Ptr<const TextBoxStyle> m_Style;

    TextEditView(const TextEditView&) = delete;
    TextEditView& operator=(const TextEditView&) = delete;
};


} // namespace os
