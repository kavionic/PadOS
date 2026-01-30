// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
