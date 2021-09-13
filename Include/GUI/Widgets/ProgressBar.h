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

#include <GUI/View.h>

namespace os
{

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ProgressBar : public View
{
public:
    ProgressBar(const std::string& name, Ptr<View> parent = nullptr, Orientation orientation = Orientation::Horizontal, uint32_t flags = 0);
    ProgressBar(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);

    // From View:
    virtual void Paint(const Rect& updateRect) override;
    virtual void FrameSized(const Point& delta) override;
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;

    // From ProgressBar:
    void    SetProgress(float value);
    float   GetProgress() const;

private:
    float 	    m_Progress = 0.0f;
    Orientation m_Orientation;
};



} // namespace os
