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
// Created: 03.01.2020 12:00:00

#pragma once

#include "System/Platform.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/VFS/KINode.h"
#include "Kernel/VFS/KFilesystem.h"
#include "Kernel/HAL/STM32/DMARequestID.h"

namespace kernel
{

class USARTDriverINode : public KINode
{
public:
	USARTDriverINode(USART_TypeDef* port, DMAMUX1_REQUEST dmaRequestRX, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency, KFilesystemFileOps* fileOps);

    ssize_t Read(Ptr<KFileNode> file, void* buffer, size_t length);
	ssize_t Write(Ptr<KFileNode> file, const void* buffer, size_t length);

private:

	static IRQResult IRQCallbackReceive(IRQn_Type irq, void* userData) { return static_cast<USARTDriverINode*>(userData)->HandleIRQReceive(); }
	IRQResult HandleIRQReceive();
	static IRQResult IRQCallbackSend(IRQn_Type irq, void* userData) { return static_cast<USARTDriverINode*>(userData)->HandleIRQSend(); }
	IRQResult HandleIRQSend();

	KMutex m_MutexRead;
	KMutex m_MutexWrite;

	KConditionVariable m_ReceiveCondition;
	KConditionVariable m_TransmitCondition;


	USART_TypeDef* m_Port;
	DMAMUX1_REQUEST	m_DMARequestRX;
	DMAMUX1_REQUEST	m_DMARequestTX;

	int				m_Baudrate = 0;
	int            m_ReceiveDMAChannel = -1;
	int            m_SendDMAChannel = -1;
	int32_t        m_ReceiveBufferSize = 1024;
	int32_t        m_ReceiveBufferOutPos = 0;
	int32_t        m_ReceiveBufferInPos = 0;
	uint8_t*       m_ReceiveBuffer;
};


class USARTDriver : public PtrTarget, public KFilesystemFileOps
{
public:
	USARTDriver();
	~USARTDriver();

    void Setup(const char* devicePath, USART_TypeDef* port, DMAMUX1_REQUEST dmaRequestRX, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency);

    virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
	USARTDriver(const USARTDriver &other) = delete;
	USARTDriver& operator=(const USARTDriver &other) = delete;
};

} // namespace
