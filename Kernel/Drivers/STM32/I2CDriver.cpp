// This file is part of PadOS.
//
// Copyright (C) 2020-2024 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <string.h>
#include <cmath>

#include <Utils/Logging.h>
#include <System/System.h>
#include <System/ExceptionHandling.h>
#include <Kernel/Drivers/STM32/I2CDriver.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KSemaphore.h>
#include <Kernel/SpinTimer.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KDriverDescriptor.h>

namespace kernel
{

PDEFINE_LOG_CATEGORY(LogCategoryI2CDriver, "I2CDRV", PLogSeverity::WARNING);

PREGISTER_KERNEL_DRIVER(I2CDriverINode, I2CDriverParameters);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

I2CDriverINode::I2CDriverINode(const I2CDriverParameters& parameters)
    : KINode(nullptr, nullptr, this, false)
    , m_Mutex("I2CDriverINode", PEMutexRecursionMode_RaiseError)
    , m_RequestCondition("I2CDriverINodeRequest")
    , m_ClockPin(parameters.ClockPin)
    , m_DataPin(parameters.DataPin)
    , m_ClockFrequency(parameters.ClockFrequency)
    , m_FallTime(parameters.FallTime)
    , m_RiseTime(parameters.RiseTime)
{
    m_State = State_e::Idle;

    m_Port = get_i2c_from_id(parameters.PortID);

    DigitalPin clockPin(m_ClockPin.PINID);
    DigitalPin dataPin(m_DataPin.PINID);

    clockPin.SetDirection(DigitalPinDirection_e::OpenCollector);
    dataPin.SetDirection(DigitalPinDirection_e::OpenCollector);

    clockPin.SetPeripheralMux(m_ClockPin.MUX);
    dataPin.SetPeripheralMux(m_DataPin.MUX);

    clockPin.SetPullMode(PinPullMode_e::Up);
    dataPin.SetPullMode(PinPullMode_e::Up);

#if defined(STM32H7)
    const IRQn_Type eventIRQ = get_i2c_irq(parameters.PortID, I2CIRQType::Event);
    const IRQn_Type errorIRQ = get_i2c_irq(parameters.PortID, I2CIRQType::Error);

    register_irq_handler(eventIRQ, IRQCallbackEvent, this);
    register_irq_handler(errorIRQ, IRQCallbackError, this);
#elif defined(STM32G0)
    const IRQn_Type portIRQ = get_i2c_irq(parameters.PortID);
    register_irq_handler(portIRQ, IRQCallbackEvent, this);
#else
#error Unknown platform.
#endif

    SetSpeed(I2CSpeed::Fast);

    ClearBus();
    ResetPeripheral();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

I2CDriverINode::~I2CDriverINode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> I2CDriverINode::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int flags)
{
    Ptr<I2CFile> file = ptr_new<I2CFile>(flags);
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

    int* inArg  = (int*)inData;
    int* outArg = (int*)outData;

    switch(request)
    {
        case I2CIOCTL_SET_SLAVE_ADDRESS:
            if (inArg == nullptr || inDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            i2cfile->m_SlaveAddress = uint8_t(*inArg);
            break;
        case I2CIOCTL_GET_SLAVE_ADDRESS:
            if (outArg == nullptr || outDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            *outArg = i2cfile->m_SlaveAddress;
            break;
        case I2CIOCTL_SET_INTERNAL_ADDR_LEN:
            if (inArg == nullptr || inDataLength != sizeof(int) || *inArg < 0 || *inArg > sizeof(m_RegisterAddress)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            i2cfile->m_InternalAddressLength = uint8_t(*inArg);
            break;
        case I2CIOCTL_GET_INTERNAL_ADDR_LEN:
            if (outArg == nullptr || outDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            *outArg = i2cfile->m_InternalAddressLength;
            break;
        case I2CIOCTL_SET_BAUDRATE:
            if (inArg == nullptr || inDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
//            SetBaudrate(*inArg);
            break;
        case I2CIOCTL_GET_BAUDRATE:
            if (outArg == nullptr || outDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            *outArg = GetBaudrate();
            break;
        case I2CIOCTL_SET_TIMEOUT:
            if (inData == nullptr || inDataLength != sizeof(bigtime_t)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            i2cfile->m_Timeout = TimeValNanos::FromNanoseconds(*reinterpret_cast<const bigtime_t*>(inData));
            break;
        case I2CIOCTL_GET_TIMEOUT:
            if (outData == nullptr || outDataLength != sizeof(bigtime_t)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            *reinterpret_cast<bigtime_t*>(outData) = i2cfile->m_Timeout.AsNanoseconds();
            break;
        case I2CIOCTL_CLEAR_BUS:
            ClearBus();
            break;
        default: PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t I2CDriverINode::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position)
{
    if (length == 0) {
        return 0;
    }
    CRITICAL_SCOPE(m_Mutex);
    
    if (m_Port->ISR & I2C_ISR_BUSY) {
        ResetPeripheral();
    }
   
    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

    m_Buffer = reinterpret_cast<uint8_t*>(buffer);
    m_Length = length;
    m_RegisterAddressPos = 0;
    m_CurPos = 0;
    m_TransactionError = PErrorCode::Success;

    uint32_t CR2 = I2C_CR2_START | ((i2cfile->m_SlaveAddress << I2C_CR2_SADD_Pos) & I2C_CR2_SADD_Msk) | ((i2cfile->m_SlaveAddressLength == I2C_ADDR_LEN_10BIT) ? I2C_CR2_ADD10 : 0);

    m_RegisterAddressLength = i2cfile->m_InternalAddressLength;
    if (m_RegisterAddressLength > 0)
    {
        m_State = State_e::SendReadAddress;

        for (int i = 0; i < m_RegisterAddressLength; ++i) m_RegisterAddress[i] = uint8_t((position >> ((m_RegisterAddressLength - i - 1) * 8)) & 0xff);

        CR2 |= uint32_t(m_RegisterAddressLength) << I2C_CR2_NBYTES_Pos;
        if (m_Port->ISR & I2C_ISR_TXE)
        {
            m_Port->TXDR = m_RegisterAddress[m_RegisterAddressPos++];
        }
    }
    else
    {
        m_State = State_e::Reading;

        if (m_Length <= (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos)) {
            CR2 |= m_Length << I2C_CR2_NBYTES_Pos;
        } else {
            CR2 |= I2C_CR2_NBYTES | I2C_CR2_RELOAD;
        }
        CR2 |= I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    }

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        const uint32_t interruptFlags = I2C_CR1_RXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;
        m_Port->ICR = I2C_ICR_ADDRCF | I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF | I2C_ICR_PECCF | I2C_ICR_TIMOUTCF | I2C_ICR_ALERTCF;
        m_Port->CR1 |= interruptFlags;
        m_Port->CR2 = CR2;

        if (m_RequestCondition.IRQWaitTimeout(i2cfile->m_Timeout) != PErrorCode::Success)
        {
            set_last_error(PErrorCode::Timeout);
            m_State = State_e::Idle;
            ResetPeripheral();
            m_TransactionError = PErrorCode::Timeout;
        }
        m_Port->CR1 &= ~interruptFlags;
    } CRITICAL_END;
    if (m_TransactionError != PErrorCode::Success)
    {
        ResetPeripheral();
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryI2CDriver, "I2CDriver::Read() request failed: {}", strerror(get_last_error()));
        PERROR_THROW_CODE(m_TransactionError);
    }
    return m_CurPos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t I2CDriverINode::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position)
{
    if (length == 0) {
        return 0;
    }
    CRITICAL_SCOPE(m_Mutex);

    if (m_Port->ISR & I2C_ISR_BUSY) {
        ResetPeripheral();
    }

    Ptr<I2CFile> i2cfile = ptr_static_cast<I2CFile>(file);

    m_Buffer = reinterpret_cast<uint8_t*>(const_cast<void*>(buffer));
    m_Length = length;
    m_RegisterAddressPos = 0;
    m_CurPos = 0;
    m_TransactionError = PErrorCode::Success;

    uint32_t CR2 = I2C_CR2_START | ((i2cfile->m_SlaveAddress << I2C_CR2_SADD_Pos) & I2C_CR2_SADD_Msk) | ((i2cfile->m_SlaveAddressLength == I2C_ADDR_LEN_10BIT) ? I2C_CR2_ADD10 : 0);

    m_RegisterAddressLength = i2cfile->m_InternalAddressLength;
    if (m_RegisterAddressLength > 0)
    {
        m_State = State_e::SendWriteAddress;

        for (int i = 0; i < m_RegisterAddressLength; ++i) m_RegisterAddress[i] = uint8_t((position >> ((m_RegisterAddressLength - i - 1) * 8)) & 0xff);

        int32_t totalLength = m_Length + m_RegisterAddressLength;
        if (totalLength <= (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos)) {
            CR2 |= (totalLength << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND;
        } else {
            CR2 |= I2C_CR2_NBYTES | I2C_CR2_RELOAD;
        }
        if (m_Port->ISR & I2C_ISR_TXE)
        {
            m_Port->TXDR = m_RegisterAddress[m_RegisterAddressPos++];
            if (m_RegisterAddressLength == 1) {
                m_State = State_e::Writing;
            }
        }
    }
    else
    {
        m_State = State_e::Writing;

        if (m_Length <= (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos)) {
            CR2 |= (m_Length << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND;
        } else {
            CR2 |= I2C_CR2_NBYTES | I2C_CR2_RELOAD;
        }
        if (m_Port->ISR & I2C_ISR_TXE)
        {
            kassert(m_CurPos >= 0);
            m_Port->TXDR = m_Buffer[m_CurPos++];
        }
    }

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        const uint32_t interruptFlags = I2C_CR1_TXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;

        m_Port->ICR = I2C_ICR_ADDRCF | I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF | I2C_ICR_PECCF | I2C_ICR_TIMOUTCF | I2C_ICR_ALERTCF;
        m_Port->CR1 |= interruptFlags;
        m_Port->CR2 = CR2;

        if (m_RequestCondition.IRQWaitTimeout(i2cfile->m_Timeout) != PErrorCode::Success)
        {
            set_last_error(PErrorCode::Timeout);
            m_State = State_e::Idle;
            ResetPeripheral();
            m_TransactionError = PErrorCode::Timeout;
        }
        m_Port->CR1 &= ~interruptFlags;
    } CRITICAL_END;
    if (m_TransactionError != PErrorCode::Success)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryI2CDriver, "I2CDriver::Write() request failed: {}", strerror(get_last_error()));
        PERROR_THROW_CODE(m_TransactionError);
    }
    return m_CurPos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::ResetPeripheral()
{
    m_Port->CR1 = 0;
    m_Port->CR1 = I2C_CR1_PE
                | ((m_DigitalFilterCount << I2C_CR1_DNF_Pos) & I2C_CR1_DNF_Msk)
                | ((m_AnalogFilterEnabled) ? 0 : I2C_CR1_ANFOFF);

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::ClearBus()
{
    DigitalPin clockPin(m_ClockPin.PINID);
    DigitalPin dataPin(m_DataPin.PINID);


    clockPin.SetPeripheralMux(DigitalPinPeripheralID::None);
    dataPin.SetPeripheralMux(DigitalPinPeripheralID::None);

    clockPin.SetDirection(DigitalPinDirection_e::OpenCollector);
    dataPin.SetDirection(DigitalPinDirection_e::OpenCollector);

    dataPin = true;
    clockPin = true;
    for (int i = 0; i < 16; ++i)
    {
        // Clear at ~50kHz
        clockPin = false;
        if (i == 15) {
            dataPin = false;
        }
        SpinTimer::SleepuS(10);
        clockPin = true;
        SpinTimer::SleepuS(10);
    }
    dataPin = true; // Send STOP
    SpinTimer::SleepuS(10);

    clockPin.SetPeripheralMux(m_ClockPin.MUX);
    dataPin.SetPeripheralMux(m_DataPin.MUX);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriverINode::SetSpeed(I2CSpeed speed)
{
    int speedIndex = int(speed);
    if (speedIndex < 0 || speedIndex > int(I2CSpeed::FastPlus)) {
        set_last_error(EINVAL);
        return -1;
    }

    CRITICAL_SCOPE(m_Mutex);

    const I2CSpec& spec = I2CSpecs[speedIndex];

    constexpr int32_t PRESC_MAX     = I2C_TIMINGR_PRESC_Msk >> I2C_TIMINGR_PRESC_Pos;
    constexpr int32_t SCLL_MAX      = I2C_TIMINGR_SCLL_Msk >> I2C_TIMINGR_SCLL_Pos;
    constexpr int32_t SCLH_MAX      = I2C_TIMINGR_SCLH_Msk >> I2C_TIMINGR_SCLH_Pos;
    constexpr int32_t SDADEL_MAX    = I2C_TIMINGR_SDADEL_Msk >> I2C_TIMINGR_SDADEL_Pos;
    constexpr int32_t SCLDEL_MAX    = I2C_TIMINGR_SCLDEL_Msk >> I2C_TIMINGR_SCLDEL_Pos;

    uint32_t minError = std::numeric_limits<uint32_t>::max();

    int32_t bestPRESC  = 0;
    int32_t bestSDADEL = 0;
    int32_t bestSCLDEL = 0;
    int32_t bestSCLL   = 0;
    int32_t bestSCLH   = 0;

    for (int prescale = 0; prescale <= PRESC_MAX; ++prescale)
    {
        uint32_t scaledClock = m_ClockFrequency / (prescale + 1);
        double clockCycleTime = double(1) / double(scaledClock);

        double filterDelayMin = m_AnalogFilterEnabled ? 50.0e-9 : 0.0;
        double filterDelayMax = 260.0e-9;

        filterDelayMin += double(m_DigitalFilterCount + 2) * clockCycleTime;
        filterDelayMax += double(m_DigitalFilterCount + 3) * clockCycleTime;

        double tsync1 = filterDelayMin + m_FallTime;
        double tsync2 = filterDelayMin + m_RiseTime;

        int32_t SCLL = std::max(0, int(ceil((spec.ClockLowMin - tsync1) / clockCycleTime)) - 1);
        if (SCLL > SCLL_MAX) continue;

        int32_t SCLH = std::max(0, int(ceil((spec.ClockHighMin - tsync2) / clockCycleTime)) - 1);
        if (SCLH > SCLH_MAX) continue;

        int32_t totalCycleTime = std::max(0, int(ceil((1.0 / double(spec.Baudrate) - tsync1 - tsync2) / clockCycleTime)) - 2);

        int32_t extraTime = totalCycleTime - SCLL - SCLH;
        int32_t extraTimeH = extraTime / 2;
        int32_t extraTimeL = extraTime - extraTimeH;

        SCLL += extraTimeL;
        if (SCLL > SCLL_MAX)
        {
            extraTimeH += SCLL - SCLL_MAX;
            SCLL = SCLL_MAX;
        }
        SCLH += extraTimeH;
        if (SCLH > SCLH_MAX)
        {
            SCLL += SCLH - SCLH_MAX;
            SCLH = SCLH_MAX;
        }
        if (SCLL > SCLL_MAX) {
            continue;
        }

        double holdTimeMin = spec.DataHoldTimeMin + m_FallTime - filterDelayMin;
        double validTimeMax = spec.DataValidTimeMax - m_RiseTime - filterDelayMax;

        int32_t validClocksMax = std::max(0, int(ceil(validTimeMax / clockCycleTime)) - 1);
        if (validClocksMax > SDADEL_MAX) validClocksMax = SDADEL_MAX;

        int32_t SDADEL = std::max(0, int(ceil(holdTimeMin / clockCycleTime)) - 1);
        if (SDADEL > validClocksMax) {
            continue;
        }

        int32_t SCLDEL = std::max(0, int(ceil(spec.DataSetupTimeMin / clockCycleTime)) - 1);
        if (SDADEL > SCLDEL_MAX) {
            continue;
        }
        double clockPeriod = double(SCLL + SCLH + 2) * clockCycleTime + tsync1 + tsync2;

        uint32_t rate = uint32_t(1.0 / clockPeriod);
        if (rate < spec.BaudrateMin || rate > spec.Baudrate) {
            continue;
        }

        uint32_t error = abs(rate - spec.Baudrate);
        if (error < minError)
        {
            minError = error;

            bestPRESC = prescale;
            bestSDADEL = SDADEL;
            bestSCLDEL = SCLDEL;
            bestSCLL = SCLL;
            bestSCLH = SCLH;
            m_Baudrate = rate;
        }
    }
    if (minError == std::numeric_limits<uint32_t>::max()) {
        kernel_log<PLogSeverity::CRITICAL>(LogCategoryI2CDriver, "ERROR: I2C failed to set baudrate!");
        return -1;
    }
//  m_Port->TIMINGR = 0x00b03fdb;
    m_Port->TIMINGR = (bestPRESC  << I2C_TIMINGR_PRESC_Pos)
                    | (bestSDADEL << I2C_TIMINGR_SDADEL_Pos)
                    | (bestSCLDEL << I2C_TIMINGR_SCLDEL_Pos)
                    | (bestSCLL   << I2C_TIMINGR_SCLL_Pos)
                    | (bestSCLH   << I2C_TIMINGR_SCLH_Pos);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int I2CDriverINode::GetBaudrate() const
{
    return m_Baudrate;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void I2CDriverINode::UpdateTransactionLength(uint32_t& CR2)
{
    CR2 &= ~(I2C_CR2_NBYTES_Msk | I2C_CR2_RELOAD);

    int32_t remainingLength = m_Length - m_CurPos;
    if (remainingLength <= (I2C_CR2_NBYTES_Msk >> I2C_CR2_NBYTES_Pos)) {
        CR2 |= (remainingLength << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND;
    } else {
        CR2 |= I2C_CR2_NBYTES_Msk | I2C_CR2_RELOAD;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult I2CDriverINode::IRQCallbackEvent(IRQn_Type irq, void* userData)
{
    return static_cast<I2CDriverINode*>(userData)->HandleEventIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult I2CDriverINode::HandleEventIRQ()
{
    if (m_Port->ISR & I2C_ISR_NACKF)
    {
        m_TransactionError = PErrorCode::ConnectionRefused;
        m_Port->ICR = I2C_ICR_NACKCF;
        m_Port->CR1 &= ~I2C_CR1_TXIE;

        m_State = State_e::Idle;
        m_RequestCondition.Wakeup(1);
        return IRQResult::HANDLED;
    }

    switch (m_State)
    {
    case State_e::SendReadAddress:
        while (m_RegisterAddressPos != m_RegisterAddressLength && (m_Port->ISR & I2C_ISR_TXE))
        {
            m_Port->TXDR = m_RegisterAddress[m_RegisterAddressPos++];
        }
        if (m_Port->ISR & I2C_ISR_STOPF) // Transfer complete.
        {
            m_Port->ICR = I2C_ICR_STOPCF;
            uint32_t CR2 = m_Port->CR2;
            UpdateTransactionLength(CR2);
            CR2 |= I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
            m_Port->CR2 = CR2;
            m_State = State_e::Reading;
        }
        else if (m_Port->ISR & I2C_ISR_TC) // Transfer complete.
        {
            uint32_t CR2 = m_Port->CR2;
            UpdateTransactionLength(CR2);
            CR2 |= I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
            m_Port->CR2 = CR2;
            m_State = State_e::Reading;
        }
        break;
    case State_e::SendWriteAddress:
        while (m_RegisterAddressPos != m_RegisterAddressLength && (m_Port->ISR & I2C_ISR_TXE))
        {
            m_Port->TXDR = m_RegisterAddress[m_RegisterAddressPos++];
        }
        if (m_RegisterAddressPos == m_RegisterAddressLength) {
            m_State = State_e::Writing;
        }
        break;
    default:
        break;
    }
    if (m_Port->ISR & I2C_ISR_TCR) // Transfer complete reload.
    {
        uint32_t CR2 = m_Port->CR2;
        UpdateTransactionLength(CR2);
        m_Port->CR2 = CR2;
    }
    switch (m_State)
    {
    case State_e::Reading:
        kassert(m_CurPos >= 0);
        while ((m_Port->ISR & I2C_ISR_RXNE) && m_CurPos != m_Length)
        {
            m_Buffer[m_CurPos++] = uint8_t(m_Port->RXDR);
        }
        break;
    case State_e::Writing:
        kassert(m_CurPos >= 0);
        while ((m_Port->ISR & I2C_ISR_TXE) && m_CurPos != m_Length)
        {
            m_Port->TXDR = m_Buffer[m_CurPos++];
        }
        break;
    default:
        break;
    }
    if (m_Port->ISR & I2C_ISR_STOPF) // Transfer complete.
    {
        m_Port->ICR = I2C_ICR_STOPCF;
        m_Port->CR1 &= ~I2C_CR1_TXIE;
        m_State = State_e::Idle;
        m_RequestCondition.Wakeup(1);
    }
    if (m_Port->ISR & I2C_ISR_TC) // Transfer complete.
    {
        m_Port->CR1 &= ~I2C_CR1_TXIE;
        m_State = State_e::Idle;
        m_RequestCondition.Wakeup(1);
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult I2CDriverINode::IRQCallbackError(IRQn_Type irq, void* userData)
{
    return static_cast<I2CDriverINode*>(userData)->HandleErrorIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult I2CDriverINode::HandleErrorIRQ()
{
    m_Port->CR1 &= ~I2C_CR1_ERRIE;
    m_TransactionError = PErrorCode::IOError;
    m_RequestCondition.Wakeup(1);

    return IRQResult::HANDLED;
}


} // namespace kernel
