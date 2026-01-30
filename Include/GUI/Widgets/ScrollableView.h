// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.05.2025 21:30

#pragma once

#include <GUI/View.h>


class PScrollableView : public PView
{
public:
    PScrollableView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PScrollableView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual PPoint CalculateContentSize() const override;
    virtual void OnLayoutChanged() override;

    void SetContentView(Ptr<PView> contentView);

private:
    Ptr<PView> m_ContentView;
};
