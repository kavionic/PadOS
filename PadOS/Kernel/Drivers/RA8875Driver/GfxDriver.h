// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 16.01.2014 22:23:28

#pragma once


#include <stdint.h>
#include "Font.h"
#include "System/Math/Point.h"
#include "System/Math/Rect.h"
#include "System/Utils/Utils.h"
#include "SystemSetup.h"

namespace kernel
{


class File;

#define LCD_DEFAULT_ADDRESS_MODE (/*ILI9481_ADDRESS_MODE_VFLIP |*/ ILI9481_ADDRESS_MODE_REFRESH_BOTTOM_TO_TOP | ILI9481_ADDRESS_MODE_RGB_BGR)



#define LCD_RED_MASK   0xF800
#define LCD_GREEN_MASK 0x07E0
#define LCD_BLUE_MASK  0x001F

#define LCD_RGB(r,g,b) ( (int16_t(r>>3)<<11) | (int16_t(g>>2)<<5) | (b>>3) )

static const uint8_t GD_TEXT_FILL_TO_END         = 0x01;
static const uint8_t GD_TEXT_RENDER_PARTIAL_CHAR = 0x02;
static const uint8_t GD_TEXT_TRANSPARENT         = 0x04;


#define RA8875_PWRR        0x01 // Power and Display Control Register
#define   RA8875_PWRR_SW_RESET_bp   0
#define   RA8875_PWRR_SW_RESET_bm   BIT8(RA8875_PWRR_SW_RESET_bp, 1)

#define   RA8875_PWRR_SLEEP_MODE_bp 1
#define   RA8875_PWRR_SLEEP_MODE_bm BIT8(RA8875_PWRR_SLEEP_MODE_bp, 1)

#define   RA8875_PWRR_DISPLAY_ON_bp 7
#define   RA8875_PWRR_DISPLAY_ON_bm BIT8(RA8875_PWRR_DISPLAY_ON_bp, 1)

#define RA8875_MRWC        0x02 // Memory Read/Write Command
#define RA8875_PCSR        0x04 // Pixel Clock Setting Register
#define RA8875_SROC        0x05 // Serial Flash/ROM Configuration Register
#define RA8875_SFCLR       0x06 // Serial Flash/ROM CLK Setting Register
#define RA8875_SYSR        0x10 // System Configuration Register
#define RA8875_GPI         0x12 // General Purpose Input from pin KIN[4:0]
#define RA8875_GPO         0x13 // General Purpose Output to KOUT[3:0]
#define RA8875_HDWR        0x14 // LCD Horizontal Display Width Register
#define RA8875_HNDFTR      0x15 // Horizontal Non-Display Period Fine Tuning Option Register
#define RA8875_HNDR        0x16 // LCD Horizontal Non-Display Period Register
#define RA8875_HSTR        0x17 // HSYNC Start Position Register
#define RA8875_HPWR        0x18 // HSYNC Pulse Width Register
#define RA8875_VDHR0       0x19 // LCD Vertical Display Height Register
#define RA8875_VDHR1       0x1A // LCD Vertical Display Height Register 0
#define RA8875_VNDR0       0x1B // LCD Vertical Non-Display Period Register
#define RA8875_VNDR1       0x1C // LCD Vertical Non-Display Period Register
#define RA8875_VSTR0       0x1D // VSYNC Start Position Register
#define RA8875_VSTR1       0x1E // VSYNC Start Position Register
#define RA8875_VPWR        0x1F // VSYNC Pulse Width Register
#define RA8875_DPCR        0x20 // Display Configuration Register
#define RA8875_FNCR0       0x21 // Font Control Register 0
#define RA8875_FNCR1       0x22 // Font Control Register 1
#define RA8875_CGSR        0x23 // CGRAM Select Register
#define RA8875_HOFS0       0x24 // Horizontal Scroll Offset Register 0
#define RA8875_HOFS1       0x25 // Horizontal Scroll Offset Register 1
#define RA8875_VOFS0       0x26 // Vertical Scroll Offset Register 0
#define RA8875_VOFS1       0x27 // Vertical Scroll Offset Register 1
#define RA8875_FLDR        0x29 // Font Line Distance Setting Register
#define RA8875_F_CURXL     0x2A // Font Write Cursor Horizontal Position Register 0
#define RA8875_F_CURXH     0x2B // Font Write Cursor Horizontal Position Register 1
#define RA8875_F_CURYL     0x2C // Font Write Cursor Vertical Position Register 0
#define RA8875_F_CURYH     0x2D // Font Write Cursor Vertical Position Register 1
#define RA8875_FWTS        0x2E // Font Write Type Setting Register
#define RA8875_SFRS        0x2F // Serial Font ROM Setting
#define RA8875_HSAW0       0x30 // Horizontal Start Point 0 of Active Window
#define RA8875_HSAW1       0x31 // Horizontal Start Point 1 of Active Window
#define RA8875_VSAW0       0x32 // Vertical Start Point 0 of Active Window
#define RA8875_VSAW1       0x33 // Vertical Start Point 1 of Active Window
#define RA8875_HEAW0       0x34 // Horizontal End Point 0 of Active Window
#define RA8875_HEAW1       0x35 // Horizontal End Point 1 of Active Window
#define RA8875_VEAW0       0x36 // Vertical End Point of Active Window 0
#define RA8875_VEAW1       0x37 // Vertical End Point of Active Window 1
#define RA8875_HSSW0       0x38 // Horizontal Start Point 0 of Scroll Window
#define RA8875_HSSW1       0x39 // Horizontal Start Point 1 of Scroll Window
#define RA8875_VSSW0       0x3A // Vertical Start Point 0 of Scroll Window
#define RA8875_VSSW1       0x3B // Vertical Start Point 1 of Scroll Window
#define RA8875_HESW0       0x3C // Horizontal End Point 0 of Scroll Window
#define RA8875_HESW1       0x3D // Horizontal End Point 1 of Scroll Window
#define RA8875_VESW0       0x3E // Vertical End Point 0 of Scroll Window
#define RA8875_VESW1       0x3F // Vertical End Point 1 of Scroll Window
#define RA8875_MWCR0       0x40 // Memory Write Control Register 0

#define   RA8875_MWCR0_RD_AUTOINC_DISABLE_bp  0
#define   RA8875_MWCR0_RD_AUTOINC_DISABLE_bm  BIT8(RA8875_MWCR0_RD_AUTOINC_DISABLE_bp, 1)

#define   RA8875_MWCR0_WR_AUTOINC_DISABLE_bp  1
#define   RA8875_MWCR0_WR_AUTOINC_DISABLE_bm  BIT8(RA8875_MWCR0_WR_AUTOINC_DISABLE_bp, 1)

#define   RA8875_MWCR0_DIRECTON_bp  2
#define   RA8875_MWCR0_DIRECTON_bm  BIT8(RA8875_MWCR0_DIRECTON_bp,0x03)
#define   RA8875_MWCR0_LR_TD_bg     BIT8(RA8875_MWCR0_DIRECTON_bp,0) // Left -> Right then Top -> Down
#define   RA8875_MWCR0_RL_TD_bg     BIT8(RA8875_MWCR0_DIRECTON_bp,1) // Right -> Left then Top -> Down
#define   RA8875_MWCR0_TD_LR_bg     BIT8(RA8875_MWCR0_DIRECTON_bp,2) // Top -> Down then Left -> Right
#define   RA8875_MWCR0_DT_LR_bg     BIT8(RA8875_MWCR0_DIRECTON_bp,3) // Down -> Top then Left -> Right

#define   RA8875_MWCR0_CURSOR_BLINK_ENABLE_bp  5
#define   RA8875_MWCR0_CURSOR_BLINK_ENABLE_bm  BIT8(RA8875_MWCR0_CURSOR_BLINK_ENABLE_bp, 1)

#define   RA8875_MWCR0_CURSOR_ENABLE_bp  6
#define   RA8875_MWCR0_CURSOR_ENABLE_bm  BIT8(RA8875_MWCR0_CURSOR_ENABLE_bp, 1)

#define   RA8875_MWCR0_TEXT_MODE_bp  7
#define   RA8875_MWCR0_TEXT_MODE_bm  BIT8(RA8875_MWCR0_TEXT_MODE_bp, 1)

#define RA8875_MWCR1       0x41 // Memory Write Control Register 1
#define RA8875_BTCR        0x44 // Blink Time Control Register
#define RA8875_MRCD        0x45 // Memory Read Cursor Direction
#define RA8875_CURH0       0x46 // Memory Write Cursor Horizontal Position Register 0
#define RA8875_CURH1       0x47 // Memory Write Cursor Horizontal Position Register 1
#define RA8875_CURV0       0x48 // Memory Write Cursor Vertical Position Register 0
#define RA8875_CURV1       0x49 // Memory Write Cursor Vertical Position Register 1
#define RA8875_RCURH0      0x4A // Memory Read Cursor Horizontal Position Register 0
#define RA8875_RCURH1      0x4B // Memory Read Cursor Horizontal Position Register 1
#define RA8875_RCURV0      0x4C // Memory Read Cursor Vertical Position Register 0
#define RA8875_RCURV1      0x4D // Memory Read Cursor Vertical Position Register 1
#define RA8875_CURHS       0x4E // Font Write Cursor and Memory Write Cursor Horizontal Size Register
#define RA8875_CURVS       0x4F // Font Write Cursor Vertical Size Register
#define RA8875_BECR0       0x50 // BTE Function Control Register 0         

#define   RA8875_BECR0_DST_LINEAR_bp 5
#define   RA8875_BECR0_DST_LINEAR_bm BIT8(RA8875_BECR0_DST_LINEAR_bp, 1)
#define   RA8875_BECR0_DST_LINEAR RA8875_BECR0_DST_LINEAR_bm
#define   RA8875_BECR0_DST_BLOCK  0

#define   RA8875_BECR0_SRC_LINEAR_bp 6
#define   RA8875_BECR0_SRC_LINEAR_bm BIT8(RA8875_BECR0_SRC_LINEAR_bp, 1)
#define   RA8875_BECR0_SRC_LINEAR RA8875_BECR0_SRC_LINEAR_bm
#define   RA8875_BECR0_SRC_BLOCK  0

#define   RA8875_BECR0_ENABLE_bp     7
#define   RA8875_BECR0_ENABLE_bm     BIT8(RA8875_BECR0_ENABLE_bp, 1)

#define RA8875_BECR1       0x51 // BTE Function Control Register1

#define   RA8875_BTE_OP_bp                  0
#define   RA8875_BTE_OP_WRITE_ROP           BIT8(RA8875_BTE_OP_bp, 0)
#define   RA8875_BTE_OP_READ                BIT8(RA8875_BTE_OP_bp, 1)
#define   RA8875_BTE_OP_MOVE_POS_ROP        BIT8(RA8875_BTE_OP_bp, 2)
#define   RA8875_BTE_OP_MOVE_NEG_ROP        BIT8(RA8875_BTE_OP_bp, 3)
#define   RA8875_BTE_OP_WRITE_TRAN          BIT8(RA8875_BTE_OP_bp, 4)
#define   RA8875_BTE_OP_MOVE_POS_TRAN       BIT8(RA8875_BTE_OP_bp, 5)
#define   RA8875_BTE_OP_PFILL_ROP           BIT8(RA8875_BTE_OP_bp, 6)
#define   RA8875_BTE_OP_PFILL_TRAN          BIT8(RA8875_BTE_OP_bp, 7)
#define   RA8875_BTE_OP_COLOR_EXP           BIT8(RA8875_BTE_OP_bp, 8)
#define   RA8875_BTE_OP_COLOR_EXP_TRAN      BIT8(RA8875_BTE_OP_bp, 9)
#define   RA8875_BTE_OP_MOVE_COLOR_EXP      BIT8(RA8875_BTE_OP_bp, 10)
#define   RA8875_BTE_OP_MOVE_COLOR_EXP_TRAN BIT8(RA8875_BTE_OP_bp, 11)
#define   RA8875_BTE_OP_FILL                BIT8(RA8875_BTE_OP_bp, 12)

#define   RA8875_BTE_ROP_bp       4
#define   RA8875_BTE_ROP_BLACK    BIT8(RA8875_BTE_ROP_bp, 0)  // 0 ( Blackness )
#define   RA8875_BTE_ROP_nSxnD    BIT8(RA8875_BTE_ROP_bp, 1)  // ~S?~D or ~ ( S+D )
#define   RA8875_BTE_ROP_nSxD     BIT8(RA8875_BTE_ROP_bp, 2)  // ~S?D
#define   RA8875_BTE_ROP_nS       BIT8(RA8875_BTE_ROP_bp, 3)  // ~S
#define   RA8875_BTE_ROP_SxnD     BIT8(RA8875_BTE_ROP_bp, 4)  // S?~D
#define   RA8875_BTE_ROP_nD       BIT8(RA8875_BTE_ROP_bp, 5)  // ~D
#define   RA8875_BTE_ROP_SxorD    BIT8(RA8875_BTE_ROP_bp, 6)  // S^D
#define   RA8875_BTE_ROP_nSplusnD BIT8(RA8875_BTE_ROP_bp, 7)  // ~S+~D or ~ ( S?D )
#define   RA8875_BTE_ROP_SxD      BIT8(RA8875_BTE_ROP_bp, 8)  // S?D
#define   RA8875_BTE_ROP_SxorDn   BIT8(RA8875_BTE_ROP_bp, 9)  // ~ ( S^D )
#define   RA8875_BTE_ROP_D        BIT8(RA8875_BTE_ROP_bp, 10) // D
#define   RA8875_BTE_ROP_nSplusD  BIT8(RA8875_BTE_ROP_bp, 11) // ~S+D
#define   RA8875_BTE_ROP_S        BIT8(RA8875_BTE_ROP_bp, 12) // S
#define   RA8875_BTE_ROP_SplusnD  BIT8(RA8875_BTE_ROP_bp, 13) // S+~D
#define   RA8875_BTE_ROP_SplusD   BIT8(RA8875_BTE_ROP_bp, 14) // S+D
#define   RA8875_BTE_ROP_WHITE    BIT8(RA8875_BTE_ROP_bp, 15) // 1 ( Whiteness )



#define RA8875_LTPR0       0x52 // Layer Transparency Register0            
#define RA8875_LTPR1       0x53 // Layer Transparency Register1            
#define RA8875_HSBE0       0x54 // Horizontal Source Point 0 of BTE        
#define RA8875_HSBE1       0x55 // Horizontal Source Point 1 of BTE        
#define RA8875_VSBE0       0x56 // Vertical Source Point 0 of BTE          
#define RA8875_VSBE1       0x57 // Vertical Source Point 1 of BTE          
#define RA8875_HDBE0       0x58 // Horizontal Destination Point 0 of BTE   
#define RA8875_HDBE1       0x59 // Horizontal Destination Point 1 of BTE   
#define RA8875_VDBE0       0x5A // Vertical Destination Point 0 of BTE     
#define RA8875_VDBE1       0x5B // Vertical Destination Point 1 of BTE     
#define RA8875_BEWR0       0x5C // BTE Width Register 0                    
#define RA8875_BEWR1       0x5D // BTE Width Register 1                    
#define RA8875_BEHR0       0x5E // BTE Height Register 0                   
#define RA8875_BEHR1       0x5F // BTE Height Register 1                   
#define RA8875_BGCR0       0x60 // Background Color Register 0             
#define RA8875_BGCR1       0x61 // Background Color Register 1             
#define RA8875_BGCR2       0x62 // Background Color Register 2             
#define RA8875_FGCR0       0x63 // Foreground Color Register 0             
#define RA8875_FGCR1       0x64 // Foreground Color Register 1             
#define RA8875_FGCR2       0x65 // Foreground Color Register 2             
#define RA8875_PTNO        0x66 // Pattern Set No for BTE                   
#define RA8875_BGTR0       0x67 // Background Color Register for Transparent 0
#define RA8875_BGTR1       0x68 // Background Color Register for Transparent 1
#define RA8875_BGTR2       0x69 // Background Color Register for Transparent 2
#define RA8875_TPCR0       0x70 // Touch Panel Control Register 0
#define RA8875_TPCR1       0x71 // Touch Panel Control Register 1
#define RA8875_TPXH        0x72 // Touch Panel X High Byte Data Register
#define RA8875_TPYH        0x73 // Touch Panel Y High Byte Data Register
#define RA8875_TPXYL       0x74 // Touch Panel X/Y Low Byte Data Register
#define RA8875_GCHP0       0x80 // Graphic Cursor Horizontal Position Register 0
#define RA8875_GCHP1       0x81 // Graphic Cursor Horizontal Position Register 1
#define RA8875_GCVP0       0x82 // Graphic Cursor Vertical Position Register 0
#define RA8875_GCVP1       0x83 // Graphic Cursor Vertical Position Register 1
#define RA8875_GCC0        0x84 // Graphic Cursor Color 0
#define RA8875_GCC1        0x85 // Graphic Cursor Color 1
#define RA8875_PLLC1       0x88 // PLL Control Register 1
#define RA8875_PLLC2       0x89 // PLL Control Register 2
#define RA8875_P1CR        0x8A // PWM1 Control Register
#define RA8875_P1DCR       0x8B // PWM1 Duty Cycle Register
#define RA8875_P2CR        0x8C // PWM2 Control Register
#define RA8875_P2DCR       0x8D // PWM2 Control Register
#define RA8875_MCLR        0x8E // Memory Clear Control Register
#define RA8875_DCR         0x90 // Draw Line/Circle/Square Control Register
#define   RA8875_DCR_TRIANGLE_bp     0
#define   RA8875_DCR_TRIANGLE_bm     BIT8(RA8875_DCR_TRIANGLE_bp, 1)
#define   RA8875_DCR_SQUARE_bp       4
#define   RA8875_DCR_SQUARE_bm       BIT8(RA8875_DCR_SQUARE_bp, 1)
#define   RA8875_DCR_FILL_bp         5
#define   RA8875_DCR_FILL_bm         BIT8(RA8875_DCR_FILL_bp, 1)
#define   RA8875_DCR_CIRCLE_bp       6
#define   RA8875_DCR_CIRCLE_bm       BIT8(RA8875_DCR_CIRCLE_bp, 1)
#define   RA8875_DCR_LINE_SQR_TRI_bp 7
#define   RA8875_DCR_LINE_SQR_TRI_bm BIT8(RA8875_DCR_LINE_SQR_TRI_bp, 1)

#define RA8875_DLHSR0      0x91 // Draw Line/Square Horizontal Start Address Register0
#define RA8875_DLHSR1      0x92 // Draw Line/Square Horizontal Start Address Register1
#define RA8875_DLVSR0      0x93 // Draw Line/Square Vertical Start Address Register0
#define RA8875_DLVSR1      0x94 // Draw Line/Square Vertical Start Address Register1
#define RA8875_DLHER0      0x95 // Draw Line/Square Horizontal End Address Register0
#define RA8875_DLHER1      0x96 // Draw Line/Square Horizontal End Address Register1
#define RA8875_DLVER0      0x97 // Draw Line/Square Vertical End Address Register0
#define RA8875_DLVER1      0x98 // Draw Line/Square Vertical End Address Register1
#define RA8875_DCHR0       0x99 // Draw Circle Center Horizontal Address Register0
#define RA8875_DCHR1       0x9A // Draw Circle Center Horizontal Address Register1
#define RA8875_DCVR0       0x9B // Draw Circle Center Vertical Address Register0
#define RA8875_DCVR1       0x9C // Draw Circle Center Vertical Address Register1
#define RA8875_DCRR        0x9D // Draw Circle Radius Register
#define RA8875_ELL_CTRL    0xA0 // Draw Ellipse/Ellipse Curve/Circle Square Control Register
#define RA8875_ELL_A0      0xA1 // Draw Ellipse/Circle Square Long axis Setting Register
#define RA8875_ELL_A1      0xA2 // Draw Ellipse/Circle Square Long axis Setting Register
#define RA8875_ELL_B0      0xA3 // Draw Ellipse/Circle Square Short axis Setting Register
#define RA8875_ELL_B1      0xA4 // Draw Ellipse/Circle Square Short axis Setting Register
#define RA8875_DEHR0       0xA5 // Draw Ellipse/Circle Square Center Horizontal Address Register0
#define RA8875_DEHR1       0xA6 // Draw Ellipse/Circle Square Center Horizontal Address Register1
#define RA8875_DEVR0       0xA7 // Draw Ellipse/Circle Square Center Vertical Address Register0
#define RA8875_DEVR1       0xA8 // Draw Ellipse/Circle Square Center Vertical Address Register1
#define RA8875_DTPH0       0xA9 // Draw Triangle Point 2 Horizontal Address Register0
#define RA8875_DTPH1       0xAA // Draw Triangle Point 2 Horizontal Address Register1
#define RA8875_DTPV0       0xAB // Draw Triangle Point 2 Vertical Address Register0
#define RA8875_DTPV1       0xAC // Draw Triangle Point 2 Vertical Address Register1
#define RA8875_SSAR0       0xB0 // Source Starting Address REG0
#define RA8875_SSAR1       0xB1 // Source Starting Address REG 1
#define RA8875_SSAR2       0xB2 // Source Starting Address REG 2
#define RA8875_BWR0_DTNR0  0xB4 // Block Width REG 0(BWR0) / DMA Transfer Number REG 0 (DTNR0)
#define RA8875_BWR1        0xB5 // Block Width REG 1
#define RA8875_BHR0_DTNR1  0xB6 // Block Height REG 0 (BHR0) /DMA Transfer Number REG 1 (DTNR1)
#define RA8875_BHR1        0xB7 // Block Height REG 1
#define RA8875_SPWR0_DTNR2 0xB8 // Source Picture Width REG 0 (SPWR0) / DMA Transfer Number REG 2 (DTNR2)
#define RA8875_SPWR1       0xB9 // Source Picture Width REG 1
#define RA8875_DMACR       0xBF // DMA Configuration REG
#define RA8875_KSCR1       0xC0 // Key-Scan Control Register 1
#define RA8875_KSCR2       0xC1 // Key-Scan Controller Register 2
#define RA8875_KSDR0       0xC2 // Key-Scan Data Register
#define RA8875_KSDR1       0xC3 // Key-Scan Data Register
#define RA8875_KSDR2       0xC4 // Key-Scan Data Register
#define RA8875_GPIOX       0xC7 // Extra General Purpose IO Register
#define RA8875_FWSAXA0     0xD0 // Floating Windows Start Address XA 0
#define RA8875_FWSAXA1     0xD1 // Floating Windows Start Address XA 1
#define RA8875_FWSAYA0     0xD2 // Floating Windows Start Address YA 0
#define RA8875_FWSAYA1     0xD3 // Floating Windows Start Address YA 1
#define RA8875_FWW0        0xD4 // Floating Windows Width 0
#define RA8875_FWW1        0xD5 // Floating Windows Width 1
#define RA8875_FWH0        0xD6 // Floating Windows Height 0
#define RA8875_FWH1        0xD7 // Floating Windows Height 1
#define RA8875_FWDXA0      0xD8 // Floating Windows Display X Address 0
#define RA8875_FWDXA1      0xD9 // Floating Windows Display X Address 1
#define RA8875_FWDYA0      0xDA // Floating Windows Display Y Address 0
#define RA8875_FWDYA1      0xDB // Floating Windows Display Y Address 1
#define RA8875_SACS_MODE   0xE0 // Serial Flash/ROM Direct Access Mode
#define RA8875_SACS_ADDR   0xE1 // Serial Flash/ROM Direct Access Mode Address
#define RA8875_SACS_DATA   0xE2 // Serial Flash/ROM Direct Access Data Read
#define RA8875_INTC1       0xF0 // Interrupt Control Register1
#define RA8875_INTC2       0xF1 // Interrupt Control Register2


class GfxDriver
{
public:
    static GfxDriver Instance;
    
