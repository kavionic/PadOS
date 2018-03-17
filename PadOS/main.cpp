// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 21.10.2017 00:18:50

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>

#undef _U
#undef _L
#undef LITTLE_ENDIAN


#include "System/Math/Point.h"
#include "System/Math/Rect.h"

#include <vector>
#include <deque>
#include <algorithm>

#include "sam.h"
#include "component/pio.h"
#include "SystemSetup.h"

#include "System/GUI/GUI.h"
#include "System/Signals//Signal.h"
#include "PaintView.h"
#include "System/Math/LineSegment.h"
#include "System/GUI/Region.h"
#include "RenderTest1.h"
#include "RenderTest2.h"
#include "RenderTest3.h"
#include "RenderTest4.h"
#include "ScrollTestView.h"
#include "Kernel/HAL/SAME70System.h"
#include "Kernel/Drivers/HSMCIDriver.h"
#include "Kernel/Drivers/RA8875Driver/TouchDriver.h"
#include "FAT.h"
#include "SDRAM.h"
#include "System/Utils/EventTimer.h"
#include "System/Threads.h"
#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"
#include "Kernel/Drivers/INA3221Driver.h"
#include "Kernel/Drivers/BME280Driver.h"
#include "Kernel/Drivers/UARTDriver.h"
#include "Kernel/Drivers/I2CDriver.h"
#include "DeviceControl/INA3221.h"
#include "DeviceControl/BME280.h"
#include "Tests.h"

extern "C" void InitializeNewLibMutexes();

typedef enum {
	SDRAMC_OK = 0,
	SDRAMC_TIMEOUT = 1,
	SDRAMC_ERROR = 2,
} sdramc_status_t;

/** SDRAM benchmark test size */
#define SDRAMC_TEST_BUFF_SIZE        (30 * 1024)

/** SDRAM benchmark test page number */
#define SDRAMC_TEST_PAGE_NUM        (1024)

/** SDRAMC access test length */
#define SDRAMC_TEST_LENGTH        (1024 * 1024 * 64)

/** SDRAMC access test data in even address */
#define SDRAMC_TEST_EVEN_TAG        (0x55aaaa55)

/** SDRAMC access test data in odd address */
#define SDRAMC_TEST_ODD_TAG        (0xaa5555aa)

bigtime_t g_FrameTime = 0;

kernel::TouchDriver kernel::TouchDriver::Instance(LCD_TP_WAKE_Pin, LCD_TP_RESET_Pin, LCD_TP_INT_Pin);

kernel::HSMCIDriver g_SDCardBlockDevice(DigitalPin(e_DigitalPortID_A, 24));
kernel::FAT         g_FileSystem;


/*
E1: 7	PA00: LCD_DATA_B00
E2: 9	PA02: LCD_DATA_B01

E1: 11	PA03: LCD_DATA_B02
E2: 11	PA03: LCD_DATA_B02

E1: 12	PA04: LCD_DATA_B03
E2: 12	PA04: LCD_DATA_B03

E2: 5	PA06: LCD_DATA_B04
E1: 4	PA19: LCD_DATA_B05
E2: 13	PA21: LCD_DATA_B06
E2: 10	PA24: LCD_DATA_B07

E1: 13	PB00: LCD_DATA_B08
E1: 14	PB01: LCD_DATA_B09

E1: 6	PB02: 0 LEDA
E1: 5	PB03: 
E2: 14	PB04

E2: 4	PC13: 2 CS
E1: 10	PC17: 3 RD
E2: 7	PC19: 1 RESET
E1: 8	PC30: 4 WR
E1: 3	PC31: 5 RS

E2: 6	PD11: LCD_DATA_B10

E1: 17	PD20: MISO
E2: 17	PD20: MISO

E1: 16	PD21: MOSI
E2: 16	PD21: MOSI

E1: 18	PD22: SPCK
E2: 18	PD22: SPCK

E1: 15	PD25: LCD_DATA_B11
E2: 8	PD26: LCD_DATA_B12
E2: 15	PD27: LCD_DATA_B13
E1: 9	PD28: LCD_DATA_B14
E2: 3	PD30: LCD_DATA_B15


E1: 6	PB02: 0 LEDA
E1: 3	PC31: 1 RESET
E1: 8	PC30: 2 CS
E1: 10	PC17: 3 RD
E2: 4	PC13: 4 WR
E2: 7	PC19: 5 RS

E1: 6	PB02: 0 LEDA
E1: 3	PC31: 5 RS
E1: 8	PC30: 4 WR
E1: 10	PC17: 3 RD
E2: 4	PC13: 2 CS
E2: 7	PC19: 1 RESET

*/




