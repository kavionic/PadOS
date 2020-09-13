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

#pragma once

#include "View.h"

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// Base class for GUI controls.
/// \ingroup gui
/// \par Description:
///	
/// \sa os::View
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


class Control : public View
{
public:
    static constexpr int32_t INVALID_ID = -1;

    Control(const String& name, Ptr<View> parent = nullptr, uint32_t flags = ViewFlags::WillDraw | ViewFlags::ClearBackground);
    Control(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData, Alignment defaultLabelAlignment = Alignment::Center);
    ~Control();

      // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) { Invalidate(); Flush(); }
    virtual void OnLabelChanged(const String& label) { Invalidate(); Flush(); PreferredSizeChanged(); }

    virtual void SetEnable(bool enabled);
    virtual bool IsEnabled() const;

    void	SetLabel(const String& label);
    String	GetLabel() const { return m_Label; }
    
    void        SetLabelAlignment(Alignment alignment) { m_LabelAlignment = alignment; PreferredSizeChanged(); Invalidate(); Flush(); }
    Alignment   GetLabelAlignment() const { return m_LabelAlignment; }

    void    SetID(int32_t ID) { m_ID = ID; }
    int32_t GetID() const { return m_ID; }

private:
    int32_t m_ID = INVALID_ID;
    String      m_Label;
	Alignment   m_LabelAlignment = Alignment::Center;
    bool    m_IsEnabled;
};

}
