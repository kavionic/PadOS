// This file is part of PadOS.
//
// Copyright (C) 2022-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.04.2022 21:00

#include <chrono>
#include <time.h>
#include <stm32h7xx.h>

#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RealtimeClock::SetClock(TimeValNanos time)
{
    time_t unixTime = time.AsSecondsI();

    const tm* timeInfo = gmtime(&unixTime);

    PWR->CR1 |= PWR_CR1_DBP; // Disable Back-up domain protection.

    // Remove RTC write protection.
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;

    RTC->ISR |= RTC_ISR_INIT;
    while ((RTC->ISR & RTC_ISR_INITF) == 0) {}

    int year = timeInfo->tm_year + 1900 - 2000;
    int month = timeInfo->tm_mon + 1;

    uint32_t timeReg = RTC->TR;
    uint32_t dateReg = RTC->DR;

    set_bit_group(dateReg, RTC_DR_YT_Msk, (year / 10) << RTC_DR_YT_Pos);
    set_bit_group(dateReg, RTC_DR_YU_Msk, (year % 10) << RTC_DR_YU_Pos);

    set_bit_group(dateReg, RTC_DR_MT_Msk, (month / 10) << RTC_DR_MT_Pos);
    set_bit_group(dateReg, RTC_DR_MU_Msk, (month % 10) << RTC_DR_MU_Pos);

    set_bit_group(dateReg, RTC_DR_DT_Msk, (timeInfo->tm_mday / 10) << RTC_DR_DT_Pos);
    set_bit_group(dateReg, RTC_DR_DU_Msk, (timeInfo->tm_mday % 10) << RTC_DR_DU_Pos);


    set_bit_group(timeReg, RTC_TR_HT_Msk, (timeInfo->tm_hour / 10) << RTC_TR_HT_Pos);
    set_bit_group(timeReg, RTC_TR_HU_Msk, (timeInfo->tm_hour % 10) << RTC_TR_HU_Pos);

    set_bit_group(timeReg, RTC_TR_MNT_Msk, (timeInfo->tm_min / 10) << RTC_TR_MNT_Pos);
    set_bit_group(timeReg, RTC_TR_MNU_Msk, (timeInfo->tm_min % 10) << RTC_TR_MNU_Pos);

    set_bit_group(timeReg, RTC_TR_ST_Msk, (timeInfo->tm_sec / 10) << RTC_TR_ST_Pos);
    set_bit_group(timeReg, RTC_TR_SU_Msk, (timeInfo->tm_sec % 10) << RTC_TR_SU_Pos);

    RTC->DR = dateReg;
    RTC->TR = timeReg;

    RTC->ISR &= ~RTC_ISR_INIT;

    // Re-enable RTC write protection.
    RTC->WPR = 0x00;

    PWR->CR1 &= ~PWR_CR1_DBP; // Enable Back-up domain protection.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos RealtimeClock::GetClock()
{
    if (RTC->ISR & RTC_ISR_INITS)
    {
        // Wait for the shadow registers to update.
        RTC->ISR &= ~RTC_ISR_RSF;
        while ((RTC->ISR & RTC_ISR_RSF) == 0) {}

        // Reading SSR or TR freeze shadow register updates.
        const uint32_t subSecond = (RTC->SSR & RTC_SSR_SS_Msk) >> RTC_SSR_SS_Pos;
        const uint32_t timeReg = RTC->TR;
        const uint32_t dateReg = RTC->DR;    // Read DR last as this restore shadow register updates.

        const int rtcSecond = int(((timeReg & RTC_TR_ST_Msk) >> RTC_TR_ST_Pos) * 10 + ((timeReg & RTC_TR_SU_Msk) >> RTC_TR_SU_Pos));
        const int rtcMinute = int(((timeReg & RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos) * 10 + ((timeReg & RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos));
        int rtcHour = int(((timeReg & RTC_TR_HT_Msk) >> RTC_TR_HT_Pos) * 10 + ((timeReg & RTC_TR_HU_Msk) >> RTC_TR_HU_Pos));
        if (timeReg & RTC_TR_PM) {
            rtcHour += 12;
        }

        const int rtcDay = int(((dateReg & RTC_DR_DT_Msk) >> RTC_DR_DT_Pos) * 10 + ((dateReg & RTC_DR_DU_Msk) >> RTC_DR_DU_Pos));
        const int rtcMonth = int(((dateReg & RTC_DR_MT_Msk) >> RTC_DR_MT_Pos) * 10 + ((dateReg & RTC_DR_MU_Msk) >> RTC_DR_MU_Pos));
        const int rtcYear = 2000 + int(((dateReg & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos) * 10 + ((dateReg & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos));

        const std::chrono::year_month_day rtcCalendarDate(
            std::chrono::year(rtcYear),
            std::chrono::month(static_cast<unsigned>(rtcMonth)),
            std::chrono::day(static_cast<unsigned>(rtcDay))
        );
        const std::chrono::sys_seconds rtcDateTime =
            std::chrono::sys_days(rtcCalendarDate)
            + std::chrono::hours(rtcHour)
            + std::chrono::minutes(rtcMinute)
            + std::chrono::seconds(rtcSecond);
        const time_t unixTime = static_cast<time_t>(rtcDateTime.time_since_epoch().count());
        const uint32_t SyncPreDiv = (RTC->PRER & RTC_PRER_PREDIV_S_Msk) >> RTC_PRER_PREDIV_S_Pos;

        TimeValNanos currentTime = TimeValNanos::FromSeconds(unixTime);
        currentTime += TimeValNanos::FromSeconds(double(SyncPreDiv - subSecond) / double(SyncPreDiv + 1));
        
        return currentTime;
    }
    return TimeValNanos::zero;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RealtimeClock::WriteBackupRegister_trw(size_t registerIndex, uint32_t value)
{
    const uint32_t registerCount = &RTC->BKP31R - &RTC->BKP0R + 1;
    if (registerIndex >= registerCount) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    volatile uint32_t* backupRegisters = &RTC->BKP0R;
    PWR->CR1 |= PWR_CR1_DBP; // Disable Back-up domain protection.
    backupRegisters[registerIndex] = value;
    PWR->CR1 &= ~PWR_CR1_DBP; // Enable Back-up domain protection.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t RealtimeClock::ReadBackupRegister_trw(size_t registerIndex)
{
    const uint32_t registerCount = &RTC->BKP31R - &RTC->BKP0R + 1;
    if (registerIndex >= registerCount) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    volatile uint32_t* backupRegisters = &RTC->BKP0R;
    return backupRegisters[registerIndex];
}

