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

#pragma once

#include <functional>
#include <stdint.h>
#include <System/Sections.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Signals/SignalUnguarded.h>

namespace kernel
{
enum class IRQResult : int;

enum class ADC_Polarity : uint32_t
{
    SingleEnded,
    Differential,
    SingleAndDiff
};

enum class ADC_BoostMode : uint32_t
{
    BOOST6_25MHz = 0,
    BOOST12_5MHz = 1,
    BOOST25MHz   = 2,
    BOOST50MHZ   = 3
};

enum class ADC_DataMode : uint32_t
{
    FIFO,
    OVERWRITE
};

enum class ADC_SampleTime : uint32_t
{
    SampleClocks_1_5    = 0,
    SampleClocks_2_5    = 1,
    SampleClocks_8_5    = 2,
    SampleClocks_16_5   = 3,
    SampleClocks_32_5   = 4,
    SampleClocks_64_5   = 5,
    SampleClocks_387_5  = 6,
    SampleClocks_810_5  = 7
};

enum class ADC_ClockMode : uint32_t
{
    ASYNC,  // Clocked asynchronously from adc_ker_ck.
    SYNC_1, // Clocked synchronously from adc_hclk
    SYNC_2, // Clocked synchronously from adc_hclk / 2
    SYNC_4  // Clocked synchronously from adc_hclk / 4
};

enum class ADC_AsyncClockPrescale : uint32_t
{
    PRESC_1     = 0,
    PRESC_2     = 1,
    PRESC_4     = 2,
    PRESC_6     = 3,
    PRESC_8     = 4,
    PRESC_10    = 5,
    PRESC_12    = 6,
    PRESC_16    = 7,
    PRESC_32    = 8,
    PRESC_64    = 9,
    PRESC_128   = 10,
    PRESC_256   = 11
};

enum class ADC_WatchdogID : int
{
    WD1,
    WD2,
    WD3
};

// Address of various calibration data programmed during device production (specific to each device).

static const uint16_t& ADC_TS_CAL1      = *reinterpret_cast<uint16_t*>(0x1ff1e820); // Temperature sensor calibration (value at 30degC).
static const uint16_t& ADC_TS_CAL2      = *reinterpret_cast<uint16_t*>(0x1ff1e840); // Temperature sensor calibration (value at 110degC).
static const uint16_t& ADC_VREFINT_CAL  = *reinterpret_cast<uint16_t*>(0x1ff1e860); // VRef calibration (value at 30degC).

static const uint32_t* const ADC1_LINEAR_CALIB_REGISTERS = reinterpret_cast<const uint32_t*>(intptr_t(0x1ff1ec00)); // Linearity calibration words for ADC1.
static const uint32_t* const ADC2_LINEAR_CALIB_REGISTERS = reinterpret_cast<const uint32_t*>(intptr_t(0x1ff1ec20)); // Linearity calibration words for ADC2.
static const uint32_t* const ADC3_LINEAR_CALIB_REGISTERS = reinterpret_cast<const uint32_t*>(intptr_t(0x1ff1ec40)); // Linearity calibration words for ADC3.
static constexpr uint32_t  ADC_LINEAR_CALIB_REGISTER_COUNT = 6;

static constexpr double ADC_CS_CAL1_TEMP = 30.0f;   // Temperature at which ADC_TS_CAL1 was produced.
static constexpr double ADC_CS_CAL2_TEMP = 110.0f;  // Temperature at which ADC_TS_CAL2 was produced.

static constexpr uint32_t ADC2_INTSRC_DAC1_1 = 16;  // ADC2 channel to which DAC1 output 1 is connected.
static constexpr uint32_t ADC2_INTSRC_DAC1_2 = 17;  // ADC2 channel to which DAC1 output 2 is connected.

static constexpr uint32_t ADC3_INTSRC_VBAT = 17;    // ADC3 channel to which the VBat voltage divider is connected.
static constexpr uint32_t ADC3_INTSRC_TEMP = 18;    // ADC3 channel to which the internal temperature sensor is connected.
static constexpr uint32_t ADC3_INTSRC_VREF = 19;    // ADC3 channel to which the internal reference voltage is connected.

class ADC_STM32
{
public:
    ADC_STM32(ADC_ID id);
    ~ADC_STM32();

