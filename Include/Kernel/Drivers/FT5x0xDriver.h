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
// Created: 19.03.2018 21:30:25


#pragma once

#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/KThread.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KSemaphore.h>
#include <Kernel/KMutex.h>
#include <Utils/Utils.h>
#include <Math/Point.h>

// FT5206

namespace kernel
{

#define FT5x0x_REG_G_CTRL 0x86
#define FT5x0x_REG_G_TIME_ENTER_MONITOR 0x87
#define FT5x0x_REG_G_PERIODE_ACTIVE 0x88
#define FT5x0x_REG_G_PERIODE_MONITOR 0x89

#define FT5x0x_REG_G_CIPHER 0xa3 // Vendor's chip ID
#define FT5x0x_REG_G_MODE 0xa4 // Interrupt status
#define FT5x0x_REG_G_PMODE 0xa5 // Power consumption mode
#define FT5x0x_REG_G_FIRMWARE_ID 0xa6 
#define FT5x0x_REG_G_STATE 0xa7 // Run mode of TPM
#define FT5x0x_REG_G_FT5201ID 0xa6 // CTPM Vendor ID
#define FT5x0x_REG_G_ERR 0xa9 // Error code
#define FT5x0x_REG_G_CLB 0xa // Configure TP module during calibration in Test Mode
#define FT5x0x_REG_G_B_AREA_TH 0xae // The threshold of big area.
#define FT5x0x_REG_LOG_MSG_CNT 0xfe // The log message count
#define FT5x0x_REG_LOG_CUR_CHAR 0xff // Current character of log message, will point to next character when one character is read.

#define FT5x0x_REG_G_MODE 0xa4
#define FT5x0x_REG_P_MODE 0xa5



struct FT5x0xOMTouchData
{
    uint8_t TOUCH_XH;
    uint8_t TOUCH_XL;
    uint8_t TOUCH_YH;
    uint8_t TOUCH_YL;
    uint8_t reserved[2];
};

#define FT5x0x_TP_REGISTER_COUNT   5

struct FT5x0xOMRegisters
{
    uint8_t DEVICE_MODE;
    uint8_t GEST_ID;
    uint8_t TD_STATUS;
    FT5x0xOMTouchData TOUCH_DATA[FT5x0x_TP_REGISTER_COUNT];
};

#define FT5x0x_DEVICE_MODE_MODE_bp   4
#define FT5x0x_DEVICE_MODE_MODE_bm   BIT8(FT5x0x_DEVICE_MODE_MODE_bp, 0x7)
#define FT5x0x_DEVICE_MODE_MODE(reg) (((reg) >> FT5x0x_DEVICE_MODE_MODE_bp) & FT5x0x_DEVICE_MODE_MODE_bm)

#define FT5x0x_DEVICE_MODE_MODE_NORMAL  0x00
#define FT5x0x_DEVICE_MODE_MODE_SYSINFO 0x01
#define FT5x0x_DEVICE_MODE_MODE_TEST    0x04

#define FT5x0x_GEST_ID_NO_GESTURE 0x00
#define FT5x0x_GEST_ID_MOVE_UP    0x10
#define FT5x0x_GEST_ID_MOVE_LEFT  0x14
#define FT5x0x_GEST_ID_MOVE_DOWN  0x18
#define FT5x0x_GEST_ID_MOVE_RIGHT 0x1c
#define FT5x0x_GEST_ID_ZOOM_IN    0x48
#define FT5x0x_GEST_ID_ZOOM_OUT   0x49


#define FT5x0x_TD_STATUS_PT_COUNT_bp   0
#define FT5x0x_TD_STATUS_PT_COUNT_bm   BIT8(FT5x0x_TD_STATUS_PT_COUNT_bp, 0xf)
#define FT5x0x_TD_STATUS_PT_COUNT(reg) (((reg) & FT5x0x_TD_STATUS_PT_COUNT_bm) >> FT5x0x_TD_STATUS_PT_COUNT_bp)

#define FT5x0x_TOUCH_XH_TOUCH_XH_bp   0
#define FT5x0x_TOUCH_XH_TOUCH_XH_bm   BIT8(FT5x0x_TOUCH_XH_TOUCH_XH_bp, 0xf)
#define FT5x0x_TOUCH_XH_TOUCH_XH(reg) (((reg) & FT5x0x_TOUCH_XH_TOUCH_XH_bm) >> FT5x0x_TOUCH_XH_TOUCH_XH_bp)

#define FT5x0x_TOUCH_XH_TOUCH_X(regL, regH) (regL | (FT5x0x_TOUCH_XH_TOUCH_XH(regH) << 8))

#define FT5x0x_TOUCH_XH_TOUCH_FLAGS_bp   6
#define FT5x0x_TOUCH_XH_TOUCH_FLAGS_bm   BIT8(FT5x0x_TOUCH_XH_TOUCH_FLAGS_bp, 0x3)
#define FT5x0x_TOUCH_XH_TOUCH_FLAGS(reg) (((reg) & FT5x0x_TOUCH_XH_TOUCH_FLAGS_bm) >> FT5x0x_TOUCH_XH_TOUCH_FLAGS_bp)

#define FT5x0x_TOUCH_FLAGS_DOWN     0
#define FT5x0x_TOUCH_FLAGS_UP       1
#define FT5x0x_TOUCH_FLAGS_CONTACT  2
#define FT5x0x_TOUCH_FLAGS_RESERVED 3

#define FT5x0x_TOUCH_YH_TOUCH_YH_bp   0
#define FT5x0x_TOUCH_YH_TOUCH_YH_bm   BIT8(FT5x0x_TOUCH_YH_TOUCH_YH_bp, 0xf)
#define FT5x0x_TOUCH_YH_TOUCH_YH(reg) (((reg) & FT5x0x_TOUCH_YH_TOUCH_YH_bm) >> FT5x0x_TOUCH_YH_TOUCH_YH_bp)

#define FT5x0x_TOUCH_YH_TOUCH_Y(regL, regH) (regL | (FT5x0x_TOUCH_YH_TOUCH_YH(regH) << 8))

#define FT5x0x_TOUCH_YH_TOUCH_ID_bp   4
#define FT5x0x_TOUCH_YH_TOUCH_ID_bm   BIT8(FT5x0x_TOUCH_YH_TOUCH_ID_bp, 0xf)
#define FT5x0x_TOUCH_YH_TOUCH_ID(reg) (((reg) & FT5x0x_TOUCH_YH_TOUCH_ID_bm) >> FT5x0x_TOUCH_YH_TOUCH_ID_bp)


class FT5x0xFile : public KFileNode
{
public:
    FT5x0xFile(int openFlags) : KFileNode(openFlags) {}

