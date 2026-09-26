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


namespace PTextBoxFlags
{
static constexpr uint32_t IncludeLineGap = 0x01 << PViewFlags::FirstUserBit;
static constexpr uint32_t RaisedFrame    = 0x02 << PViewFlags::FirstUserBit;
static constexpr uint32_t ReadOnly       = 0x04 << PViewFlags::FirstUserBit;

extern const std::map<PString, uint32_t> FlagMap;
}

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
    PTextBox(const PString& name, const PString& text, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PTextBox(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    static Ptr<PTextBoxStyle> GetDefaultStyle() { return s_DefaultStyle; }

    // From View:
    virtual void    OnFlagsChanged(uint32_t changedFlags) override;
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
