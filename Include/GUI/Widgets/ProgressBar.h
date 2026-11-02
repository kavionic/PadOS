// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ProgressBarStyleData.h>
#include <GUI/StyledView.h>


/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PProgressBar : public PStyledView
{
public:
    PProgressBar(const PString& name = PString::zero, POrientation orientation = POrientation::Horizontal, PViewFlags flags = PViewFlags::zero);
    PProgressBar(PViewFactoryContext& context, const pugi::xml_node& xmlData);

    // From View:
    virtual void OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void OnStyleChanged() override;
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    // From ProgressBar:
    void    SetProgress(float value);
    float   GetProgress() const;

private:
    void NotifyStylePropertyChanged(PProgressBarProperties properties);

    PProgressBarStyleData m_ProgressBarStyleData;
};
