// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.05.2020 21:30:24


#pragma once
#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/VFS/KDeviceNode.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/KMutex.h"
#include "System/Threads/Thread.h"
#include "System/Math/Point.h"

namespace kernel
{

constexpr uint8_t	GSLx680_REG_DATA		= 0x80; // Start of touch data (touch point register)
constexpr uint8_t	GSLx680_REG_TOUCH_COUNT	= 0x80; // Touch point count register
constexpr uint8_t	GSLx680_REG_COORDINATES	= 0x84; // 0x84~0xa8 ：Touch point coordinate register
constexpr uint8_t	GSLx680_REG_PROXIMITY	= 0xac; // Screen proximity register
constexpr uint8_t	GSLx680_REG_STATUS		= 0xb0; // Status register
constexpr uint8_t	GSLx680_REG_IRQ_COUNT	= 0xb4; // Chip internal interrupt count register
constexpr uint8_t	GSLx680_REG_POWER		= 0xbc; // Power register
constexpr uint8_t	GSLx680_REG_RESET		= 0xe0; // Chip reset register
constexpr uint8_t	GSLx680_REG_CLOCK		= 0xe4; // Chip clock register
constexpr uint8_t	GSLx680_REG_PAGE		= 0xf0; // Page number register
constexpr uint8_t	GSLx680_REG_ID			= 0xfc; // Chip ID

constexpr uint32_t	GSLx680_STATUS_OK	= 0x5A5A5A5A;
constexpr int		GSLx680_TS_DATA_LEN = 44;
constexpr uint8_t	GSLx680_CLOCK		= 0x04;

constexpr uint8_t	GSLx680_CMD_RESET	= 0x88;
constexpr uint8_t	GSLx680_CMD_START	= 0x00;


struct GSLx680_FirmwareNode
{
    uint8_t offset;
    uint32_t val;
};
extern const GSLx680_FirmwareNode* g_GSLx680_FW;
extern const int g_GSLx680_FW_Count;


struct GSLx680_PointData
{
	uint16_t y;
	uint16_t x_id;
};

constexpr uint16_t GSLx680_X_Msk = 0xfff;
constexpr uint16_t GSLx680_Y_Msk = 0xfff;
constexpr uint16_t GSLx680_ID_Pos = 12;
constexpr uint16_t GSLx680_ID_Msk = 0xf000;

struct GSLx680_TouchData
{
	uint8_t				TouchCount;
	uint8_t				padding[3];
	GSLx680_PointData	Points[10];
};
	

class GSLx680File : public KFileNode
{
    public:
    port_id m_TargetPort = -1;
};


class GSLx680Driver : public PtrTarget, public os::Thread, public KFilesystemFileOps
{
public:
	GSLx680Driver();
    ~GSLx680Driver();

    void Setup(const char* devicePath, int threadPriority, DigitalPinID pinShutdown, DigitalPinID pinIRQ, const char* i2cPath);

    virtual int Run() override;

    virtual Ptr<KFileNode>	OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> inode, int flags) override;
    virtual status_t		CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;
    virtual int				DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
	bool WriteData(int address, const void* data, size_t length);
	bool WriteRegister8(int address, uint8_t value);
	bool WriteRegister32(int address, uint32_t value);
	bool ReadData(int address, void* data, size_t length);
	
	bool InitializeChip();
	void TestI2C();
	bool StartupChip();
	bool ResetChip();
	bool ClearRegisters();
	bool LoadFirmware();
	bool VerifyFirmware();
	bool CheckStatus();

    static IRQResult IRQHandler(IRQn_Type irq, void* userData) { return static_cast<GSLx680Driver*>(userData)->HandleIRQ(); }
	IRQResult HandleIRQ();

    DigitalPin   m_PinShutdown;
    DigitalPin   m_PinIRQ;
    
    int			m_I2CDevice = -1;
	uint32_t	m_ChipID = 0;

    std::vector<GSLx680File*> m_OpenFiles;

    KMutex				m_Mutex;
    KConditionVariable	m_EventCondition;
    
    static const int MAX_POINTS = 10;
    IPoint		m_TouchPositions[MAX_POINTS];
	uint32_t	m_PointFlags = 0;	// Bitmask telling which point ID's are currently pressed.
    
	GSLx680Driver(const GSLx680Driver&) = delete;
	GSLx680Driver& operator=(const GSLx680Driver&) = delete;
};


} // namespace
