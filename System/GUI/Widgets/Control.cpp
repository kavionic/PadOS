// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/Widgets/Control.h>
#include <Utils/XMLFactory.h>
#include <Utils/XMLObjectParser.h>

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Control::Control(const String& name, Ptr<View> parent, uint32_t flags)
    : View(name, parent, flags )
{
    m_IsEnabled	= true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Control::Control(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData, Alignment defaultLabelAlignment) : View(context, parent, xmlData)
{
    m_Label             = context->GetAttribute(xmlData, "label", String::zero);
    m_LabelAlignment    = context->GetAttribute(xmlData, "label_alignment", defaultLabelAlignment);
    m_IsEnabled         = context->GetAttribute<bool>(xmlData, "enabled", true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Control::~Control()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Control::SetEnable(bool enable)
{
    if (m_IsEnabled != enable) {
        m_IsEnabled = enable;
        OnEnableStatusChanged(m_IsEnabled);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Control::IsEnabled() const
{
    return m_IsEnabled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Control::SetLabel(const String& label)
{
    if (label != m_Label)
    {
	m_Label = label;
	OnLabelChanged(m_Label);
    }
}

