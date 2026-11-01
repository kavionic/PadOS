// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.08.2020 15:30

#pragma once

#include <GUI/Widgets/Control.h>
#include <GUI/Widgets/TextEditView.h>
#include <GUI/ViewScroller.h>
#include <Ptr/NoPtr.h>


enum class PTextBoxFlag : uint32_t
{
    IncludeLineGap       = 0x01 << PVIEW_FIRST_USER_FLAG_BIT,
    RaisedFrame          = 0x02 << PVIEW_FIRST_USER_FLAG_BIT,
    ReadOnly             = 0x04 << PVIEW_FIRST_USER_FLAG_BIT,
};

PDEFINE_ENUM_BITMASK(PTextBoxFlags, PTextBoxFlag, PViewFlags);

extern const std::map<PString, PTextBoxFlags> PTEXT_BOX_FLAG_MAP;

struct PTextBoxStyle : PtrTarget
{
    PColor   BackgroundColor         = PColor(PNamedColors::white);
    PColor   TextColor               = PColor(PNamedColors::black);
    PColor   ReadOnlyBackgroundColor = PColor(PNamedColors::darkgray);
    PColor   ReadOnlyTextColor       = PColor(PNamedColors::black);
    PColor   DisabledBackgroundColor = PColor(PNamedColors::darkgray);
    PColor   DisabledTextColor       = PColor(PNamedColors::dimgray);
};

class PTextBox : public PControl, public PViewScroller
{
public:
    PTextBox(const PString& name, const PString& text, PTextBoxFlags flags = PTextBoxFlags::zero);
    PTextBox(PViewFactoryContext& context, const pugi::xml_node& xmlData);

    static Ptr<PTextBoxStyle> GetDefaultStyle() { return s_DefaultStyle; }

    // From View:
    virtual void    OnFlagsChanged(PViewFlags changedFlags) override;
    virtual void    OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void    CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void    OnFrameSized(const PPoint& delta) override;
    virtual void    OnPaint(const PRect& updateRect) override;

    virtual void    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;

    // From TextBox:
    void            SetText(const PString& text, bool sendEvent = true) { m_Editor->SetText(text, sendEvent); }
    const PString&  GetText() const { return m_Editor->GetText(); }

    PPoint           GetSizeForString(const PString& text, bool includeWidth = true, bool includeHeight = true) const;

    Ptr<const PTextBoxStyle> GetStyle() const { return m_Editor->GetStyle(); }
    void                    SetStyle(Ptr<const PTextBoxStyle> style) { m_Editor->SetStyle(style); }

    Signal<void, const PString&, bool, PTextBox*> SignalTextChanged; //(const PString& newText, bool finalUpdate, TextBox* source)
private:
    void Initialize(const PString& text);

    void SlotTextChanged(const PString& text, bool finalUpdate);

    static NoPtr<PTextBoxStyle> s_DefaultStyle;


    Ptr<PTextEditView> m_Editor;

    PTextBox(const PTextBox&) = delete;
    PTextBox& operator=(const PTextBox&) = delete;
};
