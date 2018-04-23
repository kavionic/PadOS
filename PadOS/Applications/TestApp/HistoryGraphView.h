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
// Created: 15.04.2018 11:46:11

#pragma once

#include <deque>

#include "System/GUI/View.h"
#include "System/GUI/GUIEvent.h"

using namespace os;


class HistoryGraphView : public View
{
public:
    HistoryGraphView(const String& name, Ptr<View> parent, uint32_t flags = 0);
    ~HistoryGraphView();

    void AddValue(float value);

    virtual void Paint(const Rect& updateRect) override;


private:
    std::deque<float> m_Values;
    float m_MinValue = std::numeric_limits<float>::max();
    float m_MaxValue = std::numeric_limits<float>::lowest();
    
    HistoryGraphView(const HistoryGraphView&) = delete;
    HistoryGraphView& operator=(const HistoryGraphView&) = delete;
};
