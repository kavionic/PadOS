// std_chrono_calendar_tests.cpp
// Calendar & formatting tests for <chrono> (C++20/23).
//
// Build knobs (override with -D…):
//   -DCHRONO_HAVE_PARSE=1   // force-enable from_stream tests if your lib supports it but lacks a macro
//
// Compile/link either with gtest_main, or call RunStdChronoCalendarTests() yourself.

#include <gtest/gtest.h>

#include <chrono>
#include <sstream>
#include <locale>
#include <type_traits>
#include <Utils/String.h>

using namespace std::chrono;
using namespace std::chrono_literals;

// ---------------- Feature gates ----------------
#ifndef __cpp_lib_chrono
#define __cpp_lib_chrono 0
#endif

// Some libraries ship parse()/from_stream but not a macro; allow override.
#ifndef CHRONO_HAVE_PARSE
#if (__cpp_lib_chrono >= 201907L)
#define CHRONO_HAVE_PARSE 1
#else
#define CHRONO_HAVE_PARSE 0
#endif
#endif

// ---------------- Duration & hh_mm_ss ----------------
TEST(ChronoCalendar, HhMmSsSplitAndNegative)
{
    using std::chrono::hh_mm_ss;
    hh_mm_ss<seconds> t1{3661s};
    EXPECT_EQ(t1.hours(), 1h);
    EXPECT_EQ(t1.minutes(), 1min);
    EXPECT_EQ(t1.seconds(), 1s);
    EXPECT_FALSE(t1.is_negative());

    hh_mm_ss<milliseconds> t2{-1500ms};
    EXPECT_TRUE(t2.is_negative());
    EXPECT_EQ(t2.hours(), 0h);
    EXPECT_EQ(t2.minutes(), 0min);
    EXPECT_EQ(t2.seconds(), 1s);
    EXPECT_EQ(t2.subseconds(), 500ms);
}

// ---------------- sys_days <-> year_month_day round trip ----------------
TEST(ChronoCalendar, SysDaysYearMonthDayRoundTrip)
{
    // Floor "today" to days and roundtrip through YMD.
    sys_days today = floor<days>(system_clock::now());
    year_month_day ymd{ today };
    sys_days back{ ymd };
    EXPECT_EQ(back, today);
}

// ---------------- Leap-year & last-day-of-month logic ----------------
TEST(ChronoCalendar, LeapYearAndLastDay)
{
    EXPECT_TRUE(year{ 2000 }.is_leap());   // divisible by 400
    EXPECT_FALSE(year{ 1900 }.is_leap());   // divisible by 100 but not 400
    EXPECT_TRUE(year{ 2020 }.is_leap());
    EXPECT_FALSE(year{ 2021 }.is_leap());

    year_month_day_last feb2000_last{ 2000y / February / last };
    year_month_day_last feb2021_last{ 2021y / February / last };
    sys_days d2000{ feb2000_last };
    sys_days d2021{ feb2021_last };
    year_month_day y2000{ d2000 };
    year_month_day y2021{ d2021 };
    EXPECT_EQ(unsigned(y2000.day()), 29u);
    EXPECT_EQ(unsigned(y2021.day()), 28u);
}

// ---------------- Arithmetic with months/years (clamping on month-end) ----------------
TEST(ChronoCalendar, MonthYearArithmeticClamp)
{
    year_month_day ymd = 2020y / February / 29;
    ymd += years{1};
    EXPECT_EQ(ymd, 2021y / February / 28); // clamps on non-leap year

    ymd = 2020y / January / 31;
    ymd += months{1};
    EXPECT_EQ(ymd, 2020y / February / 29); // 2020 is leap
    ymd += months{1};
    EXPECT_EQ(ymd, 2020y / March / 31);
}

// ---------------- Weekday computations & indexed/last forms ----------------
TEST(ChronoCalendar, WeekdayKnownAnchors)
{
    // 1970-01-01 was Thursday.
    sys_days epoch{ 1970y / January / 1 };
    weekday w_epoch{ epoch };
    EXPECT_EQ(w_epoch, Thursday);

    // 2000-01-01 was Saturday.
    sys_days y2k{ 2000y / January / 1 };
    weekday w_y2k{ y2k };
    EXPECT_EQ(w_y2k, Saturday);
}

TEST(ChronoCalendar, YearMonthWeekdayAndLast)
{
    // Second Friday of August 2023 is 2023-08-11.
    year_month_weekday ywd{ 2023y / August / weekday_indexed{Friday, 2} };
    EXPECT_EQ(sys_days{ ywd }, sys_days{ 2023y / August / 11 });

    // Last Monday of May 2024 was 2024-05-27.
    year_month_weekday_last ywdl{ 2024y / May / weekday_last{Monday} };
    EXPECT_EQ(sys_days{ ywdl }, sys_days{ 2024y / May / 27 });
}

// ---------------- Streaming (IOStreams) of calendar types ----------------
TEST(ChronoCalendar, StreamYearMonthDayAndWeekday)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic()); // ensure "C" locale
    oss << (2020y / February / 29);
    EXPECT_EQ(oss.str(), "2020-02-29");

    std::ostringstream oss2;
    oss2.imbue(std::locale::classic());
    oss2 << Thursday; // short English name in "C" locale
    EXPECT_EQ(oss2.str(), "Thu");
}

// ---------------- Parsing with from_stream (if available) ----------------
#if CHRONO_HAVE_PARSE
TEST(ChronoCalendar, ParseDateISO8601ToSysDays)
{
    std::istringstream iss("2020-02-29");
    iss.imbue(std::locale::classic());
    sys_days tp;
    std::chrono::from_stream(iss, "%F", tp); // %F = %Y-%m-%d
    EXPECT_TRUE(!iss.fail());
    EXPECT_EQ(tp, sys_days{ 2020y / February / 29 });
}

TEST(ChronoCalendar, ParseDateTimeToSysSeconds)
{
    std::istringstream iss("1970-01-01 12:34:56");
    iss.imbue(std::locale::classic());
    sys_time<seconds> tp;
    std::chrono::from_stream(iss, "%F %T", tp); // %T = %H:%M:%S
    EXPECT_TRUE(!iss.fail());
    EXPECT_EQ(tp, sys_days{ 1970y / January / 1 } + 12h + 34min + 56s);
}
#endif

TEST(ChronoCalendar, FormatDateAndTimePoint)
{
    auto d = sys_days{ 2020y / February / 29 };
    EXPECT_EQ(PString::format_string("{:%F}", d), "2020-02-29");

    auto tp = sys_days{ 1970y / January / 1 } + 12h + 34min + 56s;
    EXPECT_EQ(PString::format_string("{:%F %T}", tp), "1970-01-01 12:34:56");
}

TEST(ChronoCalendar, FormatDurationsQuantityUnitAndClocktime)
{
    EXPECT_EQ(PString::format_string("{:%Q%q}", 1500ms), "1500ms");    // quantity + unit
    EXPECT_EQ(PString::format_string("{:%T}", 3661s), "01:01:01");  // clock-style
}