    enum Orientation_e { e_Portrait, e_Landscape };
    enum FillDirection_e { e_FillLeftDown, e_FillDownLeft };
    enum Font_e { e_FontSmall, e_FontNormal, e_FontLarge, e_Font7Seg, e_FontCount };
        
    GfxDriver();
    void InitDisplay();

    void SetOrientation(Orientation_e orientation);
    inline void SetFillDirection( FillDirection_e direction ) { m_FillDirection = direction; UpdateAddressMode(); }
    IPoint GetResolution() const { return (m_Orientation == e_Landscape) ? IPoint(800, 480) : IPoint(480, 800); }

    static inline uint16_t MakeColor(uint8_t r, uint8_t g, uint8_t b) { return ((r & 248) << 8) | ((g & 252) << 3) | ((b & 248) >> 3); }

    void SetFgColor(uint16_t color);
    void SetBgColor(uint16_t color);
    const FONT_INFO* GetFontDesc(Font_e fontID) const;
    void SetFont(Font_e fontID);
    float GetFontHeight(Font_e fontID) const;
    float GetStringWidth(Font_e fontID, const char* string, uint16_t length ) const;
    
    inline void SetCursor(const IPoint& pos) { m_Cursor = pos; }
    inline void SetCursor(int16_t x, int16_t y) { m_Cursor.x = x; m_Cursor.y = y; }
    inline const IPoint& GetCursor() const { return m_Cursor; }

