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

namespace kernel
{

class USARTDriverINode : public KINode
{
public:
	USARTDriverINode(USART_TypeDef* port, uint32_t clockFrequency, KFilesystemFileOps* fileOps);

    ssize_t Read(Ptr<KFileNode> file, void* buffer, size_t length);
	ssize_t Write(Ptr<KFileNode> file, const void* buffer, size_t length);

private:

	static void IRQCallbackReceive(IRQn_Type irq, void* userData) { static_cast<USARTDriverINode*>(userData)->HandleIRQReceive(); }
    void HandleIRQReceive();
	static void IRQCallbackSend(IRQn_Type irq, void* userData) { static_cast<USARTDriverINode*>(userData)->HandleIRQSend(); }
    void HandleIRQSend();

	KMutex m_MutexRead;
	KMutex m_MutexWrite;

	KConditionVariable m_ReceiveCondition;
	KConditionVariable m_TransmitCondition;


	USART_TypeDef* m_Port;
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

    void Setup(const char* devicePath, USART_TypeDef* port, uint32_t clockFrequency);

    virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
	USARTDriver(const USARTDriver &other) = delete;
	USARTDriver& operator=(const USARTDriver &other) = delete;
};

} // namespace
