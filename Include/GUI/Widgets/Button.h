// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 02.04.2018 13:08:47

#pragma once

#include <GUI/Widgets/ButtonBase.h>

namespace os
{

class Button : public ButtonBase
{
public:
    Button(const String& name, const String& label, Ptr<View> parent = nullptr, uint32_t flags = 0);
	Button(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);
    ~Button();

    // From View:
    virtual void AllAttachedToScreen() override { Invalidate(); }
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void Paint(const Rect& updateRect) override;

	// From Control:
    virtual void OnEnableStatusChanged(bool bIsEnabled) override { Invalidate(); Flush(); }
	virtual void OnLabelChanged(const String& label) override;

private:
    void UpdateLabelSize();
    Point  m_LabelSize;
        
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;
};
    
    
} // namespace