void NonMaskableInt_Handler() {kernel::panic("NMI\n");}
void HardFault_Handler() {kernel::panic("HardFault\n");}
void MemoryManagement_Handler() {kernel::panic("MemManage\n");}
void BusFault_Handler() {kernel::panic("BusFault\n");}
void UsageFault_Handler() {kernel::panic("UsageFault\n");}
void DebugMonitor_Handler() {}

/*
bool IsScreenTouched()
{
#if 1
    gui.FrameProcess();

    GUIEvent event;
    while( gui.GetEvent(&event) )
    {
        switch( event.m_EventID )
        {
            case e_EventMouseDown:
            return true;
        }
    }
    return false;    
#else
    static bigtime_t prevTime;
    bigtime_t curTime = get_system_time();
    if (curTime - prevTime > bigtime_from_s(10))
    {
        prevTime = curTime;
        return true;
    }
    return false;
#endif
}


void DisplayTest1()
{
    Display::Instance.SetFgColor(0xffff);
    Display::Instance.FillRect(0, 0, 799, 479);

    int posX = 50 + rand() % 700;
    int posY = 50 + rand() % 380;
    float dirX = 1;
    float dirY = 1;

    static const int radius = 40;
    static const int speed = 5;

    for (;;)
    {
        SAME70System::ResetWatchdog();
        for (uint16_t j = 0 ; j < 10 ; ++j )
        {
            if (IsScreenTouched()) return;
            for (uint16_t j = 0 ; j < 5 ; ++j )
            {
                posX += dirX;
                posY += dirY;
                dirY += 0.05f;
                if (posX < radius) {
                    posX = radius;
                    dirX = 2 + rand() % speed;
                    } else if (posX > 799 - radius) {
                    posX = 799 - radius;
                    dirX = -(2 + rand() % speed);
                }
                if (posY < radius) {
                    posY = radius;
                    dirY = 2 + rand() % speed;
                    } else if (posY > 479 - radius) {
                    posY = 479 - radius;
                    dirY = -(2 + rand() % speed);
                }
                Display::Instance.SetFgColor(rand());
                Display::Instance.FillCircle(posX, posY, radius);
                //            Display::Instance.FillRect(rand() % 480, rand() % 320, rand() % 480, rand() % 320, rand());
                //            gui.FrameProcess();
                //            if ( TouchDriver::Instance.IsPressed() ) break;
            }
        }        
        Display::Instance.SetFgColor(rand());
        Display::Instance.FillCircle(50 + rand() % 700, 50 + rand() % 380, 5+rand() % 45);
    }
}

static const int historySize = 100;
//Point history[historySize][2];

void DisplayTest2()
{
    GfxDriver::Instance.SetFgColor(0xffff);
    Display::Instance.FillRect(0, 0, 799, 479);

    Point pos1(rand() % 799, rand() % 479);
    Point pos2(rand() % 799, rand() % 479);
    
    Point dir1(1,-1);
    Point dir2(1, 1);

    static const int speed = 3;

    std::vector<Point> history;
    history.resize(historySize*2);
  
    
    int historyPos = 0;
    for (;;)
    {
        SAME70System::ResetWatchdog();
        if (IsScreenTouched()) break;
        for (uint16_t j = 0 ; j < 50 ; ++j )
        {
            pos1 += dir1;
            pos2 += dir2;
            if (pos1.x < 0) {
                pos1.x = 0;
                dir1.x = 2 + rand() % speed;
            } else if (pos1.x > 799) {
                pos1.x = 799;
                dir1.x = -(2 + rand() % speed);
            }
            if (pos1.y < 0) {
                pos1.y = 0;
                dir1.y = 2 + rand() % speed;
            } else if (pos1.y > 479) {
                pos1.y = 479;
                dir1.y = -(2 + rand() % speed);
            }

            if (pos2.x < 0) {
                pos2.x = 0;
                dir2.x = 2 + rand() % speed;
            } else if (pos2.x > 799) {
                pos2.x = 799;
                dir2.x = -(2 + rand() % speed);
            }
            if (pos2.y < 0) {
                pos2.y = 0;
                dir2.y = 2 + rand() % speed;
            } else if (pos2.y > 479) {
                pos2.y = 479;
                dir2.y = -(2 + rand() % speed);
            }
            Display::Instance.SetFgColor(0xffff);
            Display::Instance.DrawLine(history[historyPos*2].x, history[historyPos*2].y, history[historyPos*2+1].x, history[historyPos*2+1].y);
            history[historyPos*2] = pos1;
            history[historyPos*2+1] = pos2;
            historyPos = (historyPos + 1) % historySize;
            Display::Instance.SetFgColor(rand());
            Display::Instance.DrawLine(pos1.x, pos1.y, pos2.x, pos2.y);
        }
    }
}

void DisplayTest3()
{
    Display::Instance.SetFgColor(0xffff);
    Display::Instance.FillRect(0, 0, 799, 479);
    for (;;)
    {
        SAME70System::ResetWatchdog();
        for (int i = 0; i < 100; ++i)
        {
            if (IsScreenTouched()) return;
            for (int j = 0; j < 10; ++j)
            {
                Display::Instance.SetFgColor(rand());
                Display::Instance.FillCircle(50 + rand() % 700, 50 + rand() % 380, 5+rand() % 45);
            }
        }
        Display::Instance.SetFgColor(rand());
        Display::Instance.FillRect(0, 0, 799, 479);
    }        
}

void DisplayTest4()
{
    Display::Instance.WaitBlitter();
    Display::Instance.SetWindow(0, 0, 799, 479);
    Display::Instance.SetFgColor(0xffff);
    Display::Instance.FillRect(0, 0, 799, 479);
    
    
    ClippingRegion region;
    
    IRect frameRect(50.0f, 0.0f, 400.0f, 480.0f);
    
    region.AddRect(frameRect);
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(60, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(100, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(140, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(180, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(220, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(100, 60) );


    Region region2(frameRect);

    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(60, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(100, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(140, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(180, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(220, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(100, 60) );
    region2.Optimize();
    
    std::vector<ILineSegment> clippedLines;
    
    for (int y = 10; y < 310; ++y)
    {
        std::deque<ILineSegment> clippedLine;
    
        region.Clip(ILineSegment(IPoint(20, y),IPoint(500, y)), clippedLine);

        for (auto l : clippedLine)
        {
            clippedLines.push_back(ILineSegment(l.p1, l.p2));
        }
    }
    
    for (;;)
    {
        SAME70System::ResetWatchdog();
        if (IsScreenTouched()) break;
        uint32_t startTime = Clock::GetTime();
        for (int i = 0; i < 10; ++i)
        {
            uint16_t color = random();
            GfxDriver::Instance.SetFgColor(color);
            for (int y = 10; y < 310; ++y)
            {
#if 1
            
                ENUMCLIPLIST(&region2.m_cRects, node)
                {
                    int x1 = 20;
                    int y1 = y;
                    int x2 = 400;
                    int y2 = y;
                    
//                    GfxDriver::Instance.WaitBlitter();
//                    GfxDriver::Instance.SetWindow(node->m_cBounds.left, node->m_cBounds.top, node->m_cBounds.right, node->m_cBounds.bottom);
//                    GfxDriver::Instance.DrawLine(x1, y1, x2, y2);

                    if (Region::ClipLine(node->m_cBounds, &x1, &y1, &x2, &y2)) {
                        //GfxDriver::Instance.DrawLine(20, y, 400, y);
                        GfxDriver::Instance.DrawLine(x1, y1, x2, y2);
                    }
                    
                }
#else
                std::deque<ILineSegment> clippedLine;
    
                region2.Clip(ILineSegment(IPoint(20.0f, y),IPoint(400.0f, y)), clippedLine);

                for (auto l : clippedLine)
                {
            //        auto l = clippable_line;
                    float x1 = l.p1.x;
                    float y1 = l.p1.y;
                    float x2 = l.p2.x;
                    float y2 = l.p2.y;
        
                    GfxDriver::Instance.DrawHLine(color, x1, y1, x2 - x1);
                }
#endif
            }
            Display::Instance.SetWindow(0, 0, 799, 479);
            Display::Instance.SetFgColor(rand());
            Display::Instance.FillCircle(460 + rand() % 340, 50 + rand() % 380, 5+rand() % 45);
        }        
        uint32_t curTime = Clock::GetTime();
        g_FrameTime = curTime - startTime;
        startTime = curTime;
    }        
    
}

void DrawBeveledBox(IPoint tl, IPoint br, bool raised)
{
    uint16_t colorDark  = GfxDriver::MakeColor(0xa0,0xa0,0xa0);
    uint16_t colorLight = GfxDriver::MakeColor(0xe0,0xe0,0xe0);
    if ( !raised ) std::swap(colorDark, colorLight);

    Display::Instance.SetFgColor(colorLight);
    GfxDriver::Instance.DrawLine(tl.x, tl.y, br.x, tl.y);
    Display::Instance.SetFgColor(colorDark);
    GfxDriver::Instance.DrawLine(br.x, tl.y + 1, br.x, br.y);
    GfxDriver::Instance.DrawLine(br.x - 1, br.y, tl.x - 1, br.y);
    Display::Instance.SetFgColor(colorLight);
    GfxDriver::Instance.DrawLine(tl.x, br.y, tl.x, tl.y);

    colorDark  = GfxDriver::MakeColor(0xc0,0xc0,0xc0);
    colorLight = GfxDriver::MakeColor(0xff,0xff,0xff);
    if ( !raised ) std::swap(colorDark, colorLight);
    
    tl.x++;
    tl.y++;
    br.x--;
    br.y--;
    Display::Instance.SetFgColor(colorLight);
    GfxDriver::Instance.DrawLine(tl.x, tl.y, br.x, tl.y);
    Display::Instance.SetFgColor(colorDark);
    GfxDriver::Instance.DrawLine(br.x, tl.y + 1, br.x, br.y);
    GfxDriver::Instance.DrawLine(br.x - 1, br.y, tl.x - 1, br.y);
    Display::Instance.SetFgColor(colorLight);
    GfxDriver::Instance.DrawLine(tl.x, br.y, tl.x, tl.y);

    tl.x++;
    tl.y++;
    br.x--;
    br.y--;
    
    Display::Instance.SetFgColor((raised) ? GfxDriver::MakeColor(0xd0,0xd0,0xd0) : GfxDriver::MakeColor(0xf0,0xf0,0xf0));
    GfxDriver::Instance.FillRect(tl.x, tl.y, br.x, br.y);
}
#if 0
void Painter()
{
    IPoint screenRes = Display::Instance.GetResolution();
    //    int16_t prevX = -1;
    //    int16_t prevY = -1;
    Display::Instance.SetFgColor(0xffff);
    Display::Instance.FillRect(0, 0, screenRes.x - 1, screenRes.y - 1);
    
    DrawBeveledBox(IPoint(0,440), IPoint(40,479), true);
    DrawBeveledBox(IPoint(760,440), IPoint(799,479), true);
    
    uint32_t prevUpdateTime = Clock::GetTime();
    
    int8_t pressedButton = -1;
    for(;;)
    {
        int16_t x;
        int16_t y;
        
        SAME70System::ResetWatchdog();


        uint32_t time = Clock::GetTime();
        
        if ((time - prevUpdateTime) > 1)
        {
            gui.FrameProcess();
            prevUpdateTime = time;
        }

        GUIEvent event;
        
        if ( gui.GetEvent(&event) )
        {
            //printf("Received event %d (%d, %d)\n", event.m_EventID, event.Data.Mouse.x, event.Data.Mouse.y);
            if ( event.m_EventID == e_EventMouseDown )
            {
                int16_t mouseX = event.Data.Mouse.x;
                int16_t mouseY = event.Data.Mouse.y;
                
                if ( mouseY > 280 )
                {
                    if ( mouseX < 40 ) {
                        pressedButton = 0;
                        DrawBeveledBox(IPoint(0,440), IPoint(40,479), false);
                    } else if ( mouseX > 440 ) {
                        pressedButton = 1;
                        DrawBeveledBox(IPoint(760,440), IPoint(799,479), false);
                    }
                }
                
            }
            else if ( event.m_EventID == e_EventMouseUp )
            {
                int16_t mouseX = event.Data.Mouse.x;
                int16_t mouseY = event.Data.Mouse.y;

                if ( mouseY > 440 )
                {
                    if ( pressedButton == 0 && mouseX < 40 ) {
                        Display::Instance.SetFgColor(0xffff);
                        Display::Instance.FillRect(0, 0, screenRes.x - 1, screenRes.y - 1);
                    } else if ( pressedButton == 1 && mouseX > 760 ) {
                        break;
                    }
                }
                DrawBeveledBox(IPoint(0,440), IPoint(40,479), true);
                DrawBeveledBox(IPoint(760,440), IPoint(799,479), true);
                pressedButton = -1;
            }
        }
//        if ( pressedButton == -1 && TouchDriver::Instance.IsPressed() )
        {
            TouchDriver::Instance.GetCursorPos(&x, &y);
            
            //            if ( prevX != ~0 ) {
            //                Display::Instance.FillCircle(LCD_RGB(255,255,255), prevX, prevY, 10);
            //            }
            if ( y > 440 )
            {
                if ( x < 40 ) {
                    continue;
                    //                    Display::Instance.FillRect(0, 0, screenRes.x - 1, screenRes.y - 1, 0xffff);
                } else if ( x > 760 ) {
                    continue;
                }
            }
            //            Display::Instance.FillCircle(rand(), x, y, 4);
            Display::Instance.SetFgColor(LCD_RGB(0,255,0));
            Display::Instance.FillCircle(x, y, 4);
            Display::Instance.SetFgColor(LCD_RGB(0,255,255));
            Display::Instance.FillCircle(x, y, 2);
            //            prevX = x;
            //            prevY = y;
        }
    }
}
#endif


Signal<void, float> SignalTest;

class PaintView : public SignalTarget
{
public:
    PaintView() {
        SignalTest.Connect(this, &PaintView::SlotTest);
    }
    
    void SlotTest(float arg)
    {
        printf("%f\n", arg);
    }
    
};
*/

