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
// Created: 01.06.2020 16:30:05

#pragma once

#include "System/Platform.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/VFS/KINode.h"
#include "Kernel/VFS/KFilesystem.h"
#include "Kernel/HAL/STM32/DMARequestID.h"


namespace kernel
{

class WS2812BDriverINode : public KINode
{
public:
	WS2812BDriverINode(SPI_TypeDef* port, bool swapIOPins, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency, KFilesystemFileOps* fileOps);

	int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);
	ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length);
	ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length);

private:
	enum class State
	{
		Idle,
		Sending
	};

	static constexpr size_t RESET_BYTE_COUNT = 20;
	static IRQResult IRQCallbackSend(IRQn_Type irq, void* userData) { return static_cast<WS2812BDriverINode*>(userData)->HandleIRQ(); }
	IRQResult HandleIRQ();

	bool WaitForIdle();

	bool SetLEDCount(size_t count);
	size_t GetLEDCount() const { return m_LEDCount; }
	KMutex				m_Mutex;
	KConditionVariable	m_TransmitCondition;

	volatile State m_State = State::Idle;

	SPI_TypeDef*	m_Port;
	DMAMUX1_REQUEST	m_DMARequestTX;

	int			m_SendDMAChannel = -1;
	int32_t		m_ReceiveBufferSize = 1024;
	int32_t		m_ReceiveBufferOutPos = 0;
	int32_t		m_ReceiveBufferInPos = 0;
	size_t		m_LEDCount = 0;
	size_t		m_TransmitBufferSize = 0;
	uint8_t*	m_TransmitBuffer = nullptr;

};


class WS2812BDriver : public PtrTarget, public KFilesystemFileOps
{
public:
	WS2812BDriver();
	~WS2812BDriver();

    void Setup(const char* devicePath, bool swapIOPins, SPI_TypeDef* port, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency);

    virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
	WS2812BDriver(const WS2812BDriver &other) = delete;
	WS2812BDriver& operator=(const WS2812BDriver &other) = delete;
};

} // namespace
