// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.08.2020 15:30

#pragma once

#include <GUI/Style/ViewStyleTextBox.h>
#include <GUI/ViewScroller.h>
#include <GUI/Widgets/Control.h>
#include <GUI/Widgets/TextEditView.h>


enum class PTextBoxFlag : uint32_t
{
    IncludeLineGap       = 0x01 << PVIEW_FIRST_USER_FLAG_BIT,
    RaisedFrame          = 0x02 << PVIEW_FIRST_USER_FLAG_BIT,
    ReadOnly             = 0x04 << PVIEW_FIRST_USER_FLAG_BIT,
};

PDEFINE_ENUM_BITMASK(PTextBoxFlags, PTextBoxFlag, PViewFlags);

extern const std::map<PString, PTextBoxFlags> PTEXT_BOX_FLAG_MAP;

class PTextBox : public PControl, public PViewScroller, public PViewStyleTextBox
{
public:
    PTextBox(const PString& name, const PString& text, PTextBoxFlags flags = PTextBoxFlags::zero);
    PTextBox(PViewFactoryContext& context, const pugi::xml_node& xmlData);
    ~PTextBox();

    virtual const PTextBoxStyleData& GetTextBoxStyleData() const override { return m_TextBoxStyleData; }

    // From View:
    virtual void    OnFlagsChanged(PViewFlags changedFlags) override;
    virtual void    OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void    OnStyleChanged() override;
    virtual void    CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void    OnFrameSized(const PPoint& delta) override;
    virtual void    OnPaint(const PRect& updateRect) override;

    virtual void    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;

    // From Control:
    virtual void    OnEnableStatusChanged(bool isEnabled) override;

    // From TextBox:
    void            SetText(const PString& text, bool sendEvent = true) { m_Editor->SetText(text, sendEvent); }
    const PString&  GetText() const { return m_Editor->GetText(); }

    PPoint           GetSizeForString(const PString& text, bool includeWidth = true, bool includeHeight = true) const;

    Signal<void(const PString& newText, bool finalUpdate, PTextBox* source)> SignalTextChanged;
private:
    void Initialize(const PString& text);
    void LayoutEditor();
    void UpdateEditorColors();
    void NotifyStylePropertyChanged(PTextBoxProperties properties);
    void SlotEditorPreferredSizeChanged();
    void SlotTextChanged(const PString& text, bool finalUpdate);

    Ptr<PTextEditView> m_Editor;
    Ptr<PTextEditColors> m_EditorColors;
    PTextBoxStyleData m_TextBoxStyleData;

    PTextBox(const PTextBox&) = delete;
    PTextBox& operator=(const PTextBox&) = delete;
};
