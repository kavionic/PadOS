// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.08.2020 15:30

#include <GUI/Widgets/TextBox.h>
#include <System/ExceptionHandling.h>
#include <Utils/XMLObjectParser.h>


const std::map<PString, PTextBoxFlags> PTEXT_BOX_FLAG_MAP
{
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlag, IncludeLineGap),
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlag, RaisedFrame),
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlag, ReadOnly)
};


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBox::PTextBox(const PString& name, const PString& text, PTextBoxFlags flags)
    : PControl(name, flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize), PViewStyleTextBox(this)
{
    Initialize(text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBox::PTextBox(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PControl(context, xmlData), PViewStyleTextBox(this)
{
    const PTextBoxFlags flags = context.GetFlagsAttribute<PTextBoxFlags>(xmlData, PTEXT_BOX_FLAG_MAP, "flags", PTextBoxFlags::zero);
    MergeFlags(flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize);
    Initialize(context.GetAttribute(xmlData, "text", PString::zero));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBox::~PTextBox()
{
    m_Editor->SignalPreferredSizeChanged.Disconnect(this, &PTextBox::SlotEditorPreferredSizeChanged);
    ReleaseRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnFlagsChanged(PViewFlags changedFlags)
{
    PControl::OnFlagsChanged(changedFlags);
    if (m_Editor != nullptr)
    {
        m_Editor->ReplaceFlags(GetFlags<PTextBoxFlags>());
        const PTextBoxFlags textBoxFlags(changedFlags);
        PTextBoxProperties properties;
        if (textBoxFlags.Has(PTextBoxFlag::ReadOnly))
        {
            m_TextBoxStyleData.IsReadOnly = HasFlags(PTextBoxFlag::ReadOnly);
            properties |= PTextBoxProperty::ReadOnly;
        }
        if (textBoxFlags.Has(PTextBoxFlag::RaisedFrame))
        {
            m_TextBoxStyleData.HasRaisedFrame = HasFlags(PTextBoxFlag::RaisedFrame);
            properties |= PTextBoxProperty::RaisedFrame;
        }
        if (textBoxFlags.Has(PTextBoxFlag::IncludeLineGap)) {
            m_Editor->PreferredSizeChanged();
        }
        if (properties != PTextBoxProperties::zero) {
            NotifyStylePropertyChanged(properties);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnFontChanged(Ptr<const PFont> newFont)
{
    m_Editor->SetFont(newFont);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnStyleChanged()
{
    PControl::OnStyleChanged();
    UpdateEditorColors();
    LayoutEditor();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PViewRendererTextBox* renderer = GetRenderer();
    if (renderer != nullptr)
    {
        renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
    }
    else
    {
        *minSize = m_TextBoxStyleData.EditorSize;
        *maxSize = *minSize;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnFrameSized(const PPoint& delta)
{
    PControl::OnFrameSized(delta);
    NotifyStylePropertyChanged(PTextBoxProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::OnPaint(const PRect& updateRect)
{
    const PViewRendererTextBox* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
    } else {
        PView::OnPaint(updateRect);
    }
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

void PTextBox::OnEnableStatusChanged(bool isEnabled)
{
    m_TextBoxStyleData.IsEnabled = isEnabled;
    m_Editor->SetEnable(isEnabled);
    NotifyStylePropertyChanged(PTextBoxProperty::Enabled);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PTextBox::GetSizeForString(const PString& text, bool includeWidth, bool includeHeight) const
{
    const PPoint contentSize = m_Editor->GetSizeForString(text, includeWidth, includeHeight);
    const PViewRendererTextBox* renderer = GetRenderer();
    return (renderer != nullptr) ? renderer->GetSizeForContent(*this, contentSize, GetRendererContext()) : contentSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::Initialize(const PString& text)
{
    m_TextBoxStyleData.IsReadOnly = HasFlags(PTextBoxFlag::ReadOnly);
    m_TextBoxStyleData.HasRaisedFrame = HasFlags(PTextBoxFlag::RaisedFrame);
    m_TextBoxStyleData.IsEnabled = IsEnabled();
    m_Editor = ptr_new<PTextEditView>("Editor", text, GetFlags<PTextBoxFlags>());
    m_EditorColors = ptr_new<PTextEditColors>();
    m_Editor->SetFont(GetFont());
    m_Editor->SetEnable(IsEnabled());
    AddChild(m_Editor);
    m_Editor->PreferredSizeChanged();
    m_TextBoxStyleData.EditorSize = m_Editor->GetPreferredSize(PPrefSizeType::Smallest);

    SetMaxOverscroll(40.0f, 0.0f);
    SetScrolledView(m_Editor);
    UpdateEditorColors();

    m_Editor->SignalPreferredSizeChanged.Connect(this, &PTextBox::SlotEditorPreferredSizeChanged);
    m_Editor->SignalTextChanged.Connect(this, &PTextBox::SlotTextChanged);

    InitializeRenderer();
    PScopeFail releaseRenderer([this] { ReleaseRenderer(); });
    LayoutEditor();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::LayoutEditor()
{
    const PViewRendererTextBox* renderer = GetRenderer();
    const PRect frame = (renderer != nullptr) ? renderer->GetEditorFrame(*this, GetRendererContext()) : GetBounds();
    if (m_Editor->GetFrame() != frame) {
        m_Editor->SetFrame(frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::UpdateEditorColors()
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        const PColor backgroundColor = style->GetColor(PViewRendererTextBox::GetBackgroundColorID(m_TextBoxStyleData));
        const PColor textColor = style->GetColor(PViewRendererTextBox::GetTextColorID(m_TextBoxStyleData));
        if (m_Editor->GetColors() != m_EditorColors || m_EditorColors->BackgroundColor != backgroundColor || m_EditorColors->TextColor != textColor)
        {
            m_EditorColors->BackgroundColor = backgroundColor;
            m_EditorColors->ReadOnlyBackgroundColor = backgroundColor;
            m_EditorColors->DisabledBackgroundColor = backgroundColor;
            m_EditorColors->TextColor = textColor;
            m_EditorColors->ReadOnlyTextColor = textColor;
            m_EditorColors->DisabledTextColor = textColor;
            m_Editor->SetColors(m_EditorColors);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::NotifyStylePropertyChanged(PTextBoxProperties properties)
{
    const PViewRendererTextBox* renderer = GetRenderer();
    if (renderer != nullptr)
    {
        renderer->PropertyChanged(*this, properties, GetRendererContext());
    }
    else
    {
        PreferredSizeChanged();
        Invalidate();
    }
    if (properties.HasAny(PTextBoxProperties(PTextBoxProperty::ReadOnly, PTextBoxProperty::Enabled))) {
        UpdateEditorColors();
    }
    LayoutEditor();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::SlotEditorPreferredSizeChanged()
{
    const PPoint size = m_Editor->GetPreferredSize(PPrefSizeType::Smallest);
    if (m_TextBoxStyleData.EditorSize != size)
    {
        m_TextBoxStyleData.EditorSize = size;
        NotifyStylePropertyChanged(PTextBoxProperty::EditorSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::SlotTextChanged(const PString& text, bool finalUpdate)
{
    SignalTextChanged(text, finalUpdate, this);
}
