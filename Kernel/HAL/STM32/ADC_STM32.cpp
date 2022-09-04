// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 29.08.2022 20:00

#include <string.h>
#include <Kernel/HAL/STM32/ADC_STM32.h>
#include <Kernel/IRQDispatcher.h>
#include <Utils/Utils.h>

using namespace os;

namespace kernel
{

static constexpr struct { volatile uint32_t ADC_TypeDef::* Register; uint32_t Offset; } g_SequenceSlots[] =
{
    {&ADC_TypeDef::SQR1, ADC_SQR1_SQ1_Pos},
    {&ADC_TypeDef::SQR1, ADC_SQR1_SQ2_Pos},
    {&ADC_TypeDef::SQR1, ADC_SQR1_SQ3_Pos},
    {&ADC_TypeDef::SQR1, ADC_SQR1_SQ4_Pos},
    {&ADC_TypeDef::SQR2, ADC_SQR2_SQ5_Pos},
    {&ADC_TypeDef::SQR2, ADC_SQR2_SQ6_Pos},
    {&ADC_TypeDef::SQR2, ADC_SQR2_SQ7_Pos},
    {&ADC_TypeDef::SQR2, ADC_SQR2_SQ8_Pos},
    {&ADC_TypeDef::SQR2, ADC_SQR2_SQ9_Pos},
    {&ADC_TypeDef::SQR3, ADC_SQR3_SQ10_Pos},
    {&ADC_TypeDef::SQR3, ADC_SQR3_SQ11_Pos},
    {&ADC_TypeDef::SQR3, ADC_SQR3_SQ12_Pos},
    {&ADC_TypeDef::SQR3, ADC_SQR3_SQ13_Pos},
    {&ADC_TypeDef::SQR3, ADC_SQR3_SQ14_Pos},
    {&ADC_TypeDef::SQR4, ADC_SQR4_SQ15_Pos},
    {&ADC_TypeDef::SQR4, ADC_SQR4_SQ16_Pos}
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ADC_STM32::ADC_STM32(ADC_ID id)
{
    m_ADCID = id;
    m_ADC       = get_adc_from_id(id);
    m_ADCCommon = get_adc_common_from_id(id);

    memset(m_ChannelValues, 0, sizeof(m_ChannelValues));

    IRQn_Type irqID = get_adc_irq(m_ADCID);
    m_IRQHandle = register_irq_handler(irqID, &ADC_STM32::IRQCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ADC_STM32::~ADC_STM32()
{
    m_ADC->IER = 0;
    if (m_IRQHandle != INVALID_HANDLE)
    {
        IRQn_Type irqID = get_adc_irq(m_ADCID);
        unregister_irq_handler(irqID, m_IRQHandle);
        m_IRQHandle = INVALID_HANDLE;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnablePower(bool enable)
{
    set_bit_group(m_ADC->CR, ADC_CR_DEEPPWD | ADC_CR_ADVREGEN, ADC_CR_ADVREGEN);    // Take ADC out of deep power down and enable vreg.
    snooze_ms(2); // Wait for voltage regulator to start up (min 10uS, wait 2mS to make sure we get at least one system timer tick).
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::Calibrate(ADC_Polarity polarity, bool calibrateLinearity)
{
    uint32_t flags = ADC_CR_ADCAL;
    if (polarity == ADC_Polarity::Differential) flags |= ADC_CR_ADCALDIF;
    if (calibrateLinearity)                     flags |= ADC_CR_ADCALLIN;
    
    set_bit_group(m_ADC->CR, ADC_CR_ADCALLIN | ADC_CR_ADCALDIF | ADC_CR_ADCAL, flags);
    for (TimeoutTracker timeout(100); (m_ADC->CR & ADC_CR_ADCAL) && timeout;) {} // Wait for calibration to finish.

    if (polarity == ADC_Polarity::SingleAndDiff)
    {
        flags &= ~ADC_CR_ADCALLIN;
        flags |= ADC_CR_ADCALDIF;

        set_bit_group(m_ADC->CR, ADC_CR_ADCALLIN | ADC_CR_ADCALDIF | ADC_CR_ADCAL, flags);
        for (TimeoutTracker timeout(100); (m_ADC->CR & ADC_CR_ADCAL) && timeout;) {} // Wait for calibration to finish.
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::SetLinearCalibrationWord(int index, uint32_t value)
{
    const uint32_t mask = ADC_CR_LINCALRDYW1 << index;

    set_bit_group(m_ADC->CALFACT2, ADC_CALFACT2_LINCALFACT_Msk, value << ADC_CALFACT2_LINCALFACT_Pos);
    m_ADC->CR |= mask;
    for (TimeValMicros endTime = get_system_time() + TimeValMicros::FromMilliseconds(10); (m_ADC->CR & mask) == 0 && get_system_time() < endTime;) {}

    return (m_ADC->CR & mask) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::LoadFactoryLinearCalibration()
{
    const uint32_t* calibWords = nullptr;
    switch (m_ADCID)
    {
        case ADC_ID::ADC_1: calibWords = ADC1_LINEAR_CALIB_REGISTERS; break;
        case ADC_ID::ADC_2: calibWords = ADC2_LINEAR_CALIB_REGISTERS; break;
        case ADC_ID::ADC_3: calibWords = ADC3_LINEAR_CALIB_REGISTERS; break;
    }
    if (calibWords == nullptr) {
        return false;
    }
    for (int i = 0; i < ADC_LINEAR_CALIB_REGISTER_COUNT; ++i)
    {
        if (!SetLinearCalibrationWord(i, calibWords[i])) return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetBoostMode(ADC_BoostMode mode)
{
    set_bit_group(m_ADC->CR, ADC_CR_BOOST_Msk, uint32_t(mode) << ADC_CR_BOOST_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetOversampling(uint32_t oversampling, uint32_t rightShift)
{
    set_bit_group(m_ADC->CFGR2, ADC_CFGR2_OVSR_Msk, (oversampling - 1) << ADC_CFGR2_OVSR_Pos);
    set_bit_group(m_ADC->CFGR2, ADC_CFGR2_OVSS_Msk, rightShift << ADC_CFGR2_OVSS_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetDataMode(ADC_DataMode mode)
{
    switch (mode)
    {
        case ADC_DataMode::FIFO:
            m_ADC->CFGR &= ~ADC_CFGR_OVRMOD;
            break;
        case ADC_DataMode::OVERWRITE:
            m_ADC->CFGR |= ADC_CFGR_OVRMOD;
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Enable / disable auto delay. If enabled the ADC will stop until the data
/// from the previous conversion has been read.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetAutoDelay(bool enable)
{
    if (enable) {
        m_ADC->CFGR |= ADC_CFGR_AUTDLY;
    } else {
        m_ADC->CFGR &= ~ADC_CFGR_AUTDLY;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetContinous(bool enable)
{
    if (enable) {
        m_ADC->CFGR |= ADC_CFGR_CONT;
    } else {
        m_ADC->CFGR &= ~ADC_CFGR_CONT;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetSampleTime(int channel, ADC_SampleTime sampleTime)
{
    if (channel < 10) {
        set_bit_group(m_ADC->SMPR1, 0x7 << (channel * 3), uint32_t(sampleTime) << (channel * 3));
    } else if (channel < 20) {
        set_bit_group(m_ADC->SMPR2, 0x7 << ((channel - 10) * 3), uint32_t(sampleTime) << ((channel - 10) * 3));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableRegularOversampling(bool enable)
{
    if (enable) {
        m_ADC->CFGR2 |= ADC_CFGR2_ROVSE;
    } else {
        m_ADC->CFGR2 &= ~ADC_CFGR2_ROVSE;
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableInjectedOversampling(bool enable)
{
    if (enable) {
        m_ADC->CFGR2 |= ADC_CFGR2_JOVSE;
    } else {
        m_ADC->CFGR2 &= ~ADC_CFGR2_JOVSE;
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetChannelPreselect(int channel, bool preselect)
{
    const uint32_t mask = (1 << channel) << ADC_PCSEL_PCSEL_Pos;

    if (preselect) {
        m_ADC->PCSEL |= mask;
    } else {
        m_ADC->PCSEL &= ~mask;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetClockMode(ADC_ClockMode mode)
{
    set_bit_group(m_ADCCommon->CCR, ADC_CCR_CKMODE_Msk, uint32_t(mode) << ADC_CCR_CKMODE_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetAsyncClockPrescale(ADC_AsyncClockPrescale prescale)
{
    set_bit_group(m_ADCCommon->CCR, ADC_CCR_PRESC_Msk, uint32_t(prescale) << ADC_CCR_PRESC_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableADC(bool enable)
{
    if (enable)
    {
        m_ADC->ISR |= ADC_ISR_ADRDY;
        m_ADC->CR |= ADC_CR_ADEN;
        for (TimeoutTracker timeout(100); ((m_ADC->ISR & ADC_ISR_ADRDY) == 0) && timeout;) {} // Wait for startup.
        m_ADC->ISR |= ADC_ISR_ADRDY;
    }
    else
    {
        if (m_ADC->CR & (ADC_CR_ADSTART | ADC_CR_JADSTART))
        {
            m_ADC->CR |= ADC_CR_ADSTP | ADC_CR_JADSTP;
            for (TimeoutTracker timeout(100); (m_ADC->CR & (ADC_CR_ADSTART | ADC_CR_JADSTART)) && timeout;) {}
        }
        m_ADC->CR |= ADC_CR_ADDIS;
        for (TimeoutTracker timeout(100); (m_ADC->CR & ADC_CR_ADEN) == 0 && timeout;) {} // Wait for stop.
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetSequenceLength(size_t length)
{
    set_bit_group(m_ADC->SQR1, ADC_SQR1_L_Msk, uint32_t(length - 1) << ADC_SQR1_L_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::SetSequenceSlot(size_t index, int channel)
{
    if (index >= ARRAY_COUNT(g_SequenceSlots)) {
        return false;
    }
    uint32_t mask = ADC_SQR1_SQ1_Msk >> ADC_SQR1_SQ1_Pos;

    set_bit_group(m_ADC->*g_SequenceSlots[index].Register, mask << g_SequenceSlots[index].Offset, uint32_t(channel) << g_SequenceSlots[index].Offset);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int ADC_STM32::GetSequenceSlot(size_t index) const
{
    if (index >= ARRAY_COUNT(g_SequenceSlots)) {
        return -1;
    }
    uint32_t mask = ADC_SQR1_SQ1_Msk >> ADC_SQR1_SQ1_Pos;

    return (m_ADC->*g_SequenceSlots[index].Register >> g_SequenceSlots[index].Offset) & mask;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::SetInjectedSequenceLength(size_t length)
{
    set_bit_group(m_ADC->JSQR, ADC_JSQR_JL_Msk, uint32_t(length - 1) << ADC_JSQR_JL_Pos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::SetInjectedSequenceSlot(size_t index, int channel)
{
    if (index >= 4) {
        return false;
    }
    const uint32_t bitOffset = index * (ADC_JSQR_JSQ2_Pos - ADC_JSQR_JSQ1_Pos);
    const uint32_t mask = ADC_JSQR_JSQ1_Msk << bitOffset;

    set_bit_group(m_ADC->JSQR, mask, uint32_t(channel) << (ADC_JSQR_JSQ1_Pos + bitOffset));

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int ADC_STM32::GetInjectedSequenceSlot(size_t index) const
{
    if (index >= 4) {
        return -1;
    }
    const uint32_t bitOffset = ADC_JSQR_JSQ1_Pos + index * (ADC_JSQR_JSQ2_Pos - ADC_JSQR_JSQ1_Pos);
    const uint32_t mask = ADC_JSQR_JSQ1_Msk >> ADC_JSQR_JSQ1_Pos;

    return (m_ADC->JSQR >> bitOffset) & mask;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::StartRegular()
{
    m_CurrentSeqIndex = 0;
    m_ADC->CR |= ADC_CR_ADSTART;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::StartInjected()
{
    m_CurrentInjectedSeqIndex = 0;
    m_ADC->CR |= ADC_CR_JADSTART;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableRegularUpdateIRQ(bool enable)
{
    if (enable)
    {
        m_ADC->ISR |= ADC_ISR_EOC | ADC_ISR_EOS;
        m_ADC->IER |= ADC_IER_EOCIE | ADC_IER_EOSIE;
    }
    else
    {
        m_ADC->IER &= ~(ADC_IER_EOCIE | ADC_IER_EOSIE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableInjectedUpdateIRQ(bool enable)
{
    if (enable)
    {
        m_ADC->ISR |= ADC_ISR_JEOC | ADC_ISR_JEOS;
        m_ADC->IER |= ADC_IER_JEOCIE | ADC_IER_JEOSIE;
    }
    else
    {
        m_ADC->IER &= ~(ADC_IER_JEOCIE | ADC_IER_JEOSIE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableVBat(bool enable)
{
    if (enable) {
        m_ADCCommon->CCR |= ADC_CCR_VBATEN;
    } else {
        m_ADCCommon->CCR &= ~ADC_CCR_VBATEN;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableTempSens(bool enable)
{
    if (enable) {
        m_ADCCommon->CCR |= ADC_CCR_TSEN;
    } else {
        m_ADCCommon->CCR &= ~ADC_CCR_TSEN;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ADC_STM32::EnableVRef(bool enable)
{
    if (enable) {
        m_ADCCommon->CCR |= ADC_CCR_VREFEN;
    } else {
        m_ADCCommon->CCR &= ~ADC_CCR_VREFEN;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::ConfigureWatchdog1(bool monitorRegularChannels, bool monitorInjectedChannels, int channel)
{
    uint32_t value = 0;

    if (monitorRegularChannels)  value |= ADC_CFGR_AWD1EN;
    if (monitorInjectedChannels) value |= ADC_CFGR_JAWD1EN;
    if (channel >= 0)
    {
        value |= ADC_CFGR_AWD1SGL;
        set_bit_group(value, ADC_CFGR_AWD1CH_Msk, channel << ADC_CFGR_AWD1CH_Pos);
    }
    set_bit_group(m_ADC->CFGR, ADC_CFGR_AWD1EN | ADC_CFGR_JAWD1EN | ADC_CFGR_AWD1SGL | ADC_CFGR_AWD1CH_Msk, value);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::ConfigureWatchdog23(ADC_WatchdogID watchdogID, uint32_t channelMask)
{
    switch (watchdogID)
    {
        case kernel::ADC_WatchdogID::WD2:
            m_ADC->AWD2CR = channelMask << ADC_AWD2CR_AWD2CH_Pos;
            return true;
        case kernel::ADC_WatchdogID::WD3:
            m_ADC->AWD3CR = channelMask << ADC_AWD3CR_AWD3CH_Pos;
            return true;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::SetWatchdogThreshold(ADC_WatchdogID watchdogID, uint32_t lowThreshold, uint32_t highThreshold, bool clearAlarm)
{
    switch (watchdogID)
    {
        case kernel::ADC_WatchdogID::WD1:
            m_ADC->LTR1 = lowThreshold;
            m_ADC->HTR1 = highThreshold;
            if (clearAlarm) m_ADC->ISR |= ADC_ISR_AWD1;
            return true;
        case kernel::ADC_WatchdogID::WD2:
            m_ADC->LTR2 = lowThreshold;
            m_ADC->HTR2 = highThreshold;
            if (clearAlarm) m_ADC->ISR |= ADC_ISR_AWD2;
            return true;
        case kernel::ADC_WatchdogID::WD3:
            m_ADC->LTR3 = lowThreshold;
            m_ADC->HTR3 = highThreshold;
            if (clearAlarm) m_ADC->ISR |= ADC_ISR_AWD3;
            return true;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::EnableWatchdog(ADC_WatchdogID watchdogID, bool enable, bool clearAlarm)
{
    uint32_t enableMask = 0;
    uint32_t irqMask    = 0;

    switch (watchdogID)
    {
        case kernel::ADC_WatchdogID::WD1: irqMask = ADC_ISR_AWD1; enableMask = ADC_IER_AWD1IE; break;
        case kernel::ADC_WatchdogID::WD2: irqMask = ADC_ISR_AWD2; enableMask = ADC_IER_AWD2IE; break;
        case kernel::ADC_WatchdogID::WD3: irqMask = ADC_ISR_AWD3; enableMask = ADC_IER_AWD3IE; break;
        default: return false;
    }
    if (enable && clearAlarm) {
        m_ADC->ISR |= irqMask;
    }
    if (enable) {
        m_ADC->IER |= enableMask;
    } else {
        m_ADC->IER &= ~enableMask;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ADC_STM32::IsWatchdogEnabled(ADC_WatchdogID watchdogID) const
{
    uint32_t enableMask = 0;

    switch (watchdogID)
    {
        case kernel::ADC_WatchdogID::WD1: enableMask = ADC_IER_AWD1IE; break;
        case kernel::ADC_WatchdogID::WD2: enableMask = ADC_IER_AWD2IE; break;
        case kernel::ADC_WatchdogID::WD3: enableMask = ADC_IER_AWD3IE; break;
        default: return false;
    }
    return (m_ADC->IER & enableMask) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult ADC_STM32::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<ADC_STM32*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult ADC_STM32::HandleIRQ()
{
    IRQResult result = IRQResult::UNHANDLED;
    const uint32_t interrupts = m_ADC->ISR;
    if (interrupts & ADC_ISR_EOC)
    {
        const int32_t value = m_ADC->DR;
        const int channel = GetSequenceSlot(m_CurrentSeqIndex++);
        if (channel >= 0 && channel < CHANNEL_COUNT)
        {
            m_ChannelValues[channel] = value;
            if (DelegateChannelUpdated) DelegateChannelUpdated(channel, value);
        }
        result = IRQResult::HANDLED;
    }
    if (interrupts & ADC_ISR_EOS)
    {
        m_ADC->ISR |= ADC_ISR_EOS;
        m_CurrentSeqIndex = 0;
        result = IRQResult::HANDLED;
    }
    if (interrupts & ADC_ISR_JEOC)
    {
        m_ADC->ISR |= ADC_ISR_JEOC;

        if (m_CurrentInjectedSeqIndex < 4)
        {
            const uint32_t  value   = (&m_ADC->JDR1)[m_CurrentInjectedSeqIndex];
            const int       channel = GetInjectedSequenceSlot(m_CurrentInjectedSeqIndex++);
            m_ChannelValues[channel] = value;
            if (DelegateInjectedChannelUpdated) DelegateInjectedChannelUpdated(channel, value);
        }
    }
    const int32_t value = m_ADC->DR;
    if (interrupts & ADC_ISR_JEOS)
    {
        m_ADC->ISR |= ADC_ISR_JEOS;
        m_CurrentInjectedSeqIndex = 0;
    }
    if (interrupts & ADC_ISR_AWD1)
    {
        m_ADC->ISR |= ADC_ISR_AWD1;
        DelegateWatchdogTriggered(ADC_WatchdogID::WD1, value);
    }
    if (interrupts & ADC_ISR_AWD2)
    {
        m_ADC->ISR |= ADC_ISR_AWD2;
        DelegateWatchdogTriggered(ADC_WatchdogID::WD2, value);
    }
    if (interrupts & ADC_ISR_AWD3)
    {
        m_ADC->ISR |= ADC_ISR_AWD3;
        DelegateWatchdogTriggered(ADC_WatchdogID::WD3, value);
    }
    return result;
}

} // namespace kernel