    void FillRect(const IRect& frame);
    
    void WritePixel(int16_t x, int16_t y);
    
    void DrawLine(int x1, int y1, int x2, int y2);
    void DrawHLine(int x, int y, int l);
    void DrawVLine(int x, int y, int l);
    
    void FillCircle(int32_t x, int32_t y, int32_t radius);

    void BLT_FillRect(const IRect& frame);
    void BLT_DrawLine(int x1, int y1, int x2, int y2);
    void BLT_FillCircle(int32_t x, int32_t y, int32_t radius);
    void BLT_MoveRect(const IRect& srcRect, const IPoint& dstPos);
    
    uint32_t WriteString(const char* string, size_t strLength, const IRect& clipRect);
    uint8_t WriteStringTransparent(const char* string, uint8_t strLength, int16_t maxWidth);

    void DrawImage(File* file, int16_t width, int16_t height);
    
//private:
    void PLL_ini();


    void SetWindow(int x1, int y1, int x2, int y2);
    void SetWindow(const IRect& frame) { SetWindow(frame.left, frame.top, frame.right, frame.bottom); }
    inline void UpdateAddressMode()
    {
        if ( m_FillDirection == e_FillLeftDown )
        {
            if ( m_Orientation == e_Landscape ) {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_LR_TD_bg); // Left -> Right then Top -> Down
            } else {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_TD_LR_bg); // Top -> Down then Left -> Right
            }
        }
        else
        {
            if ( m_Orientation == e_Landscape ) {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_TD_LR_bg); // Top -> Down then Left -> Right
            } else {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_LR_TD_bg); // Left -> Right then Top -> Down
            }
        }
    }
    void FastFill(uint32_t words, uint16_t color);
    inline bool RenderGlyph(char character, const IRect& clipRect);

    void MemoryWrite_Position(int X,int Y)
    {
        WriteCommand(RA8875_CURH0, RA8875_CURH1, X);
        WriteCommand(RA8875_CURV0, RA8875_CURV1, Y);
        WriteCommand(RA8875_MRWC);
    }


    void Chk_Busy()
    {
        uint8_t temp;
        do
        {
            temp=ReadCommand();
        } while((temp&0x80)==0x80);
    }

    void Chk_BTE_Busy()
    {
        uint8_t temp;
        do
        {
            temp=ReadCommand();
        } while((temp&0x40)==0x40);
    }

    void Chk_DMA_Busy()
    {
        uint8_t temp;
        do
        {
            WriteCommand(RA8875_DMACR);
            temp = ReadData();
        } while((temp&0x01)==0x01);   
    }

    void WaitBlitter()
    {
        Chk_BTE_Busy();
        for (;;)
        {
            WriteCommand(RA8875_DCR);
            if (!(ReadData() & (RA8875_DCR_CIRCLE_bm | RA8875_DCR_LINE_SQR_TRI_bm))) break;
        }            
    }

    void Chk_Circle_Busy()
    {
        uint8_t temp;
        do
        {
            WriteCommand(RA8875_DMACR);
            temp = ReadData();
        } while((temp&0x01)==0x01);   
    }

    uint16_t ReadCommand()         { return LCD_REGISTERS->CMD; }
    void     WriteCommand(uint8_t cmd)                               { LCD_REGISTERS->CMD = cmd; }
    void     WriteCommand(uint8_t cmd, uint8_t data)                 { LCD_REGISTERS->CMD = cmd; LCD_REGISTERS->DATA = data; }
    void     WriteCommand(uint8_t cmdL, uint8_t cmdH, uint16_t data) { WriteCommand(cmdL, data & 0xff); WriteCommand(cmdH, data >> 8); }

    uint16_t ReadData()             { return LCD_REGISTERS->DATA; }
    void     WriteData(uint16_t data) { LCD_REGISTERS->DATA = data; }

    Orientation_e         m_Orientation;
    FillDirection_e       m_FillDirection;
    uint16_t              m_FgColor;
    uint16_t              m_BgColor;
    Font_e                m_Font;
    uint8_t               m_FontFirstChar;
    uint8_t               m_FontHeight;
    uint8_t               m_FontHeightFullBytes;
    uint8_t               m_FontHeightRemainingBits;
    uint8_t               m_FontCharSpacing;
    const uint8_t*        m_FontGlyphData;
    const FONT_CHAR_INFO* m_FontCharInfo;
    IPoint                m_Cursor;
};

} // namespace
