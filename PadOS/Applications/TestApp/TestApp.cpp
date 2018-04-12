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

#include "sam.h"

#include "TestApp.h"
#include "System/GUI/View.h"
#include "RenderTest1.h"
#include "RenderTest2.h"
#include "RenderTest3.h"
#include "RenderTest4.h"
#include "PaintView.h"
#include "ScrollTestView.h"
#include "SensorView.h"


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TestApp::TestApp() : Application("TestApp")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TestApp::~TestApp()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TestApp::ThreadStarted()
{
//    SlotTestDone();    

    AddView(ptr_new<SensorView>(),     ViewDockType::DockedWindow);
    AddView(ptr_new<RenderTest1>(),    ViewDockType::DockedWindow);
    AddView(ptr_new<RenderTest2>(),    ViewDockType::DockedWindow);
    AddView(ptr_new<RenderTest3>(),    ViewDockType::DockedWindow);
    AddView(ptr_new<ScrollTestView>(), ViewDockType::DockedWindow);
    AddView(ptr_new<PaintView>(),      ViewDockType::DockedWindow);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TestApp::SlotTestDone()
{
    m_CurrentTest = (m_CurrentTest + 1) % 6;
    if (m_CurrentView != nullptr) {
        RemoveView(m_CurrentView);
        m_CurrentView = nullptr;
    }
    if (m_CurrentTest == 0) {
        auto currentView = ptr_new<SensorView>();
        m_CurrentView = currentView;
        currentView->SignalDone.Connect(this, &TestApp::SlotTestDone);
    } else if (m_CurrentTest == 1) {
        auto currentView = ptr_new<RenderTest1>();
        m_CurrentView = currentView;
        currentView->SignalDone.Connect(this, &TestApp::SlotTestDone);
    } else if (m_CurrentTest == 2) {
        auto currentView = ptr_new<RenderTest2>();
        m_CurrentView = currentView;
        currentView->SignalDone.Connect(this, &TestApp::SlotTestDone);
    } else if (m_CurrentTest == 3) {
        auto currentView = ptr_new<RenderTest3>();
        m_CurrentView = currentView;
        currentView->SignalDone.Connect(this, &TestApp::SlotTestDone);
    } else if (m_CurrentTest == 4) {
        auto currentView = ptr_new<ScrollTestView>();
        m_CurrentView = currentView;
        currentView->SignalDone.Connect(this, &TestApp::SlotTestDone);
    } else if (m_CurrentTest == 5) {
        auto currentView = ptr_new<PaintView>();
        m_CurrentView = currentView;
        currentView->SignalDone.Connect(this, &TestApp::SlotTestDone);
    } 
    Rect viewFrame = Application::GetScreenFrame();
    m_CurrentView->SetFrame(viewFrame);
    AddView(m_CurrentView, ViewDockType::FullscreenWindow);
}
