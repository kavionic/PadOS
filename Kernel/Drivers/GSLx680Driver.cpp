// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <string.h>
#include <fcntl.h>

#include <Kernel/HAL/Peripherals.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/Drivers/GSLx680Driver.h>
#include <Kernel/KTime.h>
#include <Kernel/KMessagePort.h>
#include <DeviceControl/I2C.h>
#include <DeviceControl/HID.h>
#include <System/SystemMessageIDs.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>
#include <GUI/GUIEvent.h>

using namespace os;

namespace kernel
{


PREGISTER_KERNEL_DRIVER(GSLx680Driver, GSLx680DriverParameters);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GSLx680Driver::GSLx680Driver(const GSLx680DriverParameters& parameters)
    : KINode(nullptr, nullptr, this, false)
    , KThread("GSLx680_driver")
    , m_Mutex("GSLx680_mutex", PEMutexRecursionMode_RaiseError)
    , m_EventCondition("GSLx680_events")
{
    m_PinShutdown   = parameters.PinShutdown;
    m_PinIRQ        = parameters.PinIRQ;

    m_PinShutdown.Write(false);
    m_PinShutdown.SetDirection(DigitalPinDirection_e::Out);

    m_PinIRQ.SetDirection(DigitalPinDirection_e::In);
    m_PinIRQ.SetPullMode(PinPullMode_e::Down);


    m_PinIRQ.SetInterruptMode(PinInterruptMode_e::RisingEdge);
    m_PinIRQ.GetAndClearInterruptStatus(); // Clear any pending interrupts.
    m_PinIRQ.EnableInterrupts();

    register_irq_handler(get_peripheral_irq(parameters.PinIRQ), IRQHandler, this);

    m_I2CDevice = kopen_trw(parameters.I2CPath.c_str(), O_RDWR);

    I2CIOCTL_SetTimeout(m_I2CDevice, TimeValNanos::FromMilliseconds(1000));
    I2CIOCTL_SetSlaveAddress(m_I2CDevice, 0x80);
    I2CIOCTL_SetInternalAddrLen(m_I2CDevice, 1);

    SetDeleteOnExit(false);
    Start_trw(PThreadDetachState_Detached, parameters.ThreadPriority);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GSLx680Driver::~GSLx680Driver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* GSLx680Driver::Run()
{
	CRITICAL_SCOPE(m_Mutex);
	
	InitializeChip();

	int failCount = 0;
    for(;;)
    {
		m_EventCondition.Wait(m_Mutex);

		GSLx680_TouchData touchData;

		if (!ReadData(GSLx680_REG_TOUCH_COUNT, &touchData, sizeof(touchData)))
		{
			failCount++;
            p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "GSLx680Driver: Failed to read touch data({}): {}", failCount, strerror(errno));
			if (failCount == 5) {
                p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "GSLx680Driver: Re-initializing chip.");
				InitializeChip();
				failCount = 0;
				continue;
			}
			snooze_ms(250);
			continue;;
		}
		failCount = 0;
		if (touchData.TouchCount > ARRAY_COUNT(touchData.Points)) touchData.TouchCount = ARRAY_COUNT(touchData.Points);

		uint32_t pointFlags = 0;
		uint32_t moveFlags = 0;
		for (int i = 0; i < touchData.TouchCount; ++i)
		{
			int id = (touchData.Points[i].x_id & GSLx680_ID_Msk) >> GSLx680_ID_Pos;
			int index = id - 1;
			
			if (index < 0 || index >= MAX_POINTS) continue;

			IPoint position(touchData.Points[i].x_id & GSLx680_X_Msk, touchData.Points[i].y & GSLx680_X_Msk);
			std::swap(position.x, position.y);
			if (position != m_TouchPositions[index]) {
				m_TouchPositions[index] = position;
				moveFlags |= 1 << index;
			}
			pointFlags |= 1 << index;
		}
		uint32_t toggledPoints = pointFlags ^ m_PointFlags;

		MessageID eventID = MessageID::NONE;

		for (int i = 0; i < MAX_POINTS; ++i)
		{
			uint32_t mask = 1 << i;
			if (toggledPoints & mask)
			{
				if (pointFlags & mask)
				{
					eventID = MessageID::MOUSE_DOWN;
				}
				else
				{
					eventID = MessageID::MOUSE_UP;
				}
			}
			else if (pointFlags & mask)
			{
				eventID = MessageID::MOUSE_MOVE;
			}
			else
			{
				continue;
			}

			if (eventID != MessageID::MOUSE_MOVE || (moveFlags & mask))
			{
                MotionEvent mouseEvent;
				mouseEvent.Timestamp    = kget_monotonic_time();
				mouseEvent.EventID      = eventID;
                mouseEvent.ToolType     = MotionToolType::Finger;
				mouseEvent.ButtonID     = MouseButton_e(int(MouseButton_e::FirstTouchID) + i);
				mouseEvent.Position     = Point(m_TouchPositions[i]);

				// p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_HIGH_VOL, "Mouse event {}: {}/{}", eventID - MessageID::MOUSE_DOWN, m_TouchPositions[i].x, m_TouchPositions[i].y);
				for (auto file : m_OpenFiles)
				{
					if (file->m_TargetPort != -1) {
                        try {
						    kmessage_port_send_trw(file->m_TargetPort, -1, int32_t(eventID), &mouseEvent, sizeof(mouseEvent));
                        } catch (const std::exception& exc) {
                            p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "GSLx680Driver: failed to send event: {}", exc.what());
                        }
                    }
				}
			}

		}
		m_PointFlags = pointFlags;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> GSLx680Driver::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> inode, int flags)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<GSLx680File> file = ptr_new<GSLx680File>(flags);
    m_OpenFiles.push_back(ptr_raw_pointer_cast(file));
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GSLx680Driver::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    CRITICAL_SCOPE(m_Mutex);
    auto i = std::find(m_OpenFiles.begin(), m_OpenFiles.end(), file);
    if (i != m_OpenFiles.end())
    {
        m_OpenFiles.erase(i);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GSLx680Driver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<GSLx680File> ftFile = ptr_static_cast<GSLx680File>(file);
    
    port_id* inArg  = (int*)inData;
    port_id* outArg = (int*)outData;
    
    switch(request)
    {
        case HIDIOCTL_SET_TARGET_PORT:
            if (inArg == nullptr || inDataLength != sizeof(port_id)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            ftFile->m_TargetPort = *inArg;
            return;
        case HIDIOCTL_GET_TARGET_PORT:
            if (outArg == nullptr || outDataLength != sizeof(port_id)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            *outArg = ftFile->m_TargetPort;
            return;
        default:
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::WriteData(int address, const void* data, size_t length)
{
	PErrorCode result = PErrorCode::IOError;
	for (int i = 0; i < 10 && result != PErrorCode::Success; ++i)
    {
		result = kpwrite(m_I2CDevice, data, length, address);
		if (result != PErrorCode::Success) {
            p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "Error {} writing reg {:02x}:{}: {}", i, address, length, strerror(std::to_underlying(result)));
		}
	}
	return result == PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::WriteRegister8(int address, uint8_t value)
{
	return WriteData(address, &value, sizeof(value));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::WriteRegister32(int address, uint32_t value)
{
	return WriteData(address, &value, sizeof(value));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::ReadData(int address, void* data, size_t length)
{
	PErrorCode result = PErrorCode::IOError;
	for (int i = 0; i < 10 && result != PErrorCode::Success; ++i) {
		result = kpread(m_I2CDevice, data, length, address);
		if (result != PErrorCode::Success && i > 0) {
            p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "Error {} reading reg {:02x}:{}: {}", i, address, length, strerror(std::to_underlying(result)));
		}
	}
	return result == PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::InitializeChip()
{
	m_PinShutdown.Write(true);
	snooze_ms(5);
	m_PinShutdown.Write(false);
	snooze_ms(50);
	m_PinShutdown.Write(true);
	snooze_ms(50);

	uint32_t chipID;
	if (ReadData(GSLx680_REG_ID, &chipID, sizeof(chipID)))
	{
		m_ChipID = chipID;
        p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "GSLx680Driver: Chip ID 0x{:8x}", m_ChipID);
	}
	else
	{
        p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "GSLx680Driver::InitializeChip() failed to read chip ID: {}", strerror(errno));
		m_ChipID = 0;
	}

	if (!ClearRegisters())	return false;
	if (!ResetChip())		return false;
	if (!LoadFirmware())	return false;
	if (!ResetChip()) return false;
	if (!StartupChip()) return false;

	VerifyFirmware();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GSLx680Driver::TestI2C()
{
	uint32_t read_buf = 0xaaaaaaaa;
	uint32_t write_buf = 0x00000012;

	ReadData(GSLx680_REG_PAGE, &read_buf, sizeof(read_buf));
    p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "gslX680 TestI2C read 0xf0: {:08x}", read_buf);
	WriteRegister32(GSLx680_REG_PAGE, write_buf);
    p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "gslX680 TestI2C write 0xf0: {:08x}", write_buf);
	ReadData(GSLx680_REG_PAGE, &read_buf, sizeof(read_buf));
    p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "gslX680 TestI2C read 0xf0: {:08x}", read_buf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::StartupChip()
{
	bool result = WriteRegister8(GSLx680_REG_RESET, GSLx680_CMD_START);
	snooze_ms(10);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::ResetChip()
{
	if (!WriteRegister8(GSLx680_REG_RESET, GSLx680_CMD_RESET)) {
		return false;
	}
	snooze_ms(10);
	if (!WriteRegister8(GSLx680_REG_CLOCK, GSLx680_CLOCK)) {
		return false;
	}
	snooze_ms(10);
	if (!WriteRegister8(GSLx680_REG_POWER, GSLx680_CMD_START)) {
		return false;
	}
	snooze_ms(10);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::ClearRegisters()
{
	if (!WriteRegister8(GSLx680_REG_RESET, GSLx680_CMD_RESET)) {
		return false;
	}
	snooze_ms(20);
	if (!WriteRegister8(GSLx680_REG_TOUCH_COUNT, 1)) {
		return false;
	}
	snooze_ms(5);
	if (!WriteRegister8(GSLx680_REG_CLOCK, GSLx680_CLOCK)) {
		return false;
	}
	snooze_ms(5);
	if (!WriteRegister8(GSLx680_REG_RESET, GSLx680_CMD_START)) {
		return false;
	}
	snooze_ms(20);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::LoadFirmware()
{
    p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "============= GSLx680Driver::LoadFirmware(): start ==============");

	for (int source_line = 0; source_line < g_GSLx680_FW_Count; ++source_line)
	{
		/* init page trans, set the page val */
		if (g_GSLx680_FW[source_line].offset == GSLx680_REG_PAGE)
		{
			WriteRegister8(GSLx680_REG_PAGE, uint8_t(g_GSLx680_FW[source_line].val & 0x000000ff));
		}
		else
		{
			if (!WriteRegister32(g_GSLx680_FW[source_line].offset, g_GSLx680_FW[source_line].val)) {
                p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "Failed to write FW: {}", strerror(errno));
			}
		}
	}
    p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "============= GSLx680Driver::LoadFirmware(): end ==============");
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::VerifyFirmware()
{
    p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "============= GSLx680Driver::VerifyFirmware(): start ==============");

	uint8_t currentPage = 0xff;
	bool succeeded = true;

	for (int source_line = 0; source_line < g_GSLx680_FW_Count; source_line++)
	{
		if (g_GSLx680_FW[source_line].offset == GSLx680_REG_PAGE)
		{
			currentPage = uint8_t(g_GSLx680_FW[source_line].val & 0x000000ff);
			WriteRegister8(GSLx680_REG_PAGE, currentPage);
		}
		else
		{
			uint32_t value;

			if (ReadData(g_GSLx680_FW[source_line].offset, &value, sizeof(value)))
			{
				if (value != g_GSLx680_FW[source_line].val) {
					succeeded = false;
                    p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "FW mismatch at {:02x}:{:02x} ({:08x} != {:08x})", currentPage, g_GSLx680_FW[source_line].offset, value, g_GSLx680_FW[source_line].val);
				}
			}
			else
			{
                p_system_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "Failed to read back FW {:02x}:{:02x}: {}", currentPage, g_GSLx680_FW[source_line].offset, strerror(errno));
				succeeded = false;
				continue;
			}
		}
	}
    p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "============= GSLx680Driver::VerifyFirmware(): end ==============");
	return succeeded;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool GSLx680Driver::CheckStatus()
{
	snooze_ms(30);
	uint32_t status;
	ReadData(GSLx680_REG_STATUS, &status, sizeof(status));
	if (status != GSLx680_STATUS_OK)
	{
        p_system_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "#########check mem read 0xb0 = {:08x} #########", status);
		m_PinShutdown = false;
		snooze_ms(20);
		m_PinShutdown = true;
		snooze_ms(20);
		TestI2C();
		ClearRegisters();
		ResetChip();
		LoadFirmware();
		StartupChip();
		ResetChip();
		StartupChip();
		ReadData(GSLx680_REG_STATUS, &status, sizeof(status));
		return status == GSLx680_STATUS_OK;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult GSLx680Driver::HandleIRQ()
{
	if (m_PinIRQ.GetAndClearInterruptStatus())
	{
		m_EventCondition.Wakeup(1);
		return IRQResult::HANDLED;
	}
	return IRQResult::UNHANDLED;
}


} // namespace kernel
