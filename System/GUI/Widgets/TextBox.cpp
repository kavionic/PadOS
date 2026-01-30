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
// Created: 22.08.2020 15:30

#include <System/Platform.h>

#include <GUI/Widgets/TextBox.h>
#include <Utils/Utils.h>
#include <Utils/XMLObjectParser.h>


const std::map<PString, uint32_t> PTextBoxFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlags, IncludeLineGap),
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlags, RaisedFrame),
    DEFINE_FLAG_MAP_ENTRY(PTextBoxFlags, ReadOnly)
};

NoPtr<PTextBoxStyle> PTextBox::s_DefaultStyle;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBox::PTextBox(const PString& name, const PString& text, Ptr<PView> parent, uint32_t flags) : PControl(name, parent, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    Initialize(text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBox::PTextBox(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PControl(context, parent, xmlData)
{
    Initialize(context.GetAttribute(xmlData, "text", PString::zero));
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, PTextBoxFlags::FlagMap, "flags", 0) | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextBox::Initialize(const PString& text)
{
    m_Editor = ptr_new<PTextEditView>("Editor", text, ptr_tmp_cast(this), GetFlags());
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

void PTextBox::OnFlagsChanged(uint32_t changedFlags)
{
    PControl::OnFlagsChanged(changedFlags);
    m_Editor->ReplaceFlags(GetFlags());
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
    } else if (HasFlags(PTextBoxFlags::ReadOnly)) {
        SetEraseColor(style->ReadOnlyBackgroundColor);
    } else {
        SetEraseColor(style->BackgroundColor);
    }

    PRect bounds = GetBounds();
    DrawFrame(bounds, HasFlags(PTextBoxFlags::RaisedFrame) ? FRAME_RAISED : FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTextBox::OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    return m_Editor->OnTouchDown(pointID, m_Editor->ConvertFromParent(position), event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTextBox::OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    return m_Editor->OnTouchUp(pointID, m_Editor->ConvertFromParent(position), event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTextBox::OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    return m_Editor->OnTouchMove(pointID, m_Editor->ConvertFromParent(position), event);
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
