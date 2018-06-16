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
#include "System/GUI/TextView.h"
#include "HistoryGraphView.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"


class EnvSensorView : public View
{
public:
    EnvSensorView(const String& title, const String& valueFormat, Ptr<View> parent, uint32_t flags = 0) : View("EnvSens", parent, flags), m_ValueFormat(valueFormat)
    {
        SetLayoutNode(ptr_new<VLayoutNode>());
        
        Ptr<View>     titleBar = ptr_new<View>("TitleBar", ptr_tmp_cast(this), ViewFlags::CLIENT_ONLY);
        titleBar->SetLayoutNode(ptr_new<HLayoutNode>());
        titleBar->SetBorders(5.0f, 5.0f, 5.0f, 0.0f);
              
        Ptr<TextView> titleView = ptr_new<TextView>("Title", title, titleBar);
        ptr_new<View>("spacer", titleBar, ViewFlags::CLIENT_ONLY);
        m_ValueView = ptr_new<TextView>("Value", String::zero, titleBar);
        m_ValueView->SetBorders(10.0f, 0.0f, 0.0f, 0.0f);
        
        
        m_HistoryView = ptr_new<HistoryGraphView>("CurrentHistory", ptr_tmp_cast(this));
        
        m_HistoryView->SetBorders(5.0f, 5.0f, 5.0f, 5.0f);
        m_HistoryView->SetEraseColor(100, 100, 100);
        m_HistoryView->SetFgColor(30, 180, 0);
        
        m_HistoryView->SetLayoutNode(ptr_new<LayoutNode>());
        
        m_HistoryView->SetWidthOverride(PrefSizeType::All, SizeOverride::Always, 230.0f);
        m_HistoryView->SetHeightOverride(PrefSizeType::All, SizeOverride::Always, 100.0f);
    }

    void Update(double value)
    {
        DEBUG_TRACK_FUNCTION();
//        ProfileTimer timer("CurrentSensorView::Update()");
        m_ValueView->SetText(String::format_string(m_ValueFormat.c_str(), value));
        m_HistoryView->AddValue(value);
    }

    virtual void AllAttachedToScreen() override
    {
//        m_ValueView->SetFont(ptr_new<Font>(kernel::GfxDriver::Font_e::e_Font7Seg));
    }

    virtual void Paint(const Rect& updateRect) override
    {
        DEBUG_TRACK_FUNCTION();
        DrawFrame(GetBounds(), FRAME_ETCHED);
    }
    
private:
    Ptr<TextView> m_ValueView;
    Ptr<HistoryGraphView> m_HistoryView;