/*!
 * \brief Initialize the SWO trace port for debug message printing
 * \param portBits Port bit mask to be configured
 * \param cpuCoreFreqHz CPU core clock frequency in Hz
 */
#if 0
void SWO_Init(uint32_t portBits, uint32_t cpuCoreFreqHz)
{
  uint32_t SWOSpeed = 2000000; /* default 64k baud rate */
  uint32_t SWOPrescaler = (cpuCoreFreqHz / SWOSpeed) - 1; /* SWOSpeed in Hz, note that cpuCoreFreqHz is expected to be match the CPU core clock */
 
#if 0
  CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; /* enable trace in core debug */
//  *((volatile unsigned *)(ITM_BASE + 0x400F0)) = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
//  *((volatile unsigned *)(ITM_BASE + 0x40010)) = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
//  *((volatile unsigned *)(ITM_BASE + 0x00FB0)) = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */

  TPI->SPPR = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
  TPI->FFCR = 0x100;
  TPI->ACPR = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
  ITM->LAR  = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */


  ITM->TCR = 0x00010000 /*ITM_TCR_TraceBusID_Msk*/ | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_ITMENA_Msk; /* ITM Trace Control Register */
  ITM->TER = portBits; /* ITM Trace Enable Register. Enabled tracing on stimulus ports. One bit per stimulus port. */
  ITM->TPR = ITM_TPR_PRIVMASK_Msk; /* ITM Trace Privilege Register */
  ITM->PORT[0].u32 = 0;

  // *((volatile unsigned *)(ITM_BASE + 0x01000)) = 0x400003FE; /* DWT_CTRL */
  // *((volatile unsigned *)(ITM_BASE + 0x40304)) = 0x00000100; /* Formatter and Flush Control Register */

//  DWT->CTRL = 0x400003FE; /* DWT_CTRL */
//  DWT->CTRL = 0x40000000; /* DWT_CTRL */
//  TPI->FFCR = 0x00000100; /* Formatter and Flush Control Register */
#endif
}

