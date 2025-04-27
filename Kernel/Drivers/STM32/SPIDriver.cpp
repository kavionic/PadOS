// This file is part of PadOS.
//
// Copyright (C) 2023-2024 Kurt Skauen <http://kavionic.com/>
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


#include <malloc.h>
#include <algorithm>
#include <cstring>

#include <Kernel/Drivers/STM32/SPIDriver.h>

#include <Ptr/Ptr.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/HAL/PeripheralMapping.h>

namespace kernel
{

static constexpr int SPI_MAX_WORD_LENGTH(SPIID spiID) { return (spiID <= SPIID::SPI_3) ? 32 : 16; }

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SPIDriverINode::SPIDriverINode(const SPIDriverSetup& setup, SPIDriver* driver)
    : KINode(nullptr, nullptr, driver, false)
    , m_Mutex("SPIDriverINodeMutex", EMutexRecursionMode::RaiseError)
    , m_TransactionCondition("SPIDriverINodeTransC")
    , m_PinCLK(setup.PinCLK)
    , m_PinMOSI(setup.PinMOSI)
    , m_PinMISO(setup.PinMISO)
{
    m_Driver = ptr_tmp_cast(driver);
    m_Port   = get_spi_from_id(setup.PortID);

    get_spi_dma_requests(setup.PortID, m_DMARequestRX, m_DMARequestTX);

    DigitalPin::ActivatePeripheralMux(m_PinCLK);
    DigitalPin::ActivatePeripheralMux(m_PinMOSI);
    DigitalPin::ActivatePeripheralMux(m_PinMISO);

    DigitalPin(m_PinCLK.PINID).SetDriveStrength(setup.PinDriveStrength);
    DigitalPin(m_PinMOSI.PINID).SetDriveStrength(setup.PinDriveStrength);
    DigitalPin(m_PinMISO.PINID).SetDriveStrength(setup.PinDriveStrength);

    m_Port->CR1 = SPI_CR1_SSI;

    uint32_t CFG1 = 0;
    set_bit_group(CFG1, SPI_CFG1_MBR_Msk, uint32_t(setup.BaudRateDivider) << SPI_CFG1_MBR_Pos);
    set_bit_group(CFG1, SPI_CFG1_DSIZE_Msk, (setup.WordSize - 1) << SPI_CFG1_DSIZE_Pos);
    set_bit_group(CFG1, SPI_CFG1_FTHLV_Msk, (setup.FIFOThreshold - 1) << SPI_CFG1_FTHLV_Pos);

    switch (setup.SlaveUnderrunMode)
    {
        case SPISlaveUnderrunMode::ConstantPattern:         set_bit_group(CFG1, SPI_CFG1_UDRCFG_Msk, 0 << SPI_CFG1_UDRCFG_Pos); break;
        case SPISlaveUnderrunMode::RepeatLastReceived:      set_bit_group(CFG1, SPI_CFG1_UDRCFG_Msk, 1 << SPI_CFG1_UDRCFG_Pos); break;
        case SPISlaveUnderrunMode::RepeatLastTransmitted:   set_bit_group(CFG1, SPI_CFG1_UDRCFG_Msk, 2 << SPI_CFG1_UDRCFG_Pos); break;
    }
    m_Port->CFG1 = CFG1;

    if (setup.CRCSize != 0)
    {
        const bool isAtMaxWordSize = (SPI_MAX_WORD_LENGTH(setup.PortID) == 32 && setup.CRCSize == 32) || (SPI_MAX_WORD_LENGTH(setup.PortID) == 16 && setup.CRCSize == 16);

        m_Port->CFG1 |= SPI_CFG1_CRCEN;
        set_bit_group(m_Port->CFG1, SPI_CFG1_CRCSIZE_Msk, (setup.CRCSize - 1) << SPI_CFG1_CRCSIZE_Pos);

        if (isAtMaxWordSize)
        {
            m_Port->CRCPOLY = setup.CRCPolynomial;
            m_Port->CR1 |= SPI_CR1_CRC33_17;
        }
        else if (setup.CRCSize != 0)
        {
            m_Port->CR1 &= ~SPI_CR1_CRC33_17;
            m_Port->CRCPOLY = setup.CRCPolynomial | (1 << setup.CRCSize);
        }
    }
    uint32_t CFG2 = SPI_CFG2_AFCNTR | SPI_CFG2_SSM;

    set_bit_group(CFG2, SPI_CFG2_MIDI_Msk, setup.InterWordIdleCycles << SPI_CFG2_MIDI_Pos);

    switch (setup.ComMode)
    {
        case SPIComMode::FullDuplex:            set_bit_group(CFG2, SPI_CFG2_COMM_Msk, 0 << SPI_CFG2_COMM_Pos); break;
        case SPIComMode::SimplexTransmitter:    set_bit_group(CFG2, SPI_CFG2_COMM_Msk, 1 << SPI_CFG2_COMM_Pos); break;
        case SPIComMode::SimplexReceiver:       set_bit_group(CFG2, SPI_CFG2_COMM_Msk, 2 << SPI_CFG2_COMM_Pos); break;
        case SPIComMode::HalfDuplex:            set_bit_group(CFG2, SPI_CFG2_COMM_Msk, 3 << SPI_CFG2_COMM_Pos); break;
    }
    switch (setup.Protocol)
    {
        case SPIProtocol::Motorola: set_bit_group(CFG2, SPI_CFG2_SP_Msk, 0 << SPI_CFG2_SP_Pos); break;
        case SPIProtocol::TI:       set_bit_group(CFG2, SPI_CFG2_SP_Msk, 1 << SPI_CFG2_SP_Pos); break;
    }
    if (setup.Role == SPIRole::Master)               CFG2 |= SPI_CFG2_MASTER;
    if (setup.Endian == SPIEndian::LSB)              CFG2 |= SPI_CFG2_LSBFRST;
    if (setup.ClockPhase == SPICLkPhase::SecondEdge) CFG2 |= SPI_CFG2_CPHA;
    if (setup.ClockPolarity == SPICLkPolarity::High) CFG2 |= SPI_CFG2_CPOL;

    m_Port->CFG2 = CFG2;

    m_ReceiveDMAChannel.AllocateChannel();
    m_SendDMAChannel.AllocateChannel();

    m_ReceiveDMAChannel.SetDirection(DMADirection::PeriphToMem);
    m_ReceiveDMAChannel.SetRequestID(m_DMARequestRX);
    m_ReceiveDMAChannel.SetRegisterAddress(&m_Port->RXDR);

    m_SendDMAChannel.SetDirection(DMADirection::MemToPeriph);
    m_SendDMAChannel.SetRequestID(m_DMARequestTX);
    m_SendDMAChannel.SetRegisterAddress(&m_Port->TXDR);

    DMAWordSize dmaWordSize;

    if (setup.WordSize > 16)
    {
        dmaWordSize = DMAWordSize::WS32;
        m_TransferCountBitShift = 2;
    }
    else if (setup.WordSize > 8)
    {
        dmaWordSize = DMAWordSize::WS16;
        m_TransferCountBitShift = 1;
    }
    else
    {
        dmaWordSize = DMAWordSize::WS8;
        m_TransferCountBitShift = 0;
    }
    m_ReceiveDMAChannel.SetMemoryWordSize(dmaWordSize);
    m_ReceiveDMAChannel.SetRegisterWordSize(dmaWordSize);
    m_SendDMAChannel.SetMemoryWordSize(dmaWordSize);
    m_SendDMAChannel.SetRegisterWordSize(dmaWordSize);

    m_ReceiveBufferSize = (setup.ReceiveBufferSize + DCACHE_LINE_SIZE - 1) & ~(DCACHE_LINE_SIZE - 1);
    if (m_ReceiveBufferSize > 0) {
        m_ReceiveBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, m_ReceiveBufferSize));
    }
    const IRQn_Type spiIRQ = get_spi_irq(setup.PortID);
    NVIC_ClearPendingIRQ(spiIRQ);
    kernel::register_irq_handler(spiIRQ, SPIIRQCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SPIDriverINode::Read(Ptr<KFileNode> file, void* buffer, const size_t length)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SPIDriverINode::Write(Ptr<KFileNode> file, const void* buffer, const size_t length)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int SPIDriverINode::DeviceControl(int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);

    switch (request)
    {
        case SPIIOCTL_SET_BAUDRATE_DIVIDER:
            if (inDataLength == sizeof(SPIBaudRateDivider)) {
                const SPIBaudRateDivider divider = *((const SPIBaudRateDivider*)inData);
                SetBaudrateDivider(divider);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_GET_BAUDRATE_DIVIDER:
            if (outDataLength == sizeof(SPIBaudRateDivider)) {
                SPIBaudRateDivider* divider = (SPIBaudRateDivider*)outData;
                *divider = SPIBaudRateDivider((m_Port->CFG1 & SPI_CFG1_MBR_Msk) >> SPI_CFG1_MBR_Pos);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_SET_READ_TIMEOUT:
            if (inDataLength == sizeof(bigtime_t)) {
                bigtime_t micros = *((const bigtime_t*)inData);
                m_ReadTimeout = TimeValMicros::FromMicroseconds(micros);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_GET_READ_TIMEOUT:
            if (outDataLength == sizeof(bigtime_t)) {
                bigtime_t* micros = (bigtime_t*)outData;
                *micros = m_ReadTimeout.AsMicroSeconds();
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_SET_PINMODE:
            if (inDataLength == sizeof(int))
            {
                int arg = *((const int*)inData);
                SPIPin     pin  = SPIPin(arg >> 16);
                SPIPinMode mode = SPIPinMode(arg & 0xffff);

                switch (pin)
                {
                    case SPIPin::CLK:
                        if (SetPinMode(m_PinCLK, mode)) {
                            m_PinModeCLK = mode;
                            return 0;
                        } else {
                            set_last_error(EINVAL);
                            return -1;
                        }
                    case SPIPin::MOSI:
                        if (SetPinMode(m_PinMOSI, mode)) {
                            m_PinModeMOSI = mode;
                            return 0;
                        } else {
                            set_last_error(EINVAL);
                            return -1;
                        }
                    case SPIPin::MISO:
                        if (SetPinMode(m_PinMISO, mode)) {
                            m_PinModeMISO = mode;
                            return 0;
                        } else {
                            set_last_error(EINVAL);
                            return -1;
                        }
                    default:
                        set_last_error(EINVAL);
                        return -1;
                }
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_GET_PINMODE:
            if (inDataLength == sizeof(int) && outDataLength == sizeof(int))
            {
                int arg = *((const int*)inData);
                SPIPin     pin = SPIPin(arg);

                int* result = (int*)outData;

                switch (pin)
                {
                    case SPIPin::CLK:
                        *result = int(m_PinModeCLK);
                        return 0;
                    case SPIPin::MOSI:
                        *result = int(m_PinModeMOSI);
                        return 0;
                    case SPIPin::MISO:
                        *result = int(m_PinModeMISO);
                        return 0;
                    default:
                        set_last_error(EINVAL);
                        return -1;
                }
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_SET_SWAP_MOSI_MISO:
            if (inDataLength == sizeof(int)) {
                int arg = *((const int*)inData);
                SetSwapMOSIMISO(arg != 0);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_GET_SWAP_MOSI_MISO:
            if (outDataLength == sizeof(int)) {
                int* result = (int*)outData;
                *result = GetSwapMOSIMISO();
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_START_TRANSACTION:
            if (inDataLength == sizeof(SPITransaction) && outDataLength == sizeof(ssize_t))
            {
                const ssize_t result = StartTransaction(*reinterpret_cast<const SPITransaction*>(inData));
                *reinterpret_cast<ssize_t*>(outData) = result;
                return (result < 0) ? -1 : 0;
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case SPIIOCTL_GET_LAST_ERROR:
            if (outDataLength == sizeof(SPIError))
            {
                SPIError* result = reinterpret_cast<SPIError*>(outData);
                *result = m_TransactionError;
                return 0;
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        default:
            set_last_error(EINVAL);
            return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SPIDriverINode::SetBaudrateDivider(SPIBaudRateDivider baudRateDivider)
{
    set_bit_group(m_Port->CFG1, SPI_CFG1_MBR_Msk, uint32_t(baudRateDivider) << SPI_CFG1_MBR_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SPIDriverINode::SetPinMode(const PinMuxTarget& pin, SPIPinMode mode)
{
    kassert(m_Mutex.IsLocked());

    if (mode == SPIPinMode::Normal)
    {
        DigitalPin::ActivatePeripheralMux(pin);
        return true;
    }
    else
    {
        DigitalPin ioPin(pin.PINID);
        switch (mode)
        {
            case SPIPinMode::Off:
                ioPin.SetPeripheralMux(DigitalPinPeripheralID::None);
                ioPin.SetDirection(DigitalPinDirection_e::Analog);
                return true;
            case SPIPinMode::Low:
                ioPin = false;
                ioPin.SetPeripheralMux(DigitalPinPeripheralID::None);
                ioPin.SetDirection(DigitalPinDirection_e::Out);
                return true;
            case SPIPinMode::High:
                ioPin = true;
                ioPin.SetPeripheralMux(DigitalPinPeripheralID::None);
                ioPin.SetDirection(DigitalPinDirection_e::Out);
                return true;
            default:
                set_last_error(EINVAL);
                return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SPIDriverINode::SetSwapMOSIMISO(bool doSwap)
{
    kassert(m_Mutex.IsLocked());

    if (doSwap) {
        m_Port->CFG2 |= SPI_CFG2_IOSWP;
    } else {
        m_Port->CFG2 &= ~SPI_CFG2_IOSWP;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC ssize_t SPIDriverINode::StartTransaction(const SPITransaction& transaction)
{
    kassert(m_Mutex.IsLocked());

    if (transaction.ReceiveBuffer != nullptr && transaction.Length > m_ReceiveBufferSize)
    {
        set_last_error(EINVAL);
        return -1;
    }

    SCB_CleanInvalidateDCache();

    uint32_t errorFlags = SPI_SR_UDR | SPI_SR_OVR | SPI_SR_MODF | SPI_SR_TIFRE;
    if (m_Port->CFG1 & SPI_CFG1_CRCEN) {
        errorFlags |= SPI_SR_CRCE;
    }
    const uint32_t wordCount = transaction.Length >> m_TransferCountBitShift;

    if (transaction.ReceiveBuffer != nullptr)
    {
//        SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(m_ReceiveBuffer), (transaction.Length + DCACHE_LINE_SIZE - 1) & ~(DCACHE_LINE_SIZE - 1));

        m_ReceiveDMAChannel.SetMemoryAddress(m_ReceiveBuffer);
        m_ReceiveDMAChannel.SetTransferLength(wordCount);
        m_ReceiveDMAChannel.Start();
        m_Port->CFG1 |= SPI_CFG1_RXDMAEN;
    }
    if (transaction.TransmitBuffer != nullptr)
    {
//        uint32_t startAddr = uint32_t(transaction.TransmitBuffer) & ~(DCACHE_LINE_SIZE - 1);
//        uint32_t endAddr = (uint32_t(transaction.TransmitBuffer) + transaction.Length + DCACHE_LINE_SIZE - 1) & ~(DCACHE_LINE_SIZE - 1);
//        SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(startAddr), endAddr - startAddr);

        m_SendDMAChannel.SetMemoryAddress(transaction.TransmitBuffer);
        m_SendDMAChannel.SetTransferLength(wordCount);
        m_SendDMAChannel.Start();
        m_Port->CFG1 |= SPI_CFG1_TXDMAEN;
    }

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        m_Port->IFCR = SPI_IER_EOTIE | errorFlags;

        uint32_t CR2 = 0;
        set_bit_group(CR2, SPI_CR2_TSIZE_Msk, wordCount << SPI_CR2_TSIZE_Pos);
        m_Port->CR2 = CR2;

        m_Port->IER = SPI_IER_EOTIE | errorFlags;

        m_Port->CR1 |= SPI_CR1_SPE;
        m_Port->CR1 |= SPI_CR1_CSTART;

        const bool result = m_TransactionCondition.IRQWait();
        m_Port->IER = 0;

        m_SendDMAChannel.Stop();
        m_ReceiveDMAChannel.Stop();

        m_Port->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;

        m_Port->CR1 &= ~SPI_CR1_SPE;
        m_Port->CFG1 &= ~(SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);

        m_Port->IFCR = errorFlags;

        if (!result) return -1;
        if (m_TransactionError == SPIError::None)
        {
            if (transaction.ReceiveBuffer != nullptr)
            {
//                SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(m_ReceiveBuffer), (transaction.Length + DCACHE_LINE_SIZE - 1) & ~(DCACHE_LINE_SIZE - 1));
                memcpy(transaction.ReceiveBuffer, m_ReceiveBuffer, transaction.Length);
            }
            return transaction.Length;
        }
        else
        {
            set_last_error(EIO);
            return -1;
        }
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SPIDriverINode::SPIIRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<SPIDriverINode*>(userData)->HandleSPIIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SPIDriverINode::HandleSPIIRQ()
{
    if ((m_Port->SR & m_Port->IER) == 0) {
        return IRQResult::UNHANDLED;
    }
    if ((m_Port->SR & m_Port->IER) == SPI_SR_EOT) {
        m_TransactionError = SPIError::None;
    } else if (m_Port->SR & SPI_SR_CRCE) {
        m_TransactionError = SPIError::CRCError;
    } else if (m_Port->SR & SPI_SR_TIFRE) {
        m_TransactionError = SPIError::TIFrameError;
    } else if (m_Port->SR & SPI_SR_MODF) {
        m_TransactionError = SPIError::ModeFault;
    } else {
        m_TransactionError = SPIError::Unknown;
    }
    m_Port->IFCR = m_Port->IER;
    m_TransactionCondition.WakeupAll();
    KSWITCH_CONTEXT();

    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SPIDriver::SPIDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SPIDriver::Setup(const SPIDriverSetup& setup)
{
    Ptr<SPIDriverINode> node = ptr_new<SPIDriverINode>(setup, this);
    Kernel::RegisterDevice(setup.DevicePath.c_str(), node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SPIDriver::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    Ptr<SPIDriverINode> node = ptr_static_cast<SPIDriverINode>(file->GetINode());
    return node->Read(file, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SPIDriver::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    Ptr<SPIDriverINode> node = ptr_static_cast<SPIDriverINode>(file->GetINode());
    return node->Write(file, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int SPIDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<SPIDriverINode> node = ptr_static_cast<SPIDriverINode>(file->GetINode());
    return node->DeviceControl(request, inData, inDataLength, outData, outDataLength);
}


} // namespace
