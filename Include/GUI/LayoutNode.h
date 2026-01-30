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

class PView;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PLayoutNode : public PtrTarget
{
public:
    PLayoutNode();
    virtual ~PLayoutNode();

    virtual void Layout();
    virtual void CalculatePreferredSize(PPoint* minSizeOut, PPoint* maxSizeOut, bool includeWidth, bool includeHeight);
    virtual void ApplyInnerBorders(const PRect& borders, float spacing);
protected:
    PView* m_View = nullptr;

private:
    friend class PView;

    void AttachedToView(PView* view);
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PHLayoutNode : public PLayoutNode
{
public:
    PHLayoutNode();
    virtual void Layout() override;
    virtual void CalculatePreferredSize(PPoint* minSizeOut, PPoint* maxSizeOut, bool includeWidth, bool includeHeight) override;
	virtual void ApplyInnerBorders(const PRect& borders, float spacing) override;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PVLayoutNode : public PLayoutNode
{
public:
    PVLayoutNode();
    virtual void Layout() override;
    virtual void CalculatePreferredSize(PPoint* minSizeOut, PPoint* maxSizeOut, bool includeWidth, bool includeHeight) override;
	virtual void ApplyInnerBorders(const PRect& borders, float spacing) override;
};