void SWO_PrintChar(char c, uint8_t portNo)
{
  volatile int timeout;
 
  /* Check if Trace Control Register (ITM->TCR at 0xE0000E80) is set */
  if ((ITM->TCR&ITM_TCR_ITMENA_Msk) == 0) { /* check Trace Control Register if ITM trace is enabled*/
    return; /* not enabled? */
  }
  /* Check if the requested channel stimulus port (ITM->TER at 0xE0000E00) is enabled */
  if ((ITM->TER & (1ul<<portNo))==0) { /* check Trace Enable Register if requested port is enabled */
    return; /* requested port not enabled? */
  }
  timeout = 5000; /* arbitrary timeout value */
  while (ITM->PORT[0].u32 == 0) {
    /* Wait until STIMx is ready, then send data */
    timeout--;
    if (timeout==0) {
      return; /* not able to send */
    }
  }
  ITM->PORT[0].u16 = 0x08 | (int(c)<<8);
}
#endif

void SlotRenderTest1(Ptr<View> view);
void SlotRenderTest2(Ptr<View> view);
void SlotRenderTest3(Ptr<View> view);
void SlotRenderTest4(Ptr<View> view);
void SlotPaintTest(Ptr<View> view);
void SlotScrollTest(Ptr<View> view);

void SlotRenderTest1(Ptr<View> view)
{
    gui.RemoveView(view);
    
    Ptr<RenderTest1> test = ptr_new<RenderTest1>();
    Rect viewFrame = gui.GetScreenFrame();
    test->SetFrame(viewFrame);
  
    test->SignalDone.Connect(SlotRenderTest2);
    gui.AddView(test);
}    

