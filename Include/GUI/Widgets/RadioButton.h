// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Widgets/ButtonBase.h>


///////////////////////////////////////////////////////////////////////////////
/// 2-state check box.
/// \ingroup gui
/// \par Description:
///
/// \sa
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PRadioButton : public PButtonBase
{
public:
    PRadioButton(const PString& name = PString::zero, PViewFlags flags = PViewFlag::WillDraw | PViewFlag::ClearBackground);
    PRadioButton(PViewFactoryContext& context, const pugi::xml_node& xmlData);
    ~PRadioButton();

      // From View:
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void OnPaint(const PRect& updateRect) override;

    // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) override;

private:
};
