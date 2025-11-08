// This file is part of PadOS.
//
// Copyright (C) 2023-2025 Kurt Skauen <http://kavionic.com/>
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
#include <Kernel/VFS/KDriverParametersBase.h>
#include <Kernel/HAL/STM32/DMARequestID.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/HAL/DMA.h>
#include <DeviceControl/SPI.h>

enum class SPIID : int;

struct SPIDriverParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "spi";

    SPIDriverParameters() = default;

    SPIDriverParameters(
        const PString&              devicePath,
        SPIID                       portID,
        PinMuxTarget                pinCLK,
        PinMuxTarget                pinMOSI,
        PinMuxTarget                pinMISO,
        DigitalPinDriveStrength_e   pinDriveStrength = DigitalPinDriveStrength_e::Low,
        SPIBaudRateDivider          baudRateDivider = SPIBaudRateDivider::DIV2,
        SPIRole                     role = SPIRole::Master,
        SPIComMode                  comMode = SPIComMode::FullDuplex,
        SPIProtocol                 protocol = SPIProtocol::Motorola,
        SPIEndian                   endian = SPIEndian::MSB,
        SPICLkPolarity              clockPolarity = SPICLkPolarity::Low,
        SPICLkPhase                 clockPhase = SPICLkPhase::FirstEdge,
        SPISlaveUnderrunMode        slaveUnderrunMode = SPISlaveUnderrunMode::ConstantPattern,
        int32_t                     fifoThreshold = 1,
        int32_t                     crcSize = 0,
        int32_t                     crcPolynomial = 7,
        int32_t                     wordSize = 8,
        uint32_t                    interWordIdleCycles = 0,
        size_t                      receiveBufferSize = 0
    ) : KDriverParametersBase(devicePath)
      , PortID(portID)
      , PinCLK(pinCLK)
      , PinMOSI(pinMOSI)
      , PinMISO(pinMISO)
      , PinDriveStrength(pinDriveStrength)
      , BaudRateDivider(baudRateDivider)
      , Role(role)
      , ComMode(comMode)
      , Protocol(protocol)
      , Endian(endian)
      , ClockPolarity(clockPolarity)
      , ClockPhase(clockPhase)
      , SlaveUnderrunMode(slaveUnderrunMode)
      , FIFOThreshold(fifoThreshold)
      , CRCSize(crcSize)
      , CRCPolynomial(crcPolynomial)
      , WordSize(wordSize)
      , InterWordIdleCycles(interWordIdleCycles)
      , ReceiveBufferSize(receiveBufferSize)
    {}

    SPIID                       PortID;
    PinMuxTarget                PinCLK;
    PinMuxTarget                PinMOSI;
    PinMuxTarget                PinMISO;
    DigitalPinDriveStrength_e   PinDriveStrength = DigitalPinDriveStrength_e::Low;
    SPIBaudRateDivider          BaudRateDivider = SPIBaudRateDivider::DIV2;
    SPIRole                     Role = SPIRole::Master;
    SPIComMode                  ComMode = SPIComMode::FullDuplex;
    SPIProtocol                 Protocol = SPIProtocol::Motorola;
    SPIEndian                   Endian = SPIEndian::MSB;
    SPICLkPolarity              ClockPolarity = SPICLkPolarity::Low;
    SPICLkPhase                 ClockPhase = SPICLkPhase::FirstEdge;
    SPISlaveUnderrunMode        SlaveUnderrunMode = SPISlaveUnderrunMode::ConstantPattern;
    int32_t                     FIFOThreshold = 1;
    int32_t                     CRCSize = 0;
    int32_t                     CRCPolynomial = 7;
    int32_t                     WordSize = 8;
    uint32_t                    InterWordIdleCycles = 0;
    size_t                      ReceiveBufferSize = 0;

    friend void to_json(Pjson& data, const SPIDriverParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
        data.update(Pjson{
            {"port_id",                 value.PortID},
            {"pin_clk",                 value.PinCLK},
            {"pin_mosi",                value.PinMOSI},
            {"pin_miso",                value.PinMISO},
            {"pin_drive_strength",      value.PinDriveStrength},
            {"baudrate_divider",        value.BaudRateDivider},
            {"role",                    value.Role},
            {"com_mode",                value.ComMode},
            {"protocol",                value.Protocol},
            {"endian",                  value.Endian},
            {"clock_polarity",          value.ClockPolarity},
            {"clock_phase",             value.ClockPhase},
            {"slave_underrun_mode",     value.SlaveUnderrunMode},
            {"fifo_threshold",          value.FIFOThreshold},
            {"crc_size",                value.CRCSize},
            {"crc_polynomial",          value.CRCPolynomial},
            {"word_size",               value.WordSize},
            {"inter_word_idle_cycles",  value.InterWordIdleCycles},
            {"receive_buffer_size",     value.ReceiveBufferSize}
        });
    }
    friend void from_json(const Pjson& data, SPIDriverParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));

        data.at("port_id").get_to(outValue.PortID);
        data.at("pin_clk").get_to(outValue.PinCLK);
        data.at("pin_mosi").get_to(outValue.PinMOSI);
        data.at("pin_miso").get_to(outValue.PinMISO);
        data.at("pin_drive_strength").get_to(outValue.PinDriveStrength);
        data.at("baudrate_divider").get_to(outValue.BaudRateDivider);
        data.at("role").get_to(outValue.Role);
        data.at("com_mode").get_to(outValue.ComMode);
        data.at("protocol").get_to(outValue.Protocol);
        data.at("endian").get_to(outValue.Endian);
        data.at("clock_polarity").get_to(outValue.ClockPolarity);
        data.at("clock_phase").get_to(outValue.ClockPhase);
        data.at("slave_underrun_mode").get_to(outValue.SlaveUnderrunMode);
        data.at("fifo_threshold").get_to(outValue.FIFOThreshold);
        data.at("crc_size").get_to(outValue.CRCSize);
        data.at("crc_polynomial").get_to(outValue.CRCPolynomial);
        data.at("word_size").get_to(outValue.WordSize);
        data.at("inter_word_idle_cycles").get_to(outValue.InterWordIdleCycles);
        data.at("receive_buffer_size").get_to(outValue.ReceiveBufferSize);
    }

};

namespace kernel
{

class SPIDriver;


class SPIDriverINode : public KINode, public KFilesystemFileOps
{
public:
    SPIDriverINode(const SPIDriverParameters& setup);

    virtual void DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    void    SetBaudrateDivider(SPIBaudRateDivider baudRateDivider);

    bool    SetPinMode(const PinMuxTarget& pin, SPIPinMode mode);

    void    SetSwapMOSIMISO(bool doSwap);
    bool    GetSwapMOSIMISO() const;

    void    StartTransaction(const SPITransaction& transaction);

    static IRQResult    SPIIRQCallback(IRQn_Type irq, void* userData);
    IRQResult           HandleSPIIRQ();

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


} // namespace
