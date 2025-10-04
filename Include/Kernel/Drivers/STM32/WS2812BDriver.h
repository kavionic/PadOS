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
#include "Kernel/IRQDispatcher.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/VFS/KINode.h"
#include "Kernel/VFS/KFilesystem.h"
#include "Kernel/HAL/STM32/DMARequestID.h"


namespace kernel
{
enum class SPIID : int;

class WS2812BDriverINode : public KINode
{
public:
    IFLASHC WS2812BDriverINode(SPIID portID, bool swapIOPins, uint32_t clockFrequency, KFilesystemFileOps* fileOps);

    IFLASHC int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);
    IFLASHC PErrorCode Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position, ssize_t& outLength);
    IFLASHC PErrorCode Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position, ssize_t& outLength);

private:
    enum class State
    {
        Idle,
        Sending
    };

    static IFLASHC constexpr size_t RESET_BYTE_COUNT = 900; // 280e-6/(1/(200e6/64)) = 875
    static IFLASHC IRQResult IRQCallbackSend(IRQn_Type irq, void* userData);
    IFLASHC IRQResult HandleIRQ();

    IFLASHC PErrorCode WaitForIdle();

    IFLASHC PErrorCode SetLEDCount(size_t count);
    IFLASHC size_t GetLEDCount() const;

    IFLASHC void SetExponential(bool exponential);
    IFLASHC bool GetExponential() const;

    KMutex              m_Mutex;
    KConditionVariable  m_TransmitCondition;

    volatile State m_State = State::Idle;

    SPI_TypeDef*    m_Port;
    DMAMUX_REQUEST  m_DMARequestTX;

    int     m_SendDMAChannel = -1;
    int32_t     m_ReceiveBufferSize = 1024;
    int32_t     m_ReceiveBufferOutPos = 0;
    int32_t     m_ReceiveBufferInPos = 0;
    size_t      m_LEDCount = 0;
    size_t      m_TransmitBufferSize = 0;
    uint8_t*    m_TransmitBuffer = nullptr;
    bool        m_Exponential = true;
};


class WS2812BDriver : public PtrTarget, public KFilesystemFileOps
{
public:
    IFLASHC WS2812BDriver();
    IFLASHC ~WS2812BDriver();

    IFLASHC void Setup(const char* devicePath, bool swapIOPins, SPIID portID, uint32_t clockFrequency);

    IFLASHC virtual PErrorCode Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position, ssize_t& outLength) override;
    IFLASHC virtual PErrorCode Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position, ssize_t& outLength) override;
    IFLASHC virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    WS2812BDriver(const WS2812BDriver &other) = delete;
    WS2812BDriver& operator=(const WS2812BDriver &other) = delete;
};

} // namespace
