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
// Created: 18.03.2018 19:41:52

#pragma once

#include "System/GUI/Application.h"

using namespace os;

class TestApp : public Application
{
public:
    TestApp();
    ~TestApp();
    
    virtual void ThreadStarted() override;
    
private:
    void SlotTestDone();
    
    int m_CurrentTest = -1;
    Ptr<View> m_CurrentView;    
    TestApp(const TestApp&) = delete;
    TestApp& operator=(const TestApp&) = delete;
};

