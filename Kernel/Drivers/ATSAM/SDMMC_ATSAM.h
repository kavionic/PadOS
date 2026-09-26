// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.05.2020 23:00:00



#pragma once

namespace kernel
{

class SDMMC_ATSAM
{
public:
	SDMMC_ATSAM();
	~SDMMC_ATSAM();

	bool Setup(const os::String& devicePath, const DigitalPin& pinCD, IRQn_Type irqNum);

	virtual void     SetClockFrequency(uint32_t frequency) override;
	virtual void     SendClock() override;

	bool ExecuteCmd(uint32_t extraCmdRFlags, uint32_t cmd, uint32_t arg);

	virtual bool		SendCmd(uint32_t cmd, uint32_t arg) override;
	virtual uint32_t	GetResponse() override;
	virtual void		GetResponse128(uint8_t* response) override;
	virtual bool		StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint32_t blockCount, const void* buffer) override;
	virtual bool		StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg) override;
	virtual void		ApplySpeedAndBusWidth() override;

private:
	static IRQResult IRQCallback(IRQn_Type irq, void* userData) { return static_cast<SDMMC_ATSAM*>(userData)->HandleIRQ(); }
	IRQResult        HandleIRQ();

	bool     ReadNoDMA(void* buffer, uint16_t blockSize, size_t blockCount);
	bool     ReadDMA(void* buffer, uint16_t blockSize, size_t blockCount);
	bool     WriteDMA(const void *buffer, uint16_t blockSize, size_t blockCount);

	bool     WaitIRQ(uint32_t flags);
	bool     WaitIRQ(uint32_t flags, bigtime_t timeout);

	void     Reset();

	void*               m_CurrentBuffer = nullptr; // If non-null this is the buffer for non-DMA transfere;
	size_t              m_BytesToRead   = 0;

};

} // namespace kernel