    void EnablePower(bool enable);
    void Calibrate(ADC_Polarity polarity, bool calibrateLinearity = false);
    bool SetLinearCalibrationWord(int index, uint32_t value);
    bool LoadFactoryLinearCalibration();
    void SetBoostMode(ADC_BoostMode mode);
    void SetOversampling(uint32_t oversampling, uint32_t rightShift);
    void SetDataMode(ADC_DataMode mode);
    void SetAutoDelay(bool enable);
    void SetContinous(bool enable);
    void SetSampleTime(int channel, ADC_SampleTime sampleTime);
    void EnableRegularOversampling(bool enable);
    void EnableInjectedOversampling(bool enable);
    void SetChannelPreselect(int channel, bool preselect);
    void SetClockMode(ADC_ClockMode mode);
    void SetAsyncClockPrescale(ADC_AsyncClockPrescale prescale);

    IFLASHC void EnableADC(bool enable);

    IFLASHC void SetSequenceLength(size_t length);
    IFLASHC bool SetSequenceSlot(size_t index, int channel);
    IFLASHC int  GetSequenceSlot(size_t index) const;

    IFLASHC void SetInjectedSequenceLength(size_t length);
    IFLASHC bool SetInjectedSequenceSlot(size_t index, int channel);
    IFLASHC int  GetInjectedSequenceSlot(size_t index) const;

    IFLASHC int32_t GetCurrentChannelValue(int channel) const { return (channel >= 0 && channel < CHANNEL_COUNT) ? m_ChannelValues[channel] : 0; }

    IFLASHC void StartRegular();
    IFLASHC void StartInjected();

    IFLASHC void EnableRegularUpdateIRQ(bool enable);
    IFLASHC void EnableInjectedUpdateIRQ(bool enable);

    IFLASHC void EnableVBat(bool enable);
    IFLASHC void EnableTempSens(bool enable);
    IFLASHC void EnableVRef(bool enable);

    IFLASHC bool ConfigureWatchdog1(bool monitorRegularChannels, bool monitorInjectedChannels, int channel = -1);
    IFLASHC bool ConfigureWatchdog23(ADC_WatchdogID watchdogID, uint32_t channelMask);
    IFLASHC bool SetWatchdogThreshold(ADC_WatchdogID watchdogID, uint32_t lowThreshold, uint32_t highThreshold, bool clearAlarm = true);

    IFLASHC bool EnableWatchdog(ADC_WatchdogID watchdogID, bool enable, bool clearAlarm = true);
    IFLASHC bool IsWatchdogEnabled(ADC_WatchdogID watchdogID) const;

    SignalUnguarded<void, int/*channel*/, int32_t/*value*/>                 SignalChannelUpdated;
    SignalUnguarded<void, int/*channel*/, int32_t/*value*/>                 SignalInjectedChannelUpdated;
    SignalUnguarded<void, ADC_WatchdogID/*watchdogID*/, uint32_t/*value*/>  SignalWatchdogTriggered;

private:
    static constexpr uint32_t CHANNEL_COUNT = 20;

    IFLASHC static kernel::IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IFLASHC kernel::IRQResult HandleIRQ();

    ADC_ID              m_ADCID;
    ADC_TypeDef*        m_ADC = nullptr;
    ADC_Common_TypeDef* m_ADCCommon = nullptr;
    int                 m_IRQHandle = INVALID_HANDLE;

    int32_t     m_ChannelValues[CHANNEL_COUNT];
    uint32_t    m_CurrentSeqIndex = 0;
    uint32_t    m_CurrentInjectedSeqIndex = 0;
};


} // namespace kernel
