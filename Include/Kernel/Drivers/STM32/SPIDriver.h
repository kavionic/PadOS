// This file is part of PadOS.
//
// Copyright (C) 2023 Kurt Skauen <http://kavionic.com/>
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
// Created: 22.06.2023 19:00

#pragma once

#include <System/Platform.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/VFS/KINode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/HAL/STM32/DMARequestID.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/HAL/DMA.h>
#include <DeviceControl/SPI.h>

namespace kernel
{

class SPIDriver;
enum class SPIID : int;

struct SPIDriverSetup
{
    os::String                  DevicePath;
    SPIID                       PortID;
    PinMuxTarget                PinCLK;
    PinMuxTarget                PinMOSI;
    PinMuxTarget                PinMISO;
    DigitalPinDriveStrength_e   PinDriveStrength    = DigitalPinDriveStrength_e::Low;
    SPIBaudRateDivider          BaudRateDivider     = SPIBaudRateDivider::DIV2;
    SPIRole                     Role                = SPIRole::Master;
    SPIComMode                  ComMode             = SPIComMode::FullDuplex;
    SPIProtocol                 Protocol            = SPIProtocol::Motorola;
    SPIEndian                   Endian              = SPIEndian::MSB;
    SPICLkPolarity              ClockPolarity       = SPICLkPolarity::Low;
    SPICLkPhase                 ClockPhase          = SPICLkPhase::FirstEdge;
    SPISlaveUnderrunMode        SlaveUnderrunMode   = SPISlaveUnderrunMode::ConstantPattern;
    int32_t                     FIFOThreshold       = 1;
    int32_t                     CRCSize             = 0;
    int32_t                     CRCPolynomial       = 7;
    int32_t                     WordSize            = 8;
    uint32_t                    InterWordIdleCycles = 0;
    size_t                      ReceiveBufferSize   = 0;
};

class SPIDriverINode : public KINode
{
public:
    IFLASHC SPIDriverINode(const SPIDriverSetup& setup, SPIDriver* driver);

    IFLASHC size_t Read(Ptr<KFileNode> file, void* buffer, size_t length);
    IFLASHC size_t Write(Ptr<KFileNode> file, const void* buffer, size_t length);
    IFLASHC void   DeviceControl(int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

private:
    IFLASHC void    SetBaudrateDivider(SPIBaudRateDivider baudRateDivider);

    IFLASHC bool    SetPinMode(const PinMuxTarget& pin, SPIPinMode mode);

    IFLASHC void    SetSwapMOSIMISO(bool doSwap);
    IFLASHC bool    GetSwapMOSIMISO() const;

    IFLASHC void    StartTransaction(const SPITransaction& transaction);

    static IFLASHC IRQResult    SPIIRQCallback(IRQn_Type irq, void* userData);
    IFLASHC IRQResult           HandleSPIIRQ();

    Ptr<SPIDriver>      m_Driver;

    KMutex              m_Mutex;
    KConditionVariable  m_TransactionCondition;

    SPI_TypeDef*    m_Port;
    
    PinMuxTarget    m_PinCLK;
    PinMuxTarget    m_PinMOSI;
    PinMuxTarget    m_PinMISO;

    DMAMUX_REQUEST  m_DMARequestRX;
    DMAMUX_REQUEST  m_DMARequestTX;

    int             m_TransferCountBitShift = 0;

    SPIPinMode      m_PinModeCLK = SPIPinMode::Normal;
    SPIPinMode      m_PinModeMOSI = SPIPinMode::Normal;
    SPIPinMode      m_PinModeMISO = SPIPinMode::Normal;
    TimeValNanos    m_ReadTimeout = TimeValNanos::infinit;
    DMAChannel      m_ReceiveDMAChannel;
    DMAChannel      m_SendDMAChannel;
    int32_t         m_ReceiveBufferSize = 0;
    uint8_t*        m_ReceiveBuffer = nullptr;
    volatile SPIError   m_TransactionError = SPIError::None;
};

class SPIDriver : public PtrTarget, public KFilesystemFileOps
{
public:
    IFLASHC SPIDriver();

    IFLASHC void Setup(const SPIDriverSetup& setup);

    IFLASHC virtual size_t Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    IFLASHC virtual size_t Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    IFLASHC virtual void   DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    SPIDriver(const SPIDriver&other) = delete;
    SPIDriver& operator=(const SPIDriver&other) = delete;
};

} // namespace
