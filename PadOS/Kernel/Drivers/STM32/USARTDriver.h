/*
 * USARTDriver.h
 *
 *  Created on: Jan 3, 2020
 *      Author: kurts
 */

#pragma once

#include "Platform.h"
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
