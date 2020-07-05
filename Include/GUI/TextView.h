// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 15.04.2018 16:38:59

#pragma once

#include "GUI/View.h"


namespace os
{

namespace TextViewFlags
{
static constexpr uint32_t IncludeLineGap = 0x01 << ViewFlags::FirstUserBit;
}

class TextView : public View
{
public:
    TextView(const String& name, const String& text, Ptr<View> parent = nullptr, uint32_t flags = 0);
	TextView(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);
    ~TextView();
    
    void SetText(const String& text);
    
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;

    virtual void Paint(const Rect& updateRect) override;
        
private:
    String m_Text;
    
    TextView(const TextView&) = delete;
    TextView& operator=(const TextView&) = delete;
};
    
    
} // namespace