void SlotRenderTest2(Ptr<View> view)
{
    gui.RemoveView(view);
    
    Ptr<RenderTest2> test = ptr_new<RenderTest2>();
    Rect viewFrame = gui.GetScreenFrame();
    test->SetFrame(viewFrame);
    
    test->SignalDone.Connect(SlotRenderTest3);
    gui.AddView(test);
}

void SlotRenderTest3(Ptr<View> view)
{
    gui.RemoveView(view);
    
    Ptr<RenderTest3> test = ptr_new<RenderTest3>();
    Rect viewFrame = gui.GetScreenFrame();
    test->SetFrame(viewFrame);
    
    test->SignalDone.Connect(SlotRenderTest4);
    gui.AddView(test);
}

void SlotRenderTest4(Ptr<View> view)
{
    gui.RemoveView(view);
    
    Ptr<RenderTest4> test = ptr_new<RenderTest4>();
    Rect viewFrame = gui.GetScreenFrame();
    test->SetFrame(viewFrame);
    
    test->SignalDone.Connect(SlotPaintTest);
    gui.AddView(test);
}

void SlotPaintTest(Ptr<View> view)
{
    gui.RemoveView(view);
    
    Ptr<PaintView> test = ptr_new<PaintView>();
    Rect viewFrame = gui.GetScreenFrame();
    test->SetFrame(viewFrame);
  
    test->SignalDone.Connect(SlotScrollTest);
    gui.AddView(test);
}    

