// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.08.2020 15:30

#include <System/Platform.h>

#include <GUI/Widgets/TextBox.h>
#include <Utils/Utils.h>
#include <Utils/XMLObjectParser.h>


const std::map<PString, PTextBoxFlags> PTEXT_BOX_FLAG_MAP
{
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlag, IncludeLineGap),
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlag, RaisedFrame),
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlag, ReadOnly)
};

NoPtr<PTextBoxStyle> PTextBox::s_DefaultStyle;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBox::PTextBox(const PString& name, const PString& text, PTextBoxFlags flags)
    : PControl(name, flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize)
{
    Initialize(text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBox::PTextBox(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PControl(context, xmlData)
{
    Initialize(context.GetAttribute(xmlData, "text", PString::zero));
    const PTextBoxFlags flags = context.GetFlagsAttribute<PTextBoxFlags>(xmlData, PTEXT_BOX_FLAG_MAP, "flags", PTextBoxFlags::zero);
    MergeFlags(flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::Initialize(const PString& text)
{
    m_Editor = ptr_new<PTextEditView>("Editor", text, GetFlags<PTextBoxFlags>());
    AddChild(m_Editor);
    m_Editor->SetBorders(4.0f, 4.0f, 4.0f, 4.0f);

    SetMaxOverscroll(40.0f, 0.0f);
    SetScrolledView(m_Editor);

    m_Editor->SignalTextChanged.Connect(this, &PTextBox::SlotTextChanged);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::SlotTextChanged(const PString& text, bool finalUpdate)
{
    SignalTextChanged(text, finalUpdate, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnFlagsChanged(PViewFlags changedFlags)
{
    PControl::OnFlagsChanged(changedFlags);
    m_Editor->ReplaceFlags(GetFlags<PTextBoxFlags>());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnFontChanged(Ptr<const PFont> newFont)
{
    m_Editor->SetFont(newFont);
    PreferredSizeChanged();
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PPoint size = m_Editor->GetPreferredSize(PPrefSizeType::Smallest);
    PRect borders = m_Editor->GetBorders();
    size.x += borders.left + borders.right;
    size.y += borders.top + borders.bottom;

    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnFrameSized(const PPoint& delta)
{
    PRect frame = GetBounds();
    PRect borders = m_Editor->GetBorders();
    frame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
    m_Editor->SetFrame(frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnPaint(const PRect& updateRect)
{
    Ptr<const PTextBoxStyle> style = GetStyle();
    if (!IsEnabled()) {
        SetEraseColor(style->DisabledBackgroundColor);
    } else if (HasFlags(PTextBoxFlag::ReadOnly)) {
        SetEraseColor(style->ReadOnlyBackgroundColor);
    } else {
        SetEraseColor(style->BackgroundColor);
    }

    PRect bounds = GetBounds();
    DrawFrame(bounds, HasFlags(PTextBoxFlag::RaisedFrame) ? FRAME_RAISED : FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    m_Editor->OnPointerDown(pointerID, m_Editor->ConvertFromParent(position), event, phase);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    m_Editor->OnPointerUp(pointerID, m_Editor->ConvertFromParent(position), event, phase);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    m_Editor->OnPointerMove(pointerID, m_Editor->ConvertFromParent(position), event, phase);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PTextBox::GetSizeForString(const PString& text, bool includeWidth, bool includeHeight) const
{
    PPoint size = m_Editor->GetSizeForString(text, includeWidth, includeHeight);
    PRect  borders = m_Editor->GetBorders();

    size.x += borders.left + borders.right;
    size.y += borders.top + borders.bottom;
    return size;
}
