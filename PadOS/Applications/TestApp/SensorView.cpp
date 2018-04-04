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
// Created: 03.04.2018 23:17:35

#include "sam.h"

#include <stdio.h>
#include <fcntl.h>

#include "SensorView.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SensorView::SensorView() : View("SensorView")
{
    m_CurrentSensor = open("/dev/ina3221/0", O_RDWR);
    if (m_CurrentSensor < 0) {
        printf("ERROR: SensorView::SensorView() Failed to open current sensor\n");
    }
    m_EnvSensor = open("/dev/bme280/0", O_RDWR);
    if (m_EnvSensor < 0) {
        printf("ERROR: SensorView::SensorView() Failed to open environment sensor\n");
    }    

    BME280IOCTL_GetValues(m_EnvSensor, &m_EnvValues);
    INA3221_GetMeasurements(m_CurrentSensor, &m_VoltageValues);    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SensorView::~SensorView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::AllAttachedToScreen()
{
    SetFgColor(255, 255, 255);
    FillRect(GetBounds());
    
//    SetFont(ptr_new<Font>(kernel::GfxDriver::e_Font7Seg));
    
    m_UpdateTimer.Set(bigtime_from_ms(200));
    m_UpdateTimer.SignalTrigged.Connect(this, &SensorView::SlotFrameProcess);
    
    Application* app = GetApplication();
    app->AddTimer(&m_UpdateTimer);
    
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::Paint(const Rect& updateRect)
{
    float y = 10.0f;
    FontHeight fontHeight = GetFontHeight();
    float lineSpacing = fontHeight.descender - fontHeight.ascender + fontHeight.line_gap;
    
    float x1 = 20.0f;
    float x2 = 195.0f;

    float x11 = 20.0f;
    float x12 = x11 + 105.0f;

    float x21 = x12 + 150.0f;
    float x22 = x21 + 105.0f;

    float x31 = x22 + 150.0f;
    float x32 = x31 + 105.0f;
   
    DrawValue(x1, x2, y, "Temperature:", "%.3f", "°C", m_EnvValues.m_Temperature);
    y += lineSpacing;
    DrawValue(x1, x2, y, "Humidity:", "%.3f", "%", m_EnvValues.m_Humidity);
    y += lineSpacing;
    DrawValue(x1, x2, y, "Pressure:", "%.3f", "hPa", m_EnvValues.m_Pressure / 100.0f);

    
    y += lineSpacing * 2.0f;
    DrawValue(x11, x12, y, "3.3 (V):", "%.3f", "V", m_VoltageValues.Voltages[0]);
    DrawValue(x21, x22, y, "5.0 (V):", "%.3f", "V", m_VoltageValues.Voltages[1]);
    DrawValue(x31, x32, y, "Bat (V):", "%.3f", "V", m_VoltageValues.Voltages[2]);    
    y += lineSpacing;
    DrawValue(x11, x12, y, "3.3 (I):", "%.2f", "mA", m_VoltageValues.Currents[0] * 1000.0);
    DrawValue(x21, x22, y, "5.0 (I):", "%.2f", "mA", m_VoltageValues.Currents[1] * 1000.0);
    DrawValue(x31, x32, y, "Bat (I):", "%.2f", "mA", m_VoltageValues.Currents[2] * 1000.0);
    y += lineSpacing;
    DrawValue(x11, x12, y, "3.3 (P):", "%.2f", "W", m_VoltageValues.Voltages[0] * m_VoltageValues.Currents[0]);
    DrawValue(x21, x22, y, "5.0 (P):", "%.2f", "mW", m_VoltageValues.Voltages[1] * m_VoltageValues.Currents[1] * 1000.0);
    DrawValue(x31, x32, y, "Bat (P):", "%.2f", "W", m_VoltageValues.Voltages[2] * m_VoltageValues.Currents[2]);

    y += lineSpacing * 2.0f;

    double efficiency = (m_VoltageValues.Voltages[0] * m_VoltageValues.Currents[0] + m_VoltageValues.Voltages[1] * m_VoltageValues.Currents[1]) * 100.0 / (m_VoltageValues.Voltages[2] * m_VoltageValues.Currents[2]);
    static double avgEfficiency = efficiency;
    avgEfficiency += (efficiency - avgEfficiency) * 0.01;
    DrawValue(x1, x2, y, "Efficiency:", "%.3f", "%", avgEfficiency);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SensorView::OnMouseUp(MouseButton_e button, const Point& position)
{
    SignalDone(ptr_tmp_cast(this));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::SlotFrameProcess()
{
    BME280IOCTL_GetValues(m_EnvSensor, &m_EnvValues);
    INA3221_GetMeasurements(m_CurrentSensor, &m_VoltageValues);
    Invalidate();
    Flush();
}
    
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::DrawValue(float x1, float x2, float y, const String& label, const char* valueFormat, const String& unit, double value)
{
    MovePenTo(x1, y);
    SetFgColor(0, 0, 0);
    DrawString(label);
    MovePenTo(x2, y);
    SetFgColor(0, 0, 255);
    DrawString(String::FormatString(valueFormat, value));
    SetFgColor(0, 0, 220);
    DrawString(unit + "   ");    
}
