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

#include "System/Math/Rect.h"
#include "System/Math/Point.h"
#include "System/Ptr/PtrTarget.h"


namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class Region : public PtrTarget
{
public:
    Region();
    Region(const IRect& rect);
    Region(const Region& region);
    Region(const Region& region, const IRect& rect, bool normalize);

    ~Region();

    void        Set(const IRect& rect);
    void        Set(const Region& region);
    void        Clear();
    bool        IsEmpty() const      { return m_Rects.empty(); }
    int         GetClipCount() const { return m_Rects.size(); }
        
    void        Include(const IRect& rect);
    void        Intersect(const Region& region);
    void        Intersect(const Region& region, const IPoint& offset);

    void        Exclude(const IRect& rect);
    void        Exclude(const Region& region);
    void        Exclude(const Region& region, const IPoint& offset);

    void        AddRect(const IRect& rect);
    
    void        Optimize();
    IRect       GetBounds() const;
        
    static bool ClipLine(const IRect& rect, IPoint* point1, IPoint* point2);
    
    void Validate();

    std::vector<IRect> m_Rects;

private:
    int FindUnusedClip(int prevUnused, int lastToCheck);
    int AddOrReuseClip(int prevUnused, int lastToCheck, const IRect& frame);
    void RemoveUnusedClips(size_t firstToCheck, size_t lastToCheck);
  
};

} // namespace