void SlotScrollTest(Ptr<View> view)
{
    gui.RemoveView(view);
    
    Ptr<ScrollTestView> test = ptr_new<ScrollTestView>();
    Rect viewFrame = gui.GetScreenFrame();
    test->SetFrame(viewFrame);
  
    test->SignalDone.Connect(SlotRenderTest1);
    gui.AddView(test);
}    

uint8_t sdram_access_test()
{
	uint32_t i;
	uint32_t *pul = (uint32_t *)SDRAM_CS_ADDR;

	for (i = 0; i < SDRAMC_TEST_LENGTH / 4; ++i) {
		if (i & 1) {
			pul[i] = SDRAMC_TEST_ODD_TAG | (i);
		} else {
			pul[i] = SDRAMC_TEST_EVEN_TAG | (i);
		}
	}

        pul[0] = 0xdeadbabe;
	for (i = 1; i < SDRAMC_TEST_LENGTH / 4; ++i) {
		if (i & 1) {
			if (pul[i] != (SDRAMC_TEST_ODD_TAG | (i))) {
                        printf("MEMTEST failed at 0x%08" PRIx32 "\n", i*4);
				return SDRAMC_ERROR;
			}
		} else {
			if (pul[i] != (SDRAMC_TEST_EVEN_TAG | (i))) {
                        printf("MEMTEST failed at 0x%08" PRIx32 "\n", i*4);
				return SDRAMC_ERROR;
			}
		}
	}

	return SDRAMC_OK;
}

int g_CurrentSensor = -1;
int g_EnvSensor = -1;

