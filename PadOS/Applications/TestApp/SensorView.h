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
// Created: 03.04.2018 23:17:36

#pragma once

#include "System/GUI/View.h"
#include "System/Utils/EventTimer.h"
#include "DeviceControl/BME280.h"
#include "DeviceControl/INA3221.h"

using namespace os;

class EnvironmentView;
class CurrentSensorView;

class SensorView : public View
{
public:
    SensorView();
    ~SensorView();

    virtual void AllAttachedToScreen() override;

    virtual void Paint(const Rect& updateRect) override;

    virtual bool OnMouseDown(MouseButton_e button, const Point& position) override { return true; }
    virtual bool OnMouseUp(MouseButton_e button, const Point& position) override;

    Signal<void, Ptr<View>> SignalDone;

private:
    void SlotFrameProcess();
    
    void PaintContent();
    void DrawValue(float x1, float x2, float y, const String& label, const char* valueFormat, const String& unit, double value);
    
    EventTimer m_UpdateTimer;

    int m_CurrentSensor = -1;
    int m_EnvSensor = -1;

    BME280Values  m_EnvValues;
    INA3221Values m_VoltageValues;

    Ptr<EnvironmentView> m_EnvironmentView;
    Ptr<CurrentSensorView> m_CurrentView33;
    Ptr<CurrentSensorView> m_CurrentView50;
    Ptr<CurrentSensorView> m_CurrentViewBat;

    SensorView(const SensorView&) = delete;
    SensorView& operator=(const SensorView&) = delete;

};
