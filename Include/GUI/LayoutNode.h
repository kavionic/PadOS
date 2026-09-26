// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
