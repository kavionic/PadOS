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

#pragma once

#include <GUI/Widgets/Control.h>
#include <Threads/EventTimer.h>


struct PTextBoxStyle;

class PTextEditView : public PControl
{
public:
    PTextEditView(const PString& name, const PString& text, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PTextEditView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);
    ~PTextEditView();

    // From View:
    virtual void    OnKeyboardFocusChanged(bool hasFocus) override;
    virtual void    OnFlagsChanged(uint32_t changedFlags) override;
    virtual void    CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual PPoint   CalculateContentSize() const override;
    virtual void    OnPaint(const PRect& updateRect) override;

    virtual bool    OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;

    virtual void OnKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& event) override;

    // From TextEditView:
    void                    SetText(const PString& text, bool sendEvent = true);
    void                    InsertText(const PString& text, bool sendEvent = true);
    void                    Delete();

    const PString&          GetText() const { return m_Text; }

    bool                    MoveCursor(int delta);
    bool                    SetCursorPos(size_t position);
    size_t                  ViewPosToCursorPos(const PPoint& position) const;

    PPoint                   GetSizeForString(const PString& text, bool includeWidth = true, bool includeHeight = true) const;

    Ptr<const PTextBoxStyle> GetStyle() const;
    void                    SetStyle(Ptr<const PTextBoxStyle> style);

    Signal<void, const PString&, bool, PTextEditView*> SignalTextChanged; //(const PString& newText, bool finalEdit, TextEditView* source)
private:
    void Initialize();
    PRect GetCursorFrame();
    void UpdateCursorTimer();
    void SlotCursorTimer();

    PString                 m_Text;
    size_t                  m_CursorPos = 0;
    PPoint                   m_CursorViewPos;
    PEventTimer              m_CursorTimer;
    bool                    m_CursorVisible = false;
    bool                    m_CursorFrozen = false;
    bool                    m_IsScrolling = false;
    bool                    m_IsDraggingCursor = false;
    PMouseButton           m_HitButton = PMouseButton::None;
    PPoint                   m_HitPos;

    Ptr<const PTextBoxStyle> m_Style;

    PTextEditView(const PTextEditView&) = delete;
    PTextEditView& operator=(const PTextEditView&) = delete;
};