void MeasureCurrent()
{
    INA3221Values values;
    if (INA3221_GetMeasurements(g_CurrentSensor, &values) >= 0)
    {
        printf("1: %.3f/%.3f (%.3f), 2: %.3f/%.3f (%.3f), 3: %.3f/%.3f (%.3f)\n", values.Voltages[0], values.Currents[0] * 1000.0, values.Voltages[0] * values.Currents[0] * 1000.0, values.Voltages[1], values.Currents[1] * 1000.0, values.Voltages[1] * values.Currents[1] * 1000.0, values.Voltages[2], values.Currents[2] * 1000.0, values.Voltages[2] * values.Currents[2] * 1000.0);
    }

    BME280Values envValues;
    if (BME280IOCTL_GetValues(g_EnvSensor, &envValues) >= 0) {
        printf("Temp: %.3f, Pres: %.3f, Hum: %.3f\n", envValues.m_Temperature, envValues.m_Pressure / 100.0, envValues.m_Humidity);
    }
}

static void SetupDevices()
{
    kernel::Kernel::RegisterDevice("uart/0", ptr_new<kernel::UARTDriver>(kernel::UART::Channels::Channel4_D18D19));

    int file = open("/dev/uart/0", O_WRONLY);
    
    if (file >= 0)
    {
        if (file != 0) {
            kernel::Kernel::DupeFile(file, 0);
        }
        if (file != 1) {
            kernel::Kernel::DupeFile(file, 1);
        }
        if (file != 2) {
            kernel::Kernel::DupeFile(file, 2);
        }
        if (file != 0 && file != 1 && file != 2) {
            close(file);
        }
        write(0, "Testing testing\n", strlen("Testing testing\n"));
    }
}

void Thread2(void* args);
/*{
    for (;;)
    {
        printf("Second task!!!\n");
//        Schedule();
    }
}*/

void MeasureThread(void* args)
{
//    __enable_irq();

    INA3221ShuntConfig shuntConfig;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_1] = 47.0e-3;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_2] = 47.0e-3;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_3] = 47.0e-3 * 0.5;

    g_CurrentSensor = open("/dev/ina3221/0", O_RDWR);
    if (g_CurrentSensor >= 0) {
        INA3221_SetShuntConfig(g_CurrentSensor, shuntConfig);
    } else {
        printf("ERROR: Failed to open current sensor\n");
    }
    g_EnvSensor = open("/dev/bme280/0", O_RDWR);
    if (g_EnvSensor < 0) {
        printf("ERROR: Failed to open environment sensor\n");
    }


    for (;;)
    {
        MeasureCurrent();
        snooze(500000);
//        printf("First task!!!\n");
//        Schedule();
    }
}

int main(void)
{
//    SystemInit();
   
    __disable_irq();


    SAME70System::EnablePeripheralClock(ID_PIOA);
    SAME70System::EnablePeripheralClock(ID_PIOB);
    SAME70System::EnablePeripheralClock(ID_PIOC);
    SAME70System::EnablePeripheralClock(ID_PIOD);
    SAME70System::EnablePeripheralClock(ID_PIOE);

    SAME70System::EnablePeripheralClock(SYSTEM_TIMER_PERIPHERALID1);
    SAME70System::EnablePeripheralClock(SYSTEM_TIMER_PERIPHERALID2);
    SAME70System::EnablePeripheralClock(ID_TC0_CHANNEL2);

//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK2 | TC_CMR_WAVE_Msk | TC_CMR_WAVESEL_UP_RC;
//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_IER = TC_IER_CPCS_Msk; // IRQ on C compare.
//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_RC = SAME70System::GetFrequencyPeripheral() / 8000; // 18750; // Wrap every ms (150e6/8 = 1.8750.000)
    
//    SYSTEM_TIMER->TC_BMR = TC_BMR_TC2XC2S_TIOA1_Val;// Clock XC2 is TIOA1

    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_EMR = TC_EMR_NODIVCLK_Msk; // Run at undivided peripheral clock.
    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CMR = TC_CMR_WAVE_Msk | TC_CMR_WAVESEL_UP; // | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
    
//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_CCR = TC_CCR_CLKEN_Msk;
    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CCR = TC_CCR_SWTRG_Msk;
    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CCR = TC_CCR_CLKEN_Msk;
    SYSTEM_TIMER->TC_BCR = TC_BCR_SYNC_Msk;
    
/*    ITM->TCR |= ITM_TCR_ITMENA_Msk;
    ITM->TER |= 0x01;    */
//    SWO_Init(0x1, CLOCK_CPU_FREQUENCY);

    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('\n');
    ITM_SendChar(0);
  
    RGBLED_R.Write(false);
    RGBLED_G.Write(false);
    RGBLED_B.Write(false);
    
    SAME70System::ResetWatchdog();
    kernel::Kernel::Initialize();


    kernel::start_scheduler();
}