    port_id m_TargetPort = -1;
};

/*class FT5x0xDriverINode : public KINode
{
public:
    
};*/

class FT5x0xDriver : public PtrTarget, public KThread, public KFilesystemFileOps
{
public:
    FT5x0xDriver();
    ~FT5x0xDriver();

    void Setup(const char* devicePath, const DigitalPin& pinWAKE, const DigitalPin& pinRESET, const DigitalPin& pinINT, IRQn_Type irqNum, const char* i2cPath);

    virtual void* Run() override;

    virtual Ptr<KFileNode>  OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> inode, int flags) override;
    virtual void            CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;
    virtual void            DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    void PrintChipStatus();
    
    static IRQResult IRQHandler(IRQn_Type irq, void* userData) { return static_cast<FT5x0xDriver*>(userData)->HandleIRQ(); }
	IRQResult HandleIRQ();

    DigitalPin   m_PinWAKE;
    DigitalPin   m_PinRESET;
    DigitalPin   m_PinINT;
    
    int m_I2CDevice = -1;

    std::vector<FT5x0xFile*> m_OpenFiles;

    KMutex     m_Mutex;
    KSemaphore m_EventSemaphore;
    
    static const int MAX_POINTS = 10;
    os::IPoint m_TouchPositions[MAX_POINTS];
    
    FT5x0xDriver(const FT5x0xDriver&) = delete;
    FT5x0xDriver& operator=(const FT5x0xDriver&) = delete;
};


} // namespace
