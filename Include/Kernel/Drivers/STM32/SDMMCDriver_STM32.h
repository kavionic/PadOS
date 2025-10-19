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
// Created: 20.05.2020 23:00:00



#pragma once

#include "Kernel/Drivers/SDMMCDriver/SDMMCDriver.h"

namespace kernel
{

enum class WakeupReason : int
{
    None,
    DataComplete,
    Error,
    Event
};

class SDMMCDriver_STM32 : public SDMMCDriver
{
public:
    SDMMCDriver_STM32();
    ~SDMMCDriver_STM32();

    void Setup(const os::String& devicePath, SDMMC_TypeDef* port, uint32_t peripheralClockFrequency, uint32_t clockCap, DigitalPinID pinCD, IRQn_Type irqNum);


	virtual void     Reset() override;
    virtual void     SetClockFrequency(uint32_t frequency) override;
    virtual void     SendClock() override;

    bool ExecuteCmd(uint32_t extraCmdRFlags, uint32_t cmd, uint32_t arg);

    virtual bool	SendCmd(uint32_t cmd, uint32_t arg) override;
    virtual uint32_t	GetResponse() override;
    virtual void	GetResponse128(uint8_t* response) override;
    virtual bool	StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint32_t blockCount, const iovec_t* segments, size_t segmentCount) override;
    virtual bool	StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg) override;
    virtual void	ApplySpeedAndBusWidth() override;

private:
    static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IRQResult        HandleIRQ();

    bool     WaitIRQ(uint32_t flags);

    SDMMC_TypeDef*  m_SDMMC;
    uint32_t	    m_PeripheralClockFrequency = 0;
    uint32_t	    m_ClockCap = 0;

    const iovec_t*          m_TransferSegments = nullptr;
    size_t                  m_SegmentCount = 0;
    volatile size_t         m_CurrentSegment = 0;
    volatile WakeupReason   m_WakeupReason = WakeupReason::None;
};

} // namespace
