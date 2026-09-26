// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 02.04.2018 13:08:47

#pragma once

#include <GUI/Widgets/ButtonBase.h>


class PButton : public PButtonBase
{
public:
    PButton(const PString& name, const PString& label, Ptr<PView> parent = nullptr, uint32_t flags = 0);
	PButton(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);
    ~PButton();

    // From View:
    virtual void AllAttachedToScreen() override { Invalidate(); }
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void OnPaint(const PRect& updateRect) override;

	// From Control:
    virtual void OnEnableStatusChanged(bool bIsEnabled) override { Invalidate(); Flush(); }
	virtual void OnLabelChanged(const PString& label) override;

private:
    void UpdateLabelSize();
    PPoint  m_LabelSize;
        
    PButton(const PButton&) = delete;
    PButton& operator=(const PButton&) = delete;
};
