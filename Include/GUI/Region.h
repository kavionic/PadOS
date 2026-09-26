// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include "Math/Rect.h"
#include "Math/Point.h"
#include "Ptr/PtrTarget.h"



///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PRegion : public PtrTarget
{
public:
    PRegion();
    PRegion(const PIRect& rect);
    PRegion(const PRegion& region);
    PRegion(const PRegion& region, const PIRect& rect, bool normalize);

    ~PRegion();

    void        Set(const PIRect& rect);
    void        Set(const PRegion& region);
    void        Clear();
    bool        IsEmpty() const      { return m_Rects.empty(); }
    int         GetClipCount() const { return m_Rects.size(); }
        
    void        Include(const PIRect& rect);
    void        Intersect(const PRegion& region);
    void        Intersect(const PRegion& region, const PIPoint& offset);

    void        Exclude(const PIRect& rect);
    void        Exclude(const PRegion& region);
    void        Exclude(const PRegion& region, const PIPoint& offset);

    void        AddRect(const PIRect& rect);
    
    void        Optimize();
    PIRect       GetBounds() const;
        
    static bool ClipLine(const PIRect& rect, PIPoint* point1, PIPoint* point2);
    
    void Validate();

    std::vector<PIRect> m_Rects;

private:
    size_t FindUnusedClip(size_t prevUnused, size_t lastToCheck);
    size_t AddOrReuseClip(size_t prevUnused, size_t lastToCheck, const PIRect& frame);
    void RemoveUnusedClips(size_t firstToCheck, size_t lastToCheck);
  
};
