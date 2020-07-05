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

#pragma once


#include <vector>

#include "Ptr/PtrTarget.h"
#include "Math/Rect.h"

namespace os
{
class View;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class LayoutNode : public PtrTarget
{
public:
    LayoutNode();
    virtual ~LayoutNode();

    virtual void Layout();
    virtual void CalculatePreferredSize(Point* minSizeOut, Point* maxSizeOut, bool includeWidth, bool includeHeight);
    virtual void ApplyInnerBorders(const Rect& borders, float spacing);
protected:
    View* m_View = nullptr;

private:
    friend class View;

    void AttachedToView(View* view);
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class HLayoutNode : public LayoutNode
{
public:
    HLayoutNode();
    virtual void Layout() override;
    virtual void CalculatePreferredSize(Point* minSizeOut, Point* maxSizeOut, bool includeWidth, bool includeHeight) override;
	virtual void ApplyInnerBorders(const Rect& borders, float spacing) override;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class VLayoutNode : public LayoutNode
{
public:
    VLayoutNode();
    virtual void Layout() override;
    virtual void CalculatePreferredSize(Point* minSizeOut, Point* maxSizeOut, bool includeWidth, bool includeHeight) override;
	virtual void ApplyInnerBorders(const Rect& borders, float spacing) override;
};




} // end of namespace