    String m_ValueFormat;
};    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class EnvironmentView : public View
{
public:
    EnvironmentView(Ptr<View> parent) : View("env", parent)
    {
        SetLayoutNode(ptr_new<HLayoutNode>());
    
        m_TempView     = ptr_new<EnvSensorView>("Tmp: ", "%.2f°C", ptr_tmp_cast(this));    
        m_HumidityView = ptr_new<EnvSensorView>("Hum: ", "%.2f%%", ptr_tmp_cast(this));    
        m_PressureView = ptr_new<EnvSensorView>("Prs: ", "%.2fhPa", ptr_tmp_cast(this));

        m_HumidityView->SetBorders(10.0f, 0.0f, 10.0f, 0.0f);        
    }
    
    void Update(const BME280Values& values)
    {
        DEBUG_TRACK_FUNCTION();
        m_TempView->Update(values.m_Temperature);
        m_HumidityView->Update(values.m_Humidity);
        m_PressureView->Update(values.m_Pressure / 100.0);
    }
    
    virtual void Paint(const Rect& updateRect) override
    {
        DEBUG_TRACK_FUNCTION();
        EraseRect(GetBounds());
    }
    
private:
    Ptr<EnvSensorView> m_TempView;
    Ptr<EnvSensorView> m_HumidityView;
    Ptr<EnvSensorView> m_PressureView;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class CurrentSensorView : public View
{
public:
    CurrentSensorView(const String& title, Ptr<View> parent, uint32_t flags) : View("CurSens", parent, flags)
    {
        SetLayoutNode(ptr_new<VLayoutNode>());
        
        Ptr<View>     titleBar = ptr_new<View>("TitleBar", ptr_tmp_cast(this), ViewFlags::CLIENT_ONLY);
        titleBar->SetLayoutNode(ptr_new<HLayoutNode>());
        titleBar->SetBorders(5.0f, 5.0f, 5.0f, 0.0f);
              
        Ptr<TextView> titleView = ptr_new<TextView>("Title", title, titleBar);
        ptr_new<View>("spacer", titleBar, ViewFlags::CLIENT_ONLY);
        m_VoltageView = ptr_new<TextView>("Voltage", String::zero, titleBar);
        m_VoltageView->SetBorders(10.0f, 0.0f, 0.0f, 0.0f);
        
        
        Ptr<View>     valueBar = ptr_new<View>("ValueBar", ptr_tmp_cast(this), ViewFlags::CLIENT_ONLY);
        valueBar->SetLayoutNode(ptr_new<HLayoutNode>());
        valueBar->SetBorders(5.0f, 0.0f, 5.0f, 0.0f);
        
        m_CurrentView = ptr_new<TextView>("Current", String::zero, valueBar);
        ptr_new<View>("spacer", valueBar, ViewFlags::CLIENT_ONLY);
        m_PowerView   = ptr_new<TextView>("Power", String::zero, valueBar);
        m_CurrentHistoryView = ptr_new<HistoryGraphView>("CurrentHistory", ptr_tmp_cast(this));
        
//        m_CurrentView->SetBorders(10.0f, 0.0f, 0.0f, 0.0f);
        m_PowerView->SetBorders(10.0f, 0.0f, 0.0f, 0.0f);
        m_CurrentHistoryView->SetBorders(5.0f, 5.0f, 5.0f, 5.0f);
        m_CurrentHistoryView->SetEraseColor(100, 100, 100);
        m_CurrentHistoryView->SetFgColor(30, 180, 0);
        
        m_CurrentHistoryView->SetLayoutNode(ptr_new<LayoutNode>());
        
        m_CurrentHistoryView->SetWidthOverride(PrefSizeType::All, SizeOverride::Always, 230.0f);
        m_CurrentHistoryView->SetHeightOverride(PrefSizeType::All, SizeOverride::Always, 100.0f);
    }
    
    void Update(double voltage, double current)
    {
        DEBUG_TRACK_FUNCTION();
//        ProfileTimer timer("CurrentSensorView::Update()");
        m_VoltageView->SetText(String::format_string("%.3fV", voltage));
        m_CurrentView->SetText(String::format_string("%.2fmA", current * 1000.0));
        m_PowerView->SetText(String::format_string("%.2fW", voltage * current));
        
        m_CurrentHistoryView->AddValue(current);
    }

    virtual void AllAttachedToScreen() override
    {
//        m_VoltageView->SetFont(ptr_new<Font>(kernel::GfxDriver::Font_e::e_FontSmall));
        m_CurrentView->SetFont(ptr_new<Font>(kernel::GfxDriver::Font_e::e_FontSmall));
        m_PowerView->SetFont(ptr_new<Font>(kernel::GfxDriver::Font_e::e_FontSmall));        
    }

    virtual void Paint(const Rect& updateRect) override
    {
        DEBUG_TRACK_FUNCTION();
        DrawFrame(GetBounds(), FRAME_ETCHED);
    }
    
private:
    Ptr<TextView> m_VoltageView;
    Ptr<TextView> m_CurrentView;
    Ptr<TextView> m_PowerView;
    Ptr<HistoryGraphView> m_CurrentHistoryView;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SensorView::SensorView() : View("Sens")
{
    m_CurrentSensor = open("/dev/ina3221/0", O_RDWR);
    if (m_CurrentSensor < 0) {
        printf("ERROR: SensorView::SensorView() Failed to open current sensor\n");
    }
    m_EnvSensor = open("/dev/bme280/0", O_RDWR);
    if (m_EnvSensor < 0) {
        printf("ERROR: SensorView::SensorView() Failed to open environment sensor\n");
    }    

    SetLayoutNode(ptr_new<VLayoutNode>());

    m_EnvironmentView = ptr_new<EnvironmentView>(ptr_tmp_cast(this));
    m_EnvironmentView->SetHAlignment(Alignment::Left);
    m_EnvironmentView->SetBorders(10.0f, 10.0f, 10.0f, 0.0f);
    m_EnvironmentView->SetWheight(0.0f);
    
    Ptr<View> currentSensorContainer = ptr_new<View>("CurrentSensors", ptr_tmp_cast(this), ViewFlags::CLIENT_ONLY);
    currentSensorContainer->SetLayoutNode(ptr_new<HLayoutNode>());
    currentSensorContainer->SetWheight(0.0f);
    
    m_CurrentView33  = ptr_new<CurrentSensorView>("3.3V", currentSensorContainer, 0);
    m_CurrentView50  = ptr_new<CurrentSensorView>("5.0V", currentSensorContainer, 0);
    m_CurrentViewBat = ptr_new<CurrentSensorView>("Bat", currentSensorContainer, 0);

    m_CurrentView33->SetBorders(10.0f, 10.0f, 0.0f, 10.0f);
    m_CurrentView50->SetBorders(10.0f, 10.0f, 0.0f, 10.0f);
    m_CurrentViewBat->SetBorders(10.0f, 10.0f, 10.0f, 10.0f);
    
    Ptr<View> spacer = ptr_new<View>("spacer", ptr_tmp_cast(this), ViewFlags::CLIENT_ONLY);

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
    
    m_UpdateTimer.Set(bigtime_from_ms(100));
    m_UpdateTimer.SignalTrigged.Connect(this, &SensorView::SlotFrameProcess);
    
    Application* app = GetApplication();
    app->AddTimer(&m_UpdateTimer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::Paint(const Rect& updateRect)
{
    DEBUG_TRACK_FUNCTION();
    PaintContent();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::PaintContent()
{
    DEBUG_TRACK_FUNCTION();
    float y = 10.0f;
    FontHeight fontHeight = GetFontHeight();
    float lineSpacing = fontHeight.descender - fontHeight.ascender + fontHeight.line_gap;
    
    float x1 = 10.0f;
    float x2 = 195.0f;

    y += lineSpacing * 4.0f;
    
    Rect clearFrame = GetBounds();
    clearFrame.bottom = y + lineSpacing;
    EraseRect(clearFrame);
    
    y += lineSpacing * 8.0f;
    clearFrame.top = clearFrame.bottom;
    clearFrame.bottom = y + lineSpacing;
    EraseRect(clearFrame);

    double efficiency = (m_VoltageValues.Voltages[0] * m_VoltageValues.Currents[0] + m_VoltageValues.Voltages[1] * m_VoltageValues.Currents[1]) * 100.0 / (m_VoltageValues.Voltages[2] * m_VoltageValues.Currents[2]);
    static double avgEfficiency = efficiency;
    avgEfficiency += (efficiency - avgEfficiency) * 0.01;
    DrawValue(x1, x2, y, "Efficiency:", "%.3f", "%", avgEfficiency);
    
    y += lineSpacing;
    clearFrame.top = clearFrame.bottom;
    clearFrame.bottom = GetBounds().bottom;
    EraseRect(clearFrame);    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SensorView::OnMouseUp(MouseButton_e button, const Point& position)
{
    DEBUG_TRACK_FUNCTION();
    SignalDone(ptr_tmp_cast(this));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::SlotFrameProcess()
{
    DEBUG_TRACK_FUNCTION();
    static uint32_t frameCount = 0;
    frameCount++;
//    bigtime_t startTime = get_system_time_hires();

    BME280Values envValues;
    BME280IOCTL_GetValues(m_EnvSensor, &envValues);
    INA3221_GetMeasurements(m_CurrentSensor, &m_VoltageValues);

    m_EnvValues.m_Temperature += (envValues.m_Temperature - m_EnvValues.m_Temperature) * 0.1;
    m_EnvValues.m_Humidity    += (envValues.m_Humidity    - m_EnvValues.m_Humidity) * 0.1;
    m_EnvValues.m_Pressure    += (envValues.m_Pressure    - m_EnvValues.m_Pressure) * 0.1;
    
    if ((frameCount % 10) == 0) {
        m_EnvironmentView->Update(m_EnvValues);
    }
    
    if ((frameCount % 2) == 0) {
        m_CurrentView33->Update(m_VoltageValues.Voltages[0], m_VoltageValues.Currents[0]);
        m_CurrentView50->Update(m_VoltageValues.Voltages[1], m_VoltageValues.Currents[1]);
        m_CurrentViewBat->Update(m_VoltageValues.Voltages[2], m_VoltageValues.Currents[2]);
    }
    Invalidate();

//    static bigtime_t prevTime = get_system_time_hires();
//    bigtime_t endTime = get_system_time_hires();
//    printf("Time: %.3f (%.3f)\n", double(endTime - startTime) / 1000.0, double(endTime - prevTime) / 1000.0);
//    prevTime = endTime;
}
    
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SensorView::DrawValue(float x1, float x2, float y, const String& label, const char* valueFormat, const String& unit, double value)
{
    DEBUG_TRACK_FUNCTION();
    MovePenTo(x1, y);
    SetFgColor(0, 0, 0);
    DrawString(label);
    MovePenTo(x2, y);
    SetFgColor(0, 0, 255);
    DrawString(String::format_string(valueFormat, value));
    SetFgColor(0, 0, 220);
    DrawString(unit + "   ");    
}
