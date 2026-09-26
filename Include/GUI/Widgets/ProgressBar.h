// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/View.h>


/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PProgressBar : public PView
{
public:
    PProgressBar(const PString& name = PString::zero, Ptr<PView> parent = nullptr, POrientation orientation = POrientation::Horizontal, uint32_t flags = 0);
    PProgressBar(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    // From View:
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    // From ProgressBar:
    void    SetProgress(float value);
    float   GetProgress() const;

private:
    float 	    m_Progress = 0.0f;
    POrientation m_Orientation;
};