void RunTests(void* args)
{
    Tests tests;

    exit_thread(0);
}

void kernel::InitThreadMain(void* argument)
{
    SetupDevices();
    
    printf("InitializeNewLibMutexes()");
    InitializeNewLibMutexes();

    printf("Setup I2C drivers\n");

    kernel::Kernel::RegisterDevice("i2c/0", ptr_new<kernel::I2CDriver>(kernel::I2CDriver::Channels::Channel0));
    kernel::Kernel::RegisterDevice("i2c/2", ptr_new<kernel::I2CDriver>(kernel::I2CDriver::Channels::Channel2));
    kernel::Kernel::RegisterDevice("ina3221/0", ptr_new<kernel::INA3221Driver>("/dev/i2c/2"));
    kernel::Kernel::RegisterDevice("bme280/0", ptr_new<kernel::BME280Driver>("/dev/i2c/2"));

    static int32_t args[] = {1, 2, 3};
    spawn_thread("SensorDumper", MeasureThread, 0, args);
    spawn_thread("Tester", RunTests, 0, args);

    printf("Start I2S clock output\n");
    //SAME70System::EnablePeripheralClock(ID_PMC);
/*    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD; // | PMC_WPMR_WPEN_Msk; // Remove register write protection.
    PMC->PMC_PCK[1] = PMC_PCK_CSS_MAIN_CLK | PMC_PCK_PRES(0);
    PMC->PMC_SCER = PMC_SCER_PCK1_Msk;
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN_Msk; // Write protect registers.
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN21_bm, DigitalPort::PeripheralID::B); // PCK1
    */
    printf("Initialize display.\n");    

    {
        DEBUG_DISABLE_IRQ();
//        GfxDriverIO::Setup();
        Display::Instance.InitDisplay();
    }

    {
        DEBUG_DISABLE_IRQ();
    g_SDCardBlockDevice.Initialize();;
    g_FileSystem.Initialize(&g_SDCardBlockDevice);

    kernel::Directory* rootDir = g_FileSystem.OpenDir("");
    if (rootDir != nullptr)
    {
        kernel::FileEntry entry;
        while(rootDir->ReadDir(&entry))
        {
            printf("%s : %" PRIu32 "\n", entry.m_Name, entry.m_Size);
        }
    }
    }
    printf("Initializing touch driver.\n");
    kernel::TouchDriver::Instance.Initialize("/dev/i2c/0");
    gui.Initialize();

    printf("Clear screen.\n");
    Display::Instance.SetFgColor(0xffff);
    Display::Instance.SetWindow(0, 0, 799, 479);
    Display::Instance.FillRect(0, 0, 799, 479);

#if 1
//    Ptr<PaintView> test = ptr_new<PaintView>();
    Ptr<RenderTest1> test = ptr_new<RenderTest1>();
//    Ptr<ScrollTestView> test = ptr_new<ScrollTestView>();
    Rect viewFrame = gui.GetScreenFrame();
//    viewFrame.Resize(40.0f, 40.0f, -40.0f, -40.0f);
    test->SetFrame(viewFrame);
  
    test->SignalDone.Connect(SlotRenderTest2);
    gui.AddView(test);
    test = nullptr;

    printf("Start GUI.\n");

//    uint32_t lastCurrentUpdateTime = Clock::GetTime();    

//    EventTimer currentMeasureTimer;
//    currentMeasureTimer.SignalTrigged.Connect(MeasureCurrent);
//    currentMeasureTimer.Start(500*1000);
    

    for (;;)
    {
        //EventTimer::Tick();
        gui.Tick();
//        snooze(5000);
        //printf("Starting new frame\n");
    }
#endif
//    Display::Instance.DrawHLine(23300, 230, 221, 16);    
//    Display::Instance.FillCircle(23300, 238, 202, 21);

/*
    while (1) 
    {
        SAME70System::ResetWatchdog();
//        Display::Instance.FillRect(0, 0, 799, 479, rand());

        DisplayTest1();
        DisplayTest2();
        DisplayTest3();
        DisplayTest4();
//        Painter();

    }*/
}
